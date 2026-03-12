#!/usr/bin/env python3
"""
read_hw_values.py – nanoITLA live hardware sensor readout
==========================================================
Reads all real-time hardware measurement registers over UART and displays
them in engineering units.  Read-only — safe to run while laser is operating.

The firmware implements the ITLA protocol over UART0 at 115200 8N1.
The Pi talks to the MCU via a USB-UART adapter or GPIO UART pins.

Hardware setup:
  nanoITLA UART TX  ->  Pi RX  (/dev/ttyUSB0 or /dev/ttyAMA0)
  nanoITLA UART RX  ->  Pi TX
  GND               ->  GND

Usage:
  pip install pyserial
  sudo python3 read_hw_values.py
  sudo python3 read_hw_values.py --port /dev/ttyAMA0
  sudo python3 read_hw_values.py --loop 2          # refresh every 2 s
"""

import serial
import time
import argparse
import sys


# ---------------------------------------------------------------------------
# ITLA frame helpers (mirrors nanoITLA.c)
# ---------------------------------------------------------------------------

def _bip4(b0, b1, b2, b3):
    bip8 = (b0 & 0x0F) ^ b1 ^ b2 ^ b3
    return ((bip8 >> 4) & 0x0F) ^ (bip8 & 0x0F)


def build_inbound_frame(is_write, reg, data, lst_rsp=0):
    isw = 1 if is_write else 0
    app = (isw << 26) | ((reg & 0xFF) << 18) | (data & 0xFFFF)
    tmp = ((lst_rsp & 1) << 27) | (app & 0x07FFFFFF)
    b0 = (tmp >> 24) & 0xFF
    b1 = (tmp >> 16) & 0xFF
    b2 = (tmp >> 8) & 0xFF
    b3 = tmp & 0xFF
    csum = _bip4(b0 & 0x0F, b1, b2, b3) & 0x0F
    return tmp | (csum << 28)


def parse_outbound_frame(frame):
    csum = (frame >> 28) & 0x0F
    b0 = (frame >> 24) & 0xFF
    b1 = (frame >> 16) & 0xFF
    b2 = (frame >> 8) & 0xFF
    b3 = frame & 0xFF
    rc = _bip4(b0 & 0x0F, b1, b2, b3) & 0x0F
    our_ce = 1 if rc != csum else 0
    ce = (frame >> 27) & 1
    xe = (frame >> 25) & 1
    reg = (frame >> 17) & 0xFF
    data = (frame >> 1) & 0xFFFF
    return ce | our_ce, xe, reg, data


def uart_read(ser, reg):
    """Send one 4-byte ITLA read frame over UART, return (ce, xe, reg_echo, data)."""
    frame = build_inbound_frame(False, reg, 0)
    tx = bytes([(frame >> 24) & 0xFF, (frame >> 16) & 0xFF,
                (frame >> 8) & 0xFF, frame & 0xFF])
    ser.reset_input_buffer()
    ser.write(tx)
    rx = ser.read(4)
    if len(rx) < 4:
        raise TimeoutError(f"UART timeout: got {len(rx)}/4 bytes")
    rframe = (rx[0] << 24) | (rx[1] << 16) | (rx[2] << 8) | rx[3]
    return parse_outbound_frame(rframe)


# ---------------------------------------------------------------------------
# Live hardware measurement registers + decode functions
# ---------------------------------------------------------------------------

def _signed16(raw):
    return raw if raw < 0x8000 else raw - 0x10000

def decode_thz(raw):      return f"{raw}", "THz"
def decode_ghz10(raw):    return f"{raw / 10.0:.1f}", "GHz"
def decode_centi_v(raw):  return f"{raw / 100.0:.2f}", "V"
def decode_temp_x10(raw): return f"{_signed16(raw) / 10.0:.1f}", "°C"
def decode_casetemp(raw): return f"{_signed16(raw) / 100.0:.2f}", "°C"
def decode_pd_x10(raw):   return f"{raw / 10.0:.1f}", "ADC"

LIVE_REGISTERS = [
    # ── Laser frequency ──────────────────────────────────────────────────────
    (0x40, "LF1",          "Laser freq – THz part",           decode_thz),
    (0x41, "LF2",          "Laser freq – GHz×10 part",        decode_ghz10),
    # ── Freq limits ──────────────────────────────────────────────────────────
    (0x42, "MinFreq_THz",  "Min lasing freq – THz",           decode_thz),
    (0x43, "MaxFreq_THz",  "Max lasing freq – THz",           decode_thz),
    (0x4F, "MinFreq_G10",  "Min lasing freq – GHz×10",        decode_ghz10),
    (0x51, "MaxFreq_THz2", "Max lasing freq – THz (dup)",     decode_thz),
    (0x52, "MaxFreq_G10",  "Max lasing freq – GHz×10",        decode_ghz10),
    (0x54, "LastFreq_THz", "Last channel freq – THz",         decode_thz),
    (0x55, "LastFreq_G10", "Last channel freq – GHz×10",      decode_ghz10),
    # ── Temperature ──────────────────────────────────────────────────────────
    (0x59, "CaseTemp",     "Case/PCB temperature",            decode_casetemp),
    # ── Manufacturer: DAC read-backs ─────────────────────────────────────────
    (0x80, "V1_rdac",      "Ring-1 voltage read-back",        decode_centi_v),
    (0x81, "V2_rdac",      "Ring-2 voltage read-back",        decode_centi_v),
    (0x82, "V3_rdac",      "Phase voltage read-back",         decode_centi_v),
    (0x83, "Gain_rdac",    "Gain bias read-back",             decode_centi_v),
    (0x84, "SOA_rdac",     "SOA current read-back",           decode_centi_v),
    (0x85, "Temp_rdac",    "Temperature ADC read-back",       decode_temp_x10),
    # ── Manufacturer: Photodetectors ─────────────────────────────────────────
    (0x86, "MPD",          "Main power detector (g_mpd)",     decode_pd_x10),
    (0x87, "WLPD",         "Etalon PD / wavelength lock",     decode_pd_x10),
    (0x88, "WMPD",         "Wavelength monitor PD",           decode_pd_x10),
    (0x89, "WMPD_meas",    "WMPD measured (alias)",           decode_pd_x10),
    (0x8A, "WLPD_meas",    "WLPD measured (alias)",           decode_pd_x10),
    (0x8B, "MPD_meas",     "MPD measured (alias)",            decode_pd_x10),
]


# ---------------------------------------------------------------------------
# Display
# ---------------------------------------------------------------------------

def read_and_print(ser):
    W = (4, 14, 32, 10, 8, 8)
    header = ("Reg", "Name", "Description", "Raw(hex)", "Value", "Unit")
    sep = "  ".join("-" * w for w in W)

    print()
    print("=" * (sum(W) + 2 * (len(W) - 1)))
    print("  nanoITLA Live Hardware Values  (UART)")
    print("=" * (sum(W) + 2 * (len(W) - 1)))
    print("  ".join(str(h).ljust(w) for h, w in zip(header, W)))
    print(sep)

    errors = 0
    for reg, name, desc, decode in LIVE_REGISTERS:
        try:
            ce, xe, _, raw = uart_read(ser, reg)
        except Exception as ex:
            row = [f"0x{reg:02X}", name, desc[:W[2]], "TIMEOUT", str(ex)[:8], ""]
            print("  ".join(str(s).ljust(w) for s, w in zip(row, W)))
            errors += 1
            continue

        if ce:
            tag, val, unit = "CE-ERR", "—", "checksum err"
        elif xe:
            tag, val, unit = f"0x{raw:04X}", "XE", "exec error"
            errors += 1
        else:
            tag = f"0x{raw:04X}"
            val, unit = decode(raw)

        row = [f"0x{reg:02X}", name, desc[:W[2]], tag, val[:W[4]], unit[:W[5]]]
        print("  ".join(str(s).ljust(w) for s, w in zip(row, W)))

    print(sep)
    if errors:
        print(f"  {errors} register(s) returned errors\n")
    else:
        print("  All registers read OK\n")


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="nanoITLA live hardware sensor readout (UART 115200 8N1)")
    parser.add_argument("--port",    default="/dev/ttyUSB0",
                        help="Serial port (default /dev/ttyUSB0)")
    parser.add_argument("--baud",    type=int, default=115200,
                        help="Baud rate (default 115200)")
    parser.add_argument("--timeout", type=float, default=0.5,
                        help="Read timeout seconds (default 0.5)")
    parser.add_argument("--loop",    type=float, default=0,
                        help="Refresh interval in seconds (0 = run once)")
    args = parser.parse_args()

    ser = serial.Serial(args.port, args.baud, timeout=args.timeout)
    time.sleep(0.1)  # let UART settle
    ser.reset_input_buffer()

    print(f"UART {args.port}  baud={args.baud}  timeout={args.timeout}s")

    try:
        if args.loop > 0:
            while True:
                read_and_print(ser)
                time.sleep(args.loop)
        else:
            read_and_print(ser)
    except KeyboardInterrupt:
        print("\nInterrupted.")
    finally:
        ser.close()


if __name__ == "__main__":
    main()
