#!/usr/bin/env python3
# SPDX-License-Identifier: CC-BY-NC-SA-4.0
"""Uploads tools/sdcard/mons/*.bin to the card in the Fake Pod over USB, with the same PUT protocol as upstream's tools/send_sd.py (the firmware only ever writes under /mons/,
and never formats). Differences: the port is a by-id path, the DTR/RTS lines are not toggled on open (that would reset the chip), it waits for the firmware to answer
before it starts, and a file already on the card with the same size is skipped, so an interrupted run can simply be started again.
  python3 tools/send_sd_fp.py --port /dev/serial/by-id/usb-Espressif_...-if00 [--ls]"""
import argparse, glob, os, re, sys, time
import serial

def open_port(path):
    s = serial.Serial(); s.port = path; s.baudrate = 115200; s.timeout = 1; s.write_timeout = 5; s.dtr = False; s.rts = False
    s.open(); time.sleep(5)          # opening the port resets the chip: give the firmware time to boot (a write before that blocks)
    return s

def lines_until(ser, end, timeout):
    t0 = time.time(); out = []
    while time.time() - t0 < timeout:
        l = ser.readline().decode(errors='replace').strip()
        if l == end: return out, True
        if l == 'ERR': return out, False
        if l: out.append(l)
    return out, False

def card_listing(ser):
    ser.reset_input_buffer()
    try: ser.write(b'LS\n')
    except serial.SerialTimeoutException: return {}, False
    out, ok = lines_until(ser, 'DONE', 8)
    sizes = {}
    for l in out:
        m = re.fullmatch(r'([\w.\-]+\.bin) (\d+)', l)       # the firmware's `name size` lines; boot-log lines never look like this
        if m: sizes[m.group(1)] = int(m.group(2))
    return sizes, ok

def main():
    ap = argparse.ArgumentParser(); ap.add_argument('--port', required=True); ap.add_argument('--ls', action='store_true'); a = ap.parse_args()
    ser = open_port(a.port)
    for _ in range(20):                       # wait until the firmware is running and answers
        sizes, ok = card_listing(ser)
        if ok: break
        time.sleep(0.5)
    else: sys.exit('the board does not answer LS (is TamaPoke running and the card mounted?)')
    print(f'card has {len(sizes)} files in /mons')
    if a.ls: return
    files = sorted(glob.glob(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'sdcard', 'mons', '*.bin')))
    todo = [f for f in files if sizes.get(os.path.basename(f)) != os.path.getsize(f)]
    print(f'{len(files)} files here, {len(todo)} to send')
    t_all = time.time(); sent = 0
    for i, path in enumerate(todo, 1):
        size = os.path.getsize(path); name = os.path.basename(path)
        for attempt in range(3):
            ser.reset_input_buffer(); ser.write(f'PUT mons/{name} {size}\n'.encode())
            _, ok = lines_until(ser, 'OK', 5)
            if not ok: time.sleep(0.5); continue
            good = True
            with open(path, 'rb') as f:
                while chunk := f.read(2048):
                    ser.write(chunk)
                    ack = ''; t_ack = time.time()
                    while ack not in ('#', 'ERR') and time.time() - t_ack < 10:   # a slow card block (cluster allocation) can take a few seconds
                        ack = ser.readline().decode(errors='replace').strip()
                    if ack != '#': good = False; break
            _, done = lines_until(ser, 'DONE', 30) if good else ([], False)
            if done: sent += size; break
            print(f'  retry {name}')
            time.sleep(7)                  # the firmware gives up on a stalled PUT after 5 s: let it, so the next command is not read as data
            ser.reset_input_buffer()
        else: sys.exit(f'FAILED on {name}')
        if i % 10 == 0 or i == len(todo):
            print(f'  {i}/{len(todo)}  {sent/1024/1024:.1f} MB  {sent/1024/max(1,time.time()-t_all):.0f} KB/s', flush=True)
    sizes, ok = card_listing(ser)
    bad = [os.path.basename(f) for f in files if sizes.get(os.path.basename(f)) != os.path.getsize(f)]
    print('verify:', 'all', len(files), 'files present with the right size' if not bad else f'MISSING/WRONG: {bad[:10]}')
    sys.exit(1 if bad else 0)

if __name__ == '__main__': main()
