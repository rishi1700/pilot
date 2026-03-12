#!/usr/bin/env python3
"""
test_registers_pi.py – nanoITLA SPI register test for Raspberry Pi
=====================================================================
Walks every implemented ITLA register (standard + manufacturer-specific),
reads each one, optionally writes a known value and reads back, then
prints a formatted table showing pass/fail for each register.

Hardware setup
--------------
  nanoITLA  <->  Raspberry Pi
  MISO      ->   GPIO 9  (MISO / pin 21)
  MOSI      ->   GPIO 10 (MOSI / pin 19)
  SCLK      ->   GPIO 11 (SCLK / pin 23)
  CS#       ->   GPIO 8  (CE0  / pin 24)
  GND       ->   GND
  3.3 V     ->   3.3 V

SPI settings
-----------
  Bus  : /dev/spidev0.0   (SPI0, CE0)
  Mode : 0 (CPOL=0, CPHA=0) — MSA OIF ITLA spec
  Bits : 8
  Speed: 10 MHz (reduce to 1 MHz if you see CE errors)

Usage
-----
  pip install spidev          # usually already present on Raspbian
  sudo python3 test_registers_pi.py
  sudo python3 test_registers_pi.py --speed 1000000   # 1 MHz for debugging
"""

import spidev
import time
import argparse
import sys

# ---------------------------------------------------------------------------
# ITLA frame helpers (mirrors nanoITLA.c build_inbound_frame logic)
# ---------------------------------------------------------------------------

def _bip4(b0, b1, b2, b3):
    """4-bit BIP checksum used by ITLA protocol."""
    bip8 = (b0 & 0x0F) ^ b1 ^ b2 ^ b3
    return ((bip8 >> 4) & 0x0F) ^ (bip8 & 0x0F)


def build_inbound_frame(is_write: bool, reg: int, data: int, lst_rsp: int = 0) -> int:
    """Pack a 32-bit ITLA inbound frame."""
    isw = 1 if is_write else 0
    app = (isw << 26) | ((reg & 0xFF) << 18) | (data & 0xFFFF)
    tmp = ((lst_rsp & 1) << 27) | (app & 0x07FFFFFF)
    b0 = (tmp >> 24) & 0xFF
    b1 = (tmp >> 16) & 0xFF
    b2 = (tmp >> 8) & 0xFF
    b3 = tmp & 0xFF
    csum = _bip4(b0 & 0x0F, b1, b2, b3) & 0x0F
    return tmp | (csum << 28)


def parse_outbound_frame(frame: int):
    """
    Decode a 32-bit ITLA outbound (device → host) frame.
    Returns (ce, xe, reg, data).
      ce  – checksum error from device perspective
      xe  – execution error (register not implemented / read-only violation)
      reg – echoed register number
      data – register value
    """
    csum = (frame >> 28) & 0x0F
    b0 = (frame >> 24) & 0xFF
    b1 = (frame >> 16) & 0xFF
    b2 = (frame >> 8) & 0xFF
    b3 = frame & 0xFF
    rc = _bip4(b0 & 0x0F, b1, b2, b3) & 0x0F
    our_ce = 1 if rc != csum else 0     # CE we compute locally
    ce = (frame >> 27) & 1              # CE bit set by device
    xe = (frame >> 25) & 1
    reg = (frame >> 17) & 0xFF
    data = (frame >> 1) & 0xFFFF
    return ce | our_ce, xe, reg, data


def frame_to_bytes(frame: int):
    """Convert 32-bit frame to 4-byte list (big-endian)."""
    return [(frame >> 24) & 0xFF,
            (frame >> 16) & 0xFF,
            (frame >> 8) & 0xFF,
            frame & 0xFF]


def bytes_to_frame(b):
    """Convert 4-byte list (big-endian) back to 32-bit int."""
    return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3]


# ---------------------------------------------------------------------------
# SPI transaction helper
# ---------------------------------------------------------------------------

def spi_transact(spi, is_write: bool, reg: int, data: int = 0, lst_rsp: int = 0):
    """
    Send one 4-byte ITLA frame and return (ce, xe, reg_echo, data_back).
    """
    frame = build_inbound_frame(is_write, reg, data, lst_rsp)
    tx = frame_to_bytes(frame)
    rx = spi.xfer2(tx)
    rframe = bytes_to_frame(rx)
    return parse_outbound_frame(rframe)


def spi_read(spi, reg: int, lst_rsp: int = 0):
    return spi_transact(spi, False, reg, 0, lst_rsp)


def spi_write(spi, reg: int, data: int, lst_rsp: int = 0):
    return spi_transact(spi, True, reg, data, lst_rsp)


# ---------------------------------------------------------------------------
# Register table
# Each entry: (reg_hex, name, access, test_write_val_or_None, description)
#   access: 'RO' | 'WO' | 'RW' | 'AEA'
#   test_write_val: value to write and read-back verify (None = read-only test)
# ---------------------------------------------------------------------------

REGISTERS = [
    # reg    name              access  test_wr   description
    (0x00, "NOP",             "RO",   None,     "No-operation / poll status"),
    (0x01, "DevTyp",          "AEA",  None,     "Device type string (AEA)"),
    (0x02, "Mfgr",            "AEA",  None,     "Manufacturer string (AEA)"),
    (0x03, "Model",           "AEA",  None,     "Model string (AEA)"),
    (0x04, "SerNo",           "AEA",  None,     "Serial number string (AEA)"),
    (0x05, "MfgDate",         "AEA",  None,     "Manufacturing date string (AEA)"),
    (0x06, "Release",         "AEA",  None,     "Firmware release string (AEA)"),
    (0x07, "RelBack",         "AEA",  None,     "Firmware back-level string (AEA)"),
    (0x08, "GenCfg",          "RW",   0x0001,   "General configuration"),
    (0x09, "AEA_EAC",         "RW",   0x0003,   "AEA extension access control"),
    (0x0A, "AEA_EA",          "RW",   0x0000,   "AEA extension address"),
    (0x0D, "IOCap",           "RW",   0x0000,   "I/O capability"),
    (0x0E, "EAC",             "RW",   0x0000,   "Extended access control"),
    (0x0F, "EA",              "RO",   None,     "Extended access data"),
    (0x10, "Channel",         "RW",   0x0001,   "Laser channel (low word)"),
    (0x14, "DLConfig",        "RW",   0x0000,   "Dither/lock config"),
    (0x15, "DLStatus",        "RO",   None,     "Dither/lock status"),
    (0x20, "StatusF",         "RW",   0x0000,   "Fatal status"),
    (0x21, "StatusW",         "RW",   0x0000,   "Warning status"),
    (0x22, "FPowTh",          "RW",   0x0064,   "Fatal power threshold"),
    (0x23, "WPowTh",          "RW",   0x0032,   "Warning power threshold"),
    (0x24, "FFreqTh",         "RW",   0x0064,   "Fatal frequency threshold"),
    (0x25, "WFreqTh",         "RW",   0x0032,   "Warning frequency threshold"),
    (0x26, "FThermTh",        "RW",   0x0064,   "Fatal thermal threshold"),
    (0x27, "WThermTh",        "RW",   0x0032,   "Warning thermal threshold"),
    (0x28, "SRQ_MASK",        "RW",   0x0000,   "SRQ mask"),
    (0x29, "FatalT",          "RW",   0x0000,   "Fatal trigger config"),
    (0x2A, "ALMT",            "RW",   0x0000,   "Alarm mask"),
    (0x30, "Power/GRID",      "RW",   0x0000,   "Power / grid composite"),
    (0x31, "PWR",             "RW",   0x0064,   "Laser output power (0.01 dBm)"),
    (0x32, "ResEna",          "RW",   0x0000,   "Resource enable"),
    (0x33, "MCB",             "RW",   0x0000,   "Module control byte"),
    (0x34, "Grid",            "RW",   0x0019,   "Channel grid spacing (GHz×10)"),
    (0x35, "FCF1_THz",        "RW",   0x00C1,   "First channel freq – THz part (193)"),
    (0x36, "FCF2_G10",        "RW",   0x1194,   "First channel freq – GHz×10 (450.0)"),
    (0x40, "LF1",             "RO",   None,     "Laser frequency – THz part"),
    (0x41, "LF2",             "RO",   None,     "Laser frequency – GHz×10 part"),
    (0x42, "MinFreq_THz",     "RO",   None,     "Min lasing frequency – THz part"),
    (0x43, "MaxFreq_THz",     "RO",   None,     "Max lasing frequency – THz part"),
    (0x4F, "MinFreq_G10",     "RO",   None,     "Min lasing frequency – GHz×10 part"),
    (0x50, "MinFreq_G10",     "RO",   None,     "Min lasing frequency – GHz×10 part"),
    (0x51, "MaxFreq_THz",     "RO",   None,     "Max lasing frequency – THz part"),
    (0x52, "MaxFreq_G10",     "RO",   None,     "Max lasing frequency – GHz×10 part"),
    (0x53, "MinPower",        "RO",   None,     "Min optical power (dBm×100)"),
    (0x54, "LastFreq_THz",    "RO",   None,     "Last channel freq – THz"),
    (0x55, "LastFreq_G10",    "RO",   None,     "Last channel freq – GHz×10"),
    (0x56, "LGrid10",         "RO",   None,     "Laser grid step (GHz×10)"),
    (0x59, "CaseTemp",        "RO",   None,     "Case/PCB temperature (°C×100)"),
    (0x62, "FTF",             "RW",   0x0000,   "Fine tune frequency (MHz, signed)"),
    (0x65, "ChannelH",        "RW",   0x0000,   "Laser channel (high word)"),
    (0x66, "ChannelL",        "RW",   0x0001,   "Laser channel (low word, alias)"),
    (0x67, "FCF3_MHz",        "RW",   0x0000,   "First channel freq – MHz part"),
    (0x68, "Grid2_MHz",       "RW",   0x0000,   "Grid 2 – MHz offset (signed)"),
    # ── Manufacturer-specific LUT / PD window (0x80–0x8B) ──────────────────
    (0x80, "V1_rdac",         "RO",   None,     "Mfr: Ring-1 voltage read-back (×100 → V)"),
    (0x81, "V2_rdac",         "RO",   None,     "Mfr: Ring-2 voltage read-back (×100 → V)"),
    (0x82, "V3_rdac",         "RO",   None,     "Mfr: Phase voltage read-back (×100 → V)"),
    (0x83, "Gain_rdac",       "RO",   None,     "Mfr: Gain bias read-back (×100 → V)"),
    (0x84, "SOA_rdac",        "RO",   None,     "Mfr: SOA current read-back (×100 → V)"),
    (0x85, "Temp_rdac",       "RO",   None,     "Mfr: Temperature read-back (×100)"),
    (0x86, "Power_PD",        "RO",   None,     "Mfr: Main power detector (MPD) ADC"),
    (0x87, "Etalon_PD",       "RO",   None,     "Mfr: Etalon PD (WLPD) ADC"),
    (0x88, "WM_PD",           "RO",   None,     "Mfr: Wavelength monitor PD (WMPD) ADC"),
    (0x89, "WM_PD_alias",     "RO",   None,     "Mfr: WMPD alias (= 0x88)"),
    (0x8A, "Etalon_PD_alias", "RO",   None,     "Mfr: WLPD alias (= 0x87)"),
    (0x8B, "Power_PD_alias",  "RO",   None,     "Mfr: MPD alias (= 0x86)"),
    # ── Manufacturer-specific tuner R/W (0x8C–0x91) ─────────────────────────
    (0x8C, "PHASE_tuner",     "RW",   0x0064,   "Mfr: Phase tuner (÷100 → V, e.g. 100=1.00 V)"),
    (0x8D, "RING1_tuner",     "RW",   0x00C8,   "Mfr: Ring-1 tuner (÷100 → V, e.g. 200=2.00 V)"),
    (0x8E, "RING2_tuner",     "RW",   0x00C8,   "Mfr: Ring-2 tuner (÷100 → V, e.g. 200=2.00 V)"),
    (0x8F, "SOA_tuner",       "RW",   0x0064,   "Mfr: SOA current tuner (÷100 → V)"),
    (0x90, "GainBias_tuner",  "RW",   0x0064,   "Mfr: Gain bias tuner (÷100 → V)"),
    (0x91, "TEC_raw",         "RW",   0x0019,   "Mfr: TEC raw signed 16-bit value"),
]

# ---------------------------------------------------------------------------
# Test runner
# ---------------------------------------------------------------------------

PASS = "PASS"
FAIL = "FAIL"
SKIP = "SKIP"
WARN = "WARN"

COL_WIDTHS = (4, 16, 6, 7, 7, 7, 40)
HEADER = ("Reg", "Name", "Access", "Read", "Write", "Verify", "Notes")


def fmt_row(*fields):
    return "  ".join(str(f).ljust(w) for f, w in zip(fields, COL_WIDTHS))


def run_tests(spi, verbose=False):
    print()
    print("=" * 95)
    print("  nano-ITLA Register Test  —  Pilot Photonics nanoITLA-01")
    print("=" * 95)
    print(fmt_row(*HEADER))
    print("-" * 95)

    results = []
    passed = failed = skipped = warned = 0

    for reg, name, access, test_val, desc in REGISTERS:
        read_result = "—"
        write_result = "—"
        verify_result = "—"
        notes = ""

        # ── READ ────────────────────────────────────────────────────────────
        try:
            ce, xe, reg_echo, val = spi_read(spi, reg)
            if ce:
                read_result = "CE-ERR"
                notes = "checksum error on read"
            elif xe:
                read_result = f"XE({val:#06x})"
                notes = "register returned execution error"
            else:
                read_result = f"{val:#06x}"
        except Exception as ex:
            read_result = "EXC"
            notes = str(ex)

        # ── WRITE + READ-BACK ────────────────────────────────────────────────
        if test_val is not None and access in ("RW",):
            try:
                wce, wxe, _, _ = spi_write(spi, reg, test_val)
                time.sleep(0.002)   # 2 ms settle

                if wce:
                    write_result = "CE-ERR"
                    notes = "checksum error on write"
                elif wxe:
                    write_result = f"XE"
                    notes = "write returned execution error"
                else:
                    write_result = f"{test_val:#06x}"

                    # read back
                    rce, rxe, _, rval = spi_read(spi, reg)
                    if rce:
                        verify_result = "CE-ERR"
                    elif rxe:
                        verify_result = "XE"
                    elif rval == test_val:
                        verify_result = PASS
                    else:
                        verify_result = f"MISMATCH({rval:#06x})"
                        notes = f"expected {test_val:#06x} got {rval:#06x}"

            except Exception as ex:
                write_result = "EXC"
                notes = str(ex)

        # ── Score ───────────────────────────────────────────────────────────
        if access == "RO" or access == "AEA":
            # Just reading — pass if no CE/XE or AEA response (status=2)
            if "CE-ERR" in read_result or "EXC" in read_result:
                overall = FAIL
                failed += 1
            else:
                overall = PASS
                passed += 1
        else:
            if verify_result == PASS:
                overall = PASS
                passed += 1
            elif verify_result == "—":
                # write wasn't attempted (AEA, WO, etc.)
                overall = SKIP
                skipped += 1
            elif "MISMATCH" in verify_result:
                overall = FAIL
                failed += 1
            elif "EXC" in (write_result, verify_result) or "CE-ERR" in (write_result, verify_result):
                overall = FAIL
                failed += 1
            else:
                overall = WARN
                warned += 1

        # Truncate notes to fit column
        notes_col = (desc if not notes else notes)[:COL_WIDTHS[-1]]

        row = fmt_row(f"0x{reg:02X}", name, access, read_result, write_result, verify_result, notes_col)
        print(row)
        results.append((reg, name, overall))

        if verbose and notes:
            print(f"      note: {notes}")

    print("-" * 95)
    print(f"  TOTAL: {len(results)}   PASS: {passed}   FAIL: {failed}   WARN: {warned}   SKIP: {skipped}")
    print("=" * 95)
    print()
    return failed == 0


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description="nanoITLA SPI register test (Raspberry Pi)")
    parser.add_argument("--bus",   type=int, default=0,        help="SPI bus (default 0)")
    parser.add_argument("--dev",   type=int, default=0,        help="SPI device/CS (default 0)")
    parser.add_argument("--speed", type=int, default=10_000_000, help="SPI clock Hz (default 10 MHz)")
    parser.add_argument("--mode",  type=int, default=0,        help="SPI mode 0-3 (default 0)")
    parser.add_argument("-v", "--verbose", action="store_true", help="Print extra detail on failures")
    args = parser.parse_args()

    spi = spidev.SpiDev()
    spi.open(args.bus, args.dev)
    spi.max_speed_hz = args.speed
    spi.mode = args.mode
    spi.bits_per_word = 8

    print(f"SPI /dev/spidev{args.bus}.{args.dev}  speed={args.speed/1e6:.1f} MHz  mode={args.mode}")

    try:
        ok = run_tests(spi, verbose=args.verbose)
    finally:
        spi.close()

    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
