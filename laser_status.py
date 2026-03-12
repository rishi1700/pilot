#!/usr/bin/env python3
"""
laser_status.py – nanoITLA human-readable laser status dashboard
=================================================================
Reads the key operating registers and displays a clean summary of:
  - Lock status and current wavelength / frequency
  - Temperature (chip, TEC)
  - Output power (MPD photodetector)
  - Decoded alarm flags (StatusF / StatusW)
  - Ring/SOA/Gain drive voltages

Usage:
  sudo python3 laser_status.py
  sudo python3 laser_status.py --port /dev/ttyAMA0
  sudo python3 laser_status.py --loop 2     # refresh every 2 s
"""

import serial, time, argparse, sys

# ---------------------------------------------------------------------------
# ITLA frame helpers
# ---------------------------------------------------------------------------

def _bip4(b0, b1, b2, b3):
    x = (b0 & 0xF) ^ b1 ^ b2 ^ b3
    return ((x >> 4) & 0xF) ^ (x & 0xF)

def _build(reg, data=0, write=False):
    isw = 1 if write else 0
    tmp = ((isw << 26) | ((reg & 0xFF) << 18) | (data & 0xFFFF))
    b0,b1,b2,b3 = (tmp>>24)&0xFF,(tmp>>16)&0xFF,(tmp>>8)&0xFF,tmp&0xFF
    return tmp | (_bip4(b0&0xF,b1,b2,b3) << 28)

def _parse(frame):
    b0,b1,b2,b3 = (frame>>24)&0xFF,(frame>>16)&0xFF,(frame>>8)&0xFF,frame&0xFF
    ce  = 1 if (_bip4(b0&0xF,b1,b2,b3) != (frame>>28)&0xF) else (frame>>27)&1
    xe  = (frame >> 25) & 1
    data = (frame >> 1) & 0xFFFF
    return ce, xe, data

def _s16(v): return v if v < 0x8000 else v - 0x10000

def reg_read(ser, reg):
    f = _build(reg)
    ser.reset_input_buffer()
    ser.write(bytes([(f>>24)&0xFF,(f>>16)&0xFF,(f>>8)&0xFF,f&0xFF]))
    rx = ser.read(4)
    if len(rx) < 4: raise TimeoutError(f"timeout reg 0x{reg:02X}")
    return _parse((rx[0]<<24)|(rx[1]<<16)|(rx[2]<<8)|rx[3])

def reg_write(ser, reg, data):
    f = _build(reg, data, write=True)
    ser.reset_input_buffer()
    ser.write(bytes([(f>>24)&0xFF,(f>>16)&0xFF,(f>>8)&0xFF,f&0xFF]))
    rx = ser.read(4)
    if len(rx) < 4: raise TimeoutError(f"timeout write reg 0x{reg:02X}")
    return _parse((rx[0]<<24)|(rx[1]<<16)|(rx[2]<<8)|rx[3])

# ---------------------------------------------------------------------------
# Status bit decode  (OIF-ITLA-MSA-01.3 §8)
# ---------------------------------------------------------------------------

STATUSF_BITS = {
    15: "SRQ",
    14: "Latched",
    7:  "FatalTherm",
    6:  "FatalPower",
    5:  "FatalFreq",
    4:  "FatalMod",
    1:  "FatalSoftware",
    0:  "FatalHardware",
}

STATUSW_BITS = {
    15: "SRQ",
    14: "Latched",
    12: "WarnTherm2",
    10: "WarnFreq2",
    8:  "WarnMod",
    7:  "WarnTherm",
    6:  "WarnPower",
    5:  "WarnFreq",
    2:  "WarnBusy",
    0:  "WarnHardware",
}

def decode_status(val, bit_map):
    active = [name for bit, name in sorted(bit_map.items(), reverse=True)
              if val & (1 << bit)]
    return active if active else ["OK"]

# ---------------------------------------------------------------------------
# Dashboard
# ---------------------------------------------------------------------------

def thz_to_nm(thz, ghz10):
    """Convert THz + GHz×10 to wavelength in nm (c = 299792458 m/s)."""
    freq_hz = (thz * 1e12) + (ghz10 * 1e11)
    if freq_hz == 0:
        return 0.0
    return 299792458.0 / freq_hz * 1e9

def read_and_display(ser):
    def rd(reg):
        ce, xe, val = reg_read(ser, reg)
        return None if (ce or xe) else val

    # ── Core readings ──────────────────────────────────────────────────────
    nop     = rd(0x00)
    lf1     = rd(0x40)   # THz
    lf2     = rd(0x41)   # GHz×10
    statusf = rd(0x20)
    statusw = rd(0x21)
    dlstat  = rd(0x15)   # dither/lock status
    channel = rd(0x10)

    # Manufacturer registers
    v1      = rd(0x80)   # Ring-1 raw × 100
    v2      = rd(0x81)   # Ring-2 raw × 100
    v3      = rd(0x82)   # Phase  raw × 100
    gain    = rd(0x83)   # Gain   raw × 100
    soa     = rd(0x84)   # SOA    raw × 100
    temp    = rd(0x85)   # °C × 10 (signed)
    mpd     = rd(0x86)   # Main PD × 10
    wlpd    = rd(0x87)   # Etalon PD × 10
    wmpd    = rd(0x88)   # WM PD × 10
    tec_raw = rd(0x91)   # TEC signed raw

    # ── Compute values ─────────────────────────────────────────────────────
    freq_thz  = (lf1 or 0) + (lf2 or 0) / 10.0 / 1000.0
    wl_nm     = thz_to_nm(lf1 or 0, lf2 or 0)
    chip_temp = _s16(temp) / 10.0  if temp is not None else None
    tec_val   = _s16(tec_raw)      if tec_raw is not None else None
    srq       = bool(nop and (nop & 0x8000))
    locked    = bool(dlstat and (dlstat & 0x0001))

    f_bits = decode_status(statusf or 0, STATUSF_BITS)
    w_bits = decode_status(statusw or 0, STATUSW_BITS)

    # ── Print dashboard ────────────────────────────────────────────────────
    W = 60
    print()
    print("=" * W)
    print("  nanoITLA Laser Status")
    print("=" * W)

    # Lock & frequency
    lock_str = "\033[92mLOCKED\033[0m" if locked else "\033[91mNOT LOCKED\033[0m"
    srq_str  = "  \033[91m[SRQ]\033[0m" if srq else ""
    print(f"  Status   : {lock_str}{srq_str}")
    print(f"  Channel  : {channel if channel is not None else '?'}")
    print(f"  Frequency: {lf1 or 0} THz + {(lf2 or 0)/10.0:.1f} GHz  "
          f"= {freq_thz:.4f} THz")
    print(f"  Wavelength: {wl_nm:.3f} nm" if wl_nm else "  Wavelength: ?")

    print()

    # Temperatures & TEC
    if chip_temp is not None:
        print(f"  Chip Temp: {chip_temp:.1f} °C")
    else:
        print(f"  Chip Temp: ?")
    if tec_val is not None:
        print(f"  TEC raw  : {tec_val}  (setpoint counts)")
    else:
        print(f"  TEC raw  : ?")

    print()

    # Optical power
    mpd_mw  = (mpd  or 0) / 10.0
    wl_mw   = (wlpd or 0) / 10.0
    wm_mw   = (wmpd or 0) / 10.0
    print(f"  MPD  (main PD)   : {mpd_mw:.1f}  (ADC counts)")
    print(f"  WLPD (etalon PD) : {wl_mw:.1f}  (ADC counts)")
    print(f"  WMPD (WM PD)     : {wm_mw:.1f}  (ADC counts)")

    print()

    # Drive voltages (raw ÷ 100)
    def fv(r): return f"{r/100.0:.2f}" if r is not None else "?"
    print(f"  Ring-1 (V1): {fv(v1):>8}    Gain : {fv(gain):>8}")
    print(f"  Ring-2 (V2): {fv(v2):>8}    SOA  : {fv(soa):>8}")
    print(f"  Phase  (V3): {fv(v3):>8}")

    print()

    # Alarms
    fatal_str = ", ".join(f_bits)
    warn_str  = ", ".join(w_bits)
    fatal_col = "\033[92m" if f_bits == ["OK"] else "\033[91m"
    warn_col  = "\033[92m" if w_bits == ["OK"] else "\033[93m"
    print(f"  Fatal  (0x{statusf or 0:04X}): {fatal_col}{fatal_str}\033[0m")
    print(f"  Warning(0x{statusw or 0:04X}): {warn_col}{warn_str}\033[0m")

    print("=" * W)

# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description="nanoITLA laser status dashboard")
    parser.add_argument("--port",    default="/dev/ttyUSB0")
    parser.add_argument("--baud",    type=int, default=115200)
    parser.add_argument("--timeout", type=float, default=0.5)
    parser.add_argument("--loop",    type=float, default=0,
                        help="Refresh interval in seconds (0 = once)")
    args = parser.parse_args()

    ser = serial.Serial(args.port, args.baud, timeout=args.timeout)
    time.sleep(0.1)
    ser.reset_input_buffer()
    print(f"UART {args.port}  {args.baud} baud")

    try:
        if args.loop > 0:
            while True:
                read_and_display(ser)
                time.sleep(args.loop)
        else:
            read_and_display(ser)
    except KeyboardInterrupt:
        print("\nInterrupted.")
    finally:
        ser.close()

if __name__ == "__main__":
    main()
