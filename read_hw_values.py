#!/usr/bin/env python3
"""
read_hw_values.py – nanoITLA live hardware sensor readout
==========================================================
Reads all real-time hardware measurement registers over SPI and displays
them in engineering units.  Read-only — safe to run while laser is operating.

Hardware setup (same as test_registers_pi.py):
  nanoITLA  <->  Raspberry Pi
  MISO      ->   GPIO 9  (MISO / pin 21)
  MOSI      ->   GPIO 10 (MOSI / pin 19)
  SCLK      ->   GPIO 11 (SCLK / pin 23)
  CS#       ->   GPIO 8  (CE0  / pin 24)

Usage:
  sudo python3 read_hw_values.py
  sudo python3 read_hw_values.py --speed 1000000   # 1 MHz for debugging
  sudo python3 read_hw_values.py --loop 2          # refresh every 2 s
"""

import spidev
import time
import argparse
import sys


# ---------------------------------------------------------------------------
# ITLA frame helpers (mirrors nanoITLA.c / test_registers_pi.py)
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


def spi_read(spi, reg):
    frame = build_inbound_frame(False, reg, 0)
    tx = [(frame >> 24) & 0xFF, (frame >> 16) & 0xFF,
          (frame >> 8) & 0xFF, frame & 0xFF]
    rx = spi.xfer2(tx)
    rframe = (rx[0] << 24) | (rx[1] << 16) | (rx[2] << 8) | rx[3]
    return parse_outbound_frame(rframe)


# ---------------------------------------------------------------------------
# Live hardware measurement registers
# Tuple: (reg, name, description, decode_fn)
#   decode_fn(raw_uint16) -> (value_str, unit_str)
#   Returns None on error.
# ---------------------------------------------------------------------------

def _signed16(raw):
    """Interpret raw uint16 as signed int16."""
    return raw if raw < 0x8000 else raw - 0x10000


def decode_thz(raw):
    return f"{raw}", "THz"

def decode_ghz10(raw):
    return f"{raw / 10.0:.1f}", "GHz"

def decode_centi_v(raw):
    """raw = V × 100"""
    return f"{raw / 100.0:.2f}", "V"

def decode_temp_x10(raw):
    """raw = °C × 10 (from g_temp * 10)"""
    return f"{raw / 10.0:.1f}", "°C"

def decode_casetemp(raw):
    """raw = °C × 100 (signed)"""
    return f"{_signed16(raw) / 100.0:.2f}", "°C"

def decode_pd_x10(raw):
    """raw = value × 10 (photodetector ADC counts × 10)"""
    return f"{raw / 10.0:.1f}", "ADC"

def decode_freq_mhz(raw):
    """raw = signed int16 MHz offset"""
    return f"{_signed16(raw)}", "MHz"

LIVE_REGISTERS = [
    # ── Laser frequency (active channel) ────────────────────────────────────
    (0x40, "LF1",             "Laser freq – THz part",            decode_thz),
    (0x41, "LF2",             "Laser freq – GHz×10 part",         decode_ghz10),
    # ── Frequency range limits ───────────────────────────────────────────────
    (0x42, "MinFreq_THz",     "Min lasing freq – THz",            decode_thz),
    (0x43, "MaxFreq_THz",     "Max lasing freq – THz",            decode_thz),
    (0x4F, "MinFreq_G10",     "Min lasing freq – GHz×10",         decode_ghz10),
    (0x50, "MinFreq_G10b",    "Min lasing freq – GHz×10 (dup)",   decode_ghz10),
    (0x51, "MaxFreq_THz_b",   "Max lasing freq – THz (dup)",      decode_thz),
    (0x52, "MaxFreq_G10",     "Max lasing freq – GHz×10",         decode_ghz10),
    (0x54, "LastFreq_THz",    "Last channel freq – THz",          decode_thz),
    (0x55, "LastFreq_G10",    "Last channel freq – GHz×10",       decode_ghz10),
    # ── Temperature ──────────────────────────────────────────────────────────
    (0x59, "CaseTemp",        "Case/PCB temperature",             decode_casetemp),
    # ── Manufacturer: DAC read-backs ─────────────────────────────────────────
    (0x80, "V1_rdac",         "Ring-1 voltage read-back",         decode_centi_v),
    (0x81, "V2_rdac",         "Ring-2 voltage read-back",         decode_centi_v),
    (0x82, "V3_rdac",         "Phase voltage read-back",          decode_centi_v),
    (0x83, "Gain_rdac",       "Gain bias read-back",              decode_centi_v),
    (0x84, "SOA_rdac",        "SOA current read-back",            decode_centi_v),
    (0x85, "Temp_rdac",       "Temperature ADC read-back",        decode_temp_x10),
    # ── Manufacturer: Photodetectors ─────────────────────────────────────────
    (0x86, "MPD",             "Main power detector (g_mpd)",      decode_pd_x10),
    (0x87, "WLPD",            "Etalon PD / wavelength lock",      decode_pd_x10),
    (0x88, "WMPD",            "Wavelength monitor PD",            decode_pd_x10),
    (0x89, "WMPD_meas",       "WMPD measured (alias)",            decode_pd_x10),
    (0x8A, "WLPD_meas",       "WLPD measured (alias)",            decode_pd_x10),
    (0x8B, "MPD_meas",        "MPD measured (alias)",             decode_pd_x10),
]


# ---------------------------------------------------------------------------
# Display
# ---------------------------------------------------------------------------

def read_and_print(spi):
    W = (4, 16, 32, 12, 6, 8)
    header = ("Reg", "Name", "Description", "Raw (hex)", "Value", "Unit")
    sep = "  ".join("-" * w for w in W)
    hdr = "  ".join(str(h).ljust(w) for h, w in zip(header, W))

    print()
    print("=" * (sum(W) + 2 * (len(W) - 1)))
    print("  nanoITLA Live Hardware Values")
    print("=" * (sum(W) + 2 * (len(W) - 1)))
    print(hdr)
    print(sep)

    errors = 0
    for reg, name, desc, decode in LIVE_REGISTERS:
        try:
            ce, xe, _, raw = spi_read(spi, reg)
        except Exception as ex:
            print("  ".join(s.ljust(w) for s, w in zip(
                [f"0x{reg:02X}", name, desc, "EXC", str(ex)[:10], ""], W)))
            errors += 1
            continue

        if ce:
            tag, val, unit = "CE-ERR", "—", "checksum error"
        elif xe:
            tag, val, unit = f"0x{raw:04X}", "XE", "execution error"
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
        description="nanoITLA live hardware sensor readout (read-only, Pi SPI)")
    parser.add_argument("--bus",   type=int, default=0,
                        help="SPI bus (default 0)")
    parser.add_argument("--dev",   type=int, default=0,
                        help="SPI device/CS (default 0)")
    parser.add_argument("--speed", type=int, default=10_000_000,
                        help="SPI clock Hz (default 10 MHz)")
    parser.add_argument("--mode",  type=int, default=0,
                        help="SPI mode 0-3 (default 0)")
    parser.add_argument("--loop",  type=float, default=0,
                        help="Refresh interval in seconds (0 = run once)")
    args = parser.parse_args()

    spi = spidev.SpiDev()
    spi.open(args.bus, args.dev)
    spi.max_speed_hz = args.speed
    spi.mode = args.mode
    spi.bits_per_word = 8

    print(f"SPI /dev/spidev{args.bus}.{args.dev}  "
          f"speed={args.speed/1e6:.1f} MHz  mode={args.mode}")

    try:
        if args.loop > 0:
            while True:
                read_and_print(spi)
                time.sleep(args.loop)
        else:
            read_and_print(spi)
    except KeyboardInterrupt:
        print("\nInterrupted.")
    finally:
        spi.close()


if __name__ == "__main__":
    main()
