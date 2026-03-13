#!/usr/bin/env python3
"""
Enable / disable nano-ITLA laser output via ResEna (0x32).

Usage:
    sudo python3 enable_laser.py [--port /dev/ttyUSB0] [--off]

ResEna bit definitions (OIF-ITLA-MSA §9.4.4):
    bit 3  SENA  – Software enable (1 = laser output ON)

Default action: set ResEna = 0x0008  (laser ON)
With --off:     set ResEna = 0x0000  (laser OFF)
"""

import argparse
import struct
import sys
import time
import serial


# ---------------------------------------------------------------------------
# ITLA wire protocol (same BIP4 framing as register_scan.py)
# ---------------------------------------------------------------------------

def _bip4(b0_lo4, b1, b2, b3):
    bip8 = (b0_lo4 & 0x0F) ^ b1 ^ b2 ^ b3
    return ((bip8 >> 4) & 0x0F) ^ (bip8 & 0x0F)


def build_frame(is_write, reg, data, lstrsp=0):
    app = ((is_write & 1) << 26) | ((reg & 0xFF) << 18) | (data & 0xFFFF)
    tmp = ((lstrsp & 1) << 27) | (app & 0x07FFFFFF)
    b0 = (tmp >> 24) & 0xFF
    b1 = (tmp >> 16) & 0xFF
    b2 = (tmp >> 8) & 0xFF
    b3 = tmp & 0xFF
    csum = _bip4(b0 & 0x0F, b1, b2, b3) & 0x0F
    return struct.pack('>I', tmp | (csum << 28))


def parse_response(raw4):
    frame = struct.unpack('>I', raw4)[0]
    csum = (frame >> 28) & 0x0F
    b0 = (frame >> 24) & 0xFF
    b1 = (frame >> 16) & 0xFF
    b2 = (frame >> 8) & 0xFF
    b3 = frame & 0xFF
    calc = _bip4(b0 & 0x0F, b1, b2, b3) & 0x0F
    ce = 1 if calc != csum else 0
    xe = (frame >> 25) & 1
    reg = (frame >> 17) & 0xFF
    data = (frame >> 1) & 0xFFFF
    return ce, xe, reg, data


def transact(ser, is_write, reg, data):
    ser.write(build_frame(is_write, reg, data))
    raw = ser.read(4)
    if len(raw) != 4:
        raise IOError(f'Short read on reg 0x{reg:02X}: got {len(raw)} bytes')
    return parse_response(raw)


def read_reg(ser, reg):
    return transact(ser, 0, reg, 0)


def write_reg(ser, reg, data):
    return transact(ser, 1, reg, data)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

RESENA_REG = 0x32
SENA_BIT   = 0x0008    # bit 3 = software enable


def main():
    ap = argparse.ArgumentParser(description='Enable/disable nano-ITLA laser output')
    ap.add_argument('--port', default='/dev/ttyUSB0', help='Serial port')
    ap.add_argument('--off',  action='store_true',    help='Turn laser OUTPUT off (ResEna=0x0000)')
    args = ap.parse_args()

    target_val = 0x0000 if args.off else SENA_BIT
    action_str = 'OFF (0x0000)' if args.off else 'ON  (0x0008, SENA set)'

    with serial.Serial(args.port, 115200, timeout=1.0) as ser:
        time.sleep(0.1)
        ser.reset_input_buffer()

        # Read current ResEna
        ce, xe, _, cur = read_reg(ser, RESENA_REG)
        if ce or xe:
            print(f'ERROR reading ResEna (0x{RESENA_REG:02X}): CE={ce} XE={xe}')
            sys.exit(1)
        print(f'ResEna current value : 0x{cur:04X}')

        # Write new value
        ce, xe, _, _ = write_reg(ser, RESENA_REG, target_val)
        if ce or xe:
            print(f'ERROR writing ResEna: CE={ce} XE={xe}')
            sys.exit(1)

        time.sleep(0.05)

        # Readback verify
        ce, xe, _, new = read_reg(ser, RESENA_REG)
        if ce or xe:
            print(f'ERROR reading back ResEna: CE={ce} XE={xe}')
            sys.exit(1)

        if new == target_val:
            print(f'ResEna set to       : 0x{new:04X}  →  laser output {action_str}  ✓')
        else:
            print(f'MISMATCH: wrote 0x{target_val:04X} but read back 0x{new:04X}')
            sys.exit(1)

        # Read MPD / WLPD / WMPD from manufacturer window to confirm PD activity
        time.sleep(0.5)     # allow photocurrent to settle
        print()
        print('Live PD readings after enable (manufacturer window):')
        for reg, label in [(0x8B, 'MPD  (main PD)'),
                           (0x8A, 'WLPD (wavelength locker PD)'),
                           (0x89, 'WMPD (wavelength monitor PD)')]:
            ce2, xe2, _, val = read_reg(ser, reg)
            if xe2:
                print(f'  0x{reg:02X}  {label:30s}  XE (not available)')
            else:
                print(f'  0x{reg:02X}  {label:30s}  0x{val:04X}  ({val} ADC counts)')


if __name__ == '__main__':
    main()
