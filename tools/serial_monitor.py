#!/usr/bin/env python3
"""
serial_monitor.py — serial monitor for the Ai-WB2-12F Arduino core.

The SDK's log console runs on UART0 (TX=GPIO16 / RX=GPIO7) at 2 Mbaud, so
that is the default baud here. Prints received bytes to stdout and forwards
keyboard input back to the device (handy once a CLI console exists).

Usage:
    python3 serial_monitor.py [--port /dev/ttyUSB0] [--baud 2000000]

Ctrl-C to exit.
"""
import argparse
import glob
import os
import sys
import time

import serial


def find_port():
    # WSL2 exposes /dev/ttyS0 as a phantom device that can't be opened —
    # only auto-detect real USB-UART bridges (CH340/CP210x/FTDI etc).
    for pattern in ("/dev/ttyUSB*", "/dev/ttyACM*"):
        hits = sorted(glob.glob(pattern))
        if hits:
            return hits[0]
    return None


def main():
    ap = argparse.ArgumentParser(description="Ai-WB2-12F serial monitor")
    ap.add_argument("--port", default=None,
                    help="serial device (default: autodetect /dev/ttyUSB* /dev/ttyACM*)")
    ap.add_argument("--baud", type=int, default=2000000,
                    help="baud rate (SDK console default is 2000000)")
    ap.add_argument("--raw", action="store_true",
                    help="print raw bytes without trying to decode as latin-1")
    args = ap.parse_args()

    port = args.port or find_port()
    if not port:
        sys.exit("No serial port found. Plug in the board "
                 "(WSL2: usbipd attach --wsl <dev> or --auto), then retry.")

    print(f"Opening {port} @ {args.baud} baud  (Ctrl-C to exit)", flush=True)
    try:
        ser = serial.Serial(port, args.baud, timeout=0.05)
        # De-assert DTR/RTS right after open: leaving them asserted would pulse
        # the reset line, rebooting the board (its 2M boot log then shows up as
        # garbage at 115200).
        ser.dtr = False
        ser.rts = False
    except (serial.SerialException, AttributeError) as e:
        sys.exit(f"Cannot open {port}: {e}")

    import select

    try:
        while True:
            n = ser.in_waiting
            if n:
                data = ser.read(n)
                sys.stdout.buffer.write(data)
                sys.stdout.buffer.flush()
            if sys.stdin in select.select([sys.stdin], [], [], 0)[0]:
                line = sys.stdin.readline()
                if line:
                    ser.write(line.encode("latin-1"))
                    ser.flush()
            time.sleep(0.01)
    except KeyboardInterrupt:
        print("\nbye", flush=True)
    finally:
        ser.close()


if __name__ == "__main__":
    main()
