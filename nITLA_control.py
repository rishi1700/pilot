#!/usr/bin/env python3
"""
nITLA_control.py – MSA-protocol adapter for nITLA_Control_lib
==============================================================
Drop-in replacement for 'nITLA_Control_lib 1.py' that talks the
ITLA binary protocol (115200 8N1) instead of ASCII commands.

The original library used ASCII commands (GAIN#, SR1V#, …) which are
not present in the current MSA firmware.  This adapter uses the MSA
tuner registers (0x8C–0x91) which the firmware applies to hardware
every main-loop iteration via SetR1/SetR2/SetPhase/SetSOA/SetGain.

Register mapping:
  0x8C  Phase  tuner  (centi-V,  write V*100)   → g_v3 → SetPhase()
  0x8D  Ring-1 tuner  (centi-V,  write V*100)   → g_v1 → SetR1()
  0x8E  Ring-2 tuner  (centi-V,  write V*100)   → g_v2 → SetR2()
  0x8F  SOA    tuner  (centi-mA, write mA*100)  → g_soa → SetSOA()
  0x90  Gain   tuner  (centi-mA, write mA*100)  → g_gain → SetGain()
  0x91  TEC    raw    (signed,   write raw)      → g_temp → SetTemp()
  0x89  WMPD   ADC    (÷10 = mV, read-only)
  0x8A  WLPD   ADC    (÷10 = mV, read-only)
  0x8B  MPD    ADC    (÷10 = mV, read-only)

Usage:
  from nITLA_control import nITLA, LookUpTable

  laser = nITLA(port="/dev/ttyUSB0",
                fpath="LUTs/Unit5_Boxed_CenterMode_T20_Ta20_Ig120_FullLUT.csv")
  laser.set_frequency(0)
  T, MPD, WLPD, WMPD = laser.read_feedback()
  laser.shutdown()
"""

import serial
import time
import pandas as pd

# ---------------------------------------------------------------------------
# ITLA frame helpers (same as read_hw_values.py / test_registers_pi.py)
# ---------------------------------------------------------------------------

def _bip4(b0, b1, b2, b3):
    bip8 = (b0 & 0x0F) ^ b1 ^ b2 ^ b3
    return ((bip8 >> 4) & 0x0F) ^ (bip8 & 0x0F)

def _build_frame(is_write, reg, data):
    isw = 1 if is_write else 0
    tmp = (isw << 26) | ((reg & 0xFF) << 18) | (data & 0xFFFF)
    b0 = (tmp >> 24) & 0xFF
    b1 = (tmp >> 16) & 0xFF
    b2 = (tmp >>  8) & 0xFF
    b3 =  tmp        & 0xFF
    csum = _bip4(b0 & 0x0F, b1, b2, b3) & 0x0F
    frame = tmp | (csum << 28)
    return bytes([(frame >> 24) & 0xFF, (frame >> 16) & 0xFF,
                  (frame >>  8) & 0xFF,  frame        & 0xFF])

def _parse_frame(rx):
    frame = (rx[0] << 24) | (rx[1] << 16) | (rx[2] << 8) | rx[3]
    csum  = (frame >> 28) & 0x0F
    b0    = (frame >> 24) & 0xFF
    b1    = (frame >> 16) & 0xFF
    b2    = (frame >>  8) & 0xFF
    b3    =  frame        & 0xFF
    ce    = 0 if (_bip4(b0 & 0x0F, b1, b2, b3) & 0x0F) == csum else 1
    xe    = (frame >> 25) & 1
    data  = (frame >>  1) & 0xFFFF
    return ce, xe, data


# ---------------------------------------------------------------------------
# LookUpTable  (same interface as nITLA_Control_lib 1.py)
# ---------------------------------------------------------------------------

class LookUpTable:
    """
    Load a calibration CSV produced by the characterisation station.

    Expected columns (by index):
      0  f_set   – target frequency (THz)
      1  f_real  – measured frequency (THz); rows where f_real==0 are skipped
      2  w_real  – measured wavelength (nm)
      3  V1      – Ring-1 voltage (V)
      4  V2      – Ring-2 voltage (V)
      5  V3      – Phase voltage (V)
      6  Ig      – Gain current (mA)
      7  Is      – SOA current (mA)
      10 PD1     – MPD ADC counts
      11 PD2     – WLPD ADC counts
      12 PD3     – WMPD ADC counts
      13 VTEC    – TEC voltage (V)
      23 validated (optional)
    """
    def __init__(self, fpath):
        data   = pd.read_csv(fpath)
        k      = data.keys()
        f_real = data[k[1]]
        valid  = f_real != 0.

        self.f_lst    = data[k[0]][valid].to_list()
        self.w_lst    = data[k[2]][valid].to_list()
        self.V1_lst   = data[k[3]][valid].to_list()
        self.V2_lst   = data[k[4]][valid].to_list()
        self.V3_lst   = data[k[5]][valid].to_list()
        self.Ig_lst   = data[k[6]][valid].to_list()
        self.Is_lst   = data[k[7]][valid].to_list()
        self.PD1_lst  = data[k[10]][valid].to_list()
        self.PD2_lst  = data[k[11]][valid].to_list()
        self.PD3_lst  = data[k[12]][valid].to_list()
        self.VTEC_lst = data[k[13]][valid].to_list()

        try:
            self.validated_lst = data[k[23]][valid].to_list()
        except Exception:
            self.validated_lst = None

    def __len__(self):
        return len(self.f_lst)


# ---------------------------------------------------------------------------
# nITLA  – MSA-protocol version
# ---------------------------------------------------------------------------

class nITLA:
    """
    nITLA control via ITLA MSA binary protocol.

    Parameters
    ----------
    port  : serial port string, e.g. "/dev/ttyUSB0" (Linux/Pi) or "COM3" (Windows)
    fpath : path to calibration CSV (LookUpTable format above)
    baud  : baud rate (default 115200)
    """

    def __init__(self, port="/dev/ttyUSB0", fpath=None, baud=115200):
        self.ser = serial.Serial(
            port=port,
            baudrate=baud,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=0.5
        )
        if not self.ser.is_open:
            self.ser.open()
        time.sleep(0.1)
        self.ser.reset_input_buffer()

        self.LUT = LookUpTable(fpath) if fpath else None
        print(f"nITLA connected on {port}  "
              f"({'%d LUT entries' % len(self.LUT) if self.LUT else 'no LUT'})")

    # ------------------------------------------------------------------
    # Low-level register access
    # ------------------------------------------------------------------

    def _write_reg(self, reg, value):
        """Write a 16-bit value to a register. Raises on CE/XE."""
        tx = _build_frame(True, reg, value & 0xFFFF)
        self.ser.reset_input_buffer()
        self.ser.write(tx)
        rx = self.ser.read(4)
        if len(rx) < 4:
            raise TimeoutError(f"Reg 0x{reg:02X} write timeout")
        ce, xe, _ = _parse_frame(rx)
        if ce:
            raise IOError(f"Reg 0x{reg:02X} write: checksum error")
        if xe:
            raise IOError(f"Reg 0x{reg:02X} write: execution error (value={value})")

    def _read_reg(self, reg):
        """Read a register, return raw 16-bit value."""
        tx = _build_frame(False, reg, 0)
        self.ser.reset_input_buffer()
        self.ser.write(tx)
        rx = self.ser.read(4)
        if len(rx) < 4:
            raise TimeoutError(f"Reg 0x{reg:02X} read timeout")
        ce, xe, data = _parse_frame(rx)
        if ce:
            raise IOError(f"Reg 0x{reg:02X} read: checksum error")
        if xe:
            raise IOError(f"Reg 0x{reg:02X} read: execution error")
        return data

    # ------------------------------------------------------------------
    # Hardware setters  (same method names as nITLA_Control_lib 1.py)
    # ------------------------------------------------------------------

    def set_current(self, I_mA, sec="G"):
        """Set Gain (sec='G') or SOA (sec='S') current in mA."""
        if sec == "G":
            self._write_reg(0x90, int(round(I_mA * 100)))   # centi-mA
        elif sec == "S":
            self._write_reg(0x8F, int(round(I_mA * 100)))   # centi-mA
        else:
            raise ValueError("sec must be 'G' or 'S'")

    def set_tuner_voltage(self, V, sec="R1"):
        """Set Ring-1 (R1), Ring-2 (R2) or Phase (P) voltage in V."""
        val = int(round(abs(V) * 100))                       # centi-V
        if sec == "R1":
            self._write_reg(0x8D, val)
        elif sec == "R2":
            self._write_reg(0x8E, val)
        elif sec == "P":
            self._write_reg(0x8C, val)
        else:
            raise ValueError("sec must be 'R1', 'R2' or 'P'")

    def set_TEC(self, T_raw):
        """Write raw TEC setpoint to register 0x91 (signed 16-bit)."""
        self._write_reg(0x91, int(T_raw) & 0xFFFF)

    def blank_V(self):
        """Zero all tuner voltages."""
        for reg in (0x8C, 0x8D, 0x8E):
            self._write_reg(reg, 0)

    # ------------------------------------------------------------------
    # High-level: set_frequency from LUT row
    # ------------------------------------------------------------------

    def set_frequency(self, tab_index, blank=True):
        """
        Apply a full LUT row by index.

        tab_index : index into the valid rows of the loaded LUT
        blank     : if True, zero and re-apply gain to minimise hysteresis
                    (mirrors the original library behaviour)
        """
        if self.LUT is None:
            raise RuntimeError("No LUT loaded — pass fpath= to constructor")

        self.set_tuner_voltage(self.LUT.V1_lst[tab_index],  "R1")
        self.set_tuner_voltage(self.LUT.V2_lst[tab_index],  "R2")
        self.set_tuner_voltage(self.LUT.V3_lst[tab_index],  "P")
        self.set_current(self.LUT.Is_lst[tab_index], "S")
        self.set_current(self.LUT.Ig_lst[tab_index], "G")

        if blank:
            self.set_current(0, "G")
            time.sleep(0.05)
            self.set_current(self.LUT.Ig_lst[tab_index], "G")

        return self.LUT.f_lst[tab_index], self.LUT.w_lst[tab_index]

    # ------------------------------------------------------------------
    # Read live hardware feedback
    # ------------------------------------------------------------------

    def read_feedback(self):
        """
        Read live ADC photodetector values.
        Returns (MPD, WLPD, WMPD) in ADC mV units (÷10 of raw).

        Matches original return signature where possible.
        Note: temperature is not yet wired to a readable register in the
        current firmware; returns None for T until itla_update_hw_telemetry
        is plumbed to a dedicated register.
        """
        wmpd_raw = self._read_reg(0x89)
        wlpd_raw = self._read_reg(0x8A)
        mpd_raw  = self._read_reg(0x8B)

        MPD  = wmpd_raw / 10.0
        WLPD = wlpd_raw / 10.0
        WMPD = mpd_raw  / 10.0

        return MPD, WLPD, WMPD

    # ------------------------------------------------------------------
    # Shutdown / close
    # ------------------------------------------------------------------

    def shutdown(self):
        """Zero all drive signals and close the port."""
        try:
            self.set_current(0, "G")
            self.set_current(0, "S")
            self.blank_V()
            self.set_TEC(0)
        finally:
            self.close()

    def close(self):
        if self.ser.is_open:
            self.ser.close()
