#!/usr/bin/env python3
"""Frame-by-frame capture from the Pi Pet firmware over USB serial (needs pyserial, Pillow).

  python3 device-recorder.py rec  <out.png> <frames> ["cmd1;cmd2"] [delay_s] [--port P] [--box x,y,size,step]
  python3 device-recorder.py shot <out.png> ["cmd1;cmd2"] [delay_s] [--port P]

`rec` asks the firmware to store <frames> cropped, downsampled frames in PSRAM at its own loop
rate (`rec <n> <x> <y> <size> <step>`), then dumps them (`recdump`) into a contact sheet with
per-frame timestamps. `shot` dumps the full 468x466 RGB565 framebuffer. Commands (e.g.
`mood thinking`, `expr 11`, `look 0 0`, `blink`, `spin`) are sent first. Stop the pi-pet hub
first: it owns the port (`/pet release` in pi frees it for 30 s). Opening the port can reset
the board; the tool waits for it. Pixel data is byte-swapped RGB565 as M5GFX stores it.
"""
import argparse, glob, sys, time
import serial
from PIL import Image, ImageDraw

def open_port(path):
    for _ in range(40):
        try:
            return serial.Serial(path, 115200, timeout=0.2)
        except serial.SerialException:
            time.sleep(0.25)
    sys.exit(f"cannot open {path}")

class Dev:
    def __init__(self, path):
        self.path, self.p = path, open_port(path)
    def read(self, n=4096):
        try:
            return self.p.read(n)
        except serial.SerialException:  # re-enumerated after a reset
            time.sleep(0.5); self.p = open_port(self.path); return b""
    def drain(self, secs):
        end = time.time() + secs
        while time.time() < end: self.read()
    def send(self, line):
        self.p.write((line + "\n").encode())
    def until(self, token, timeout=30):
        buf, t0 = b"", time.time()
        while token not in buf:
            buf += self.read(1)
            if time.time() - t0 > timeout: sys.exit(f"timeout waiting for {token!r}")
        return buf
    def line(self):
        buf = b""
        while not buf.endswith(b"\n"): buf += self.read(1)
        return buf.decode().strip()
    def exact(self, n):
        data = b""
        while len(data) < n: data += self.read(n - len(data))
        return data

def rgb(data, off, count):
    out = []
    for j in range(count):
        c = (data[off + 2 * j] << 8) | data[off + 2 * j + 1]
        out.append((((c >> 11) & 31) * 255 // 31, ((c >> 5) & 63) * 255 // 63, (c & 31) * 255 // 31))
    return out

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("mode", choices=["rec", "shot"])
    ap.add_argument("out")
    ap.add_argument("rest", nargs="*")
    ap.add_argument("--port", default=(glob.glob("/dev/cu.usbmodem*") or [None])[0])
    ap.add_argument("--box", default="84,54,300,2")
    a = ap.parse_args()
    if not a.port: sys.exit("no Stopwatch serial port")
    rest = list(a.rest)
    frames = int(rest.pop(0)) if a.mode == "rec" else 0
    cmds = rest.pop(0) if rest else ""
    delay = float(rest.pop(0)) if rest else 0
    d = Dev(a.port); d.drain(1.0)
    for c in filter(None, cmds.split(";")): d.send(c); time.sleep(0.05)
    time.sleep(delay)
    if a.mode == "shot":
        d.p.reset_input_buffer(); d.send("shot"); d.until(b"GB_SHOT ", 8)
        w, h = map(int, d.line().split())
        data = d.exact(w * h * 2)
        im = Image.new("RGB", (w, h)); im.putdata(rgb(data, 0, w * h)); im.save(a.out)
        print("saved", a.out); return
    x, y, size, step = map(int, a.box.split(","))
    d.send(f"rec {frames} {x} {y} {size} {step}"); d.until(b"GB_REC_DONE", 60)
    d.p.reset_input_buffer(); d.send("recdump"); d.until(b"GB_REC ")
    n, sz = map(int, d.line().split())
    ts = list(map(int, d.line().split()))
    data = d.exact(n * sz * sz * 2)
    cols, tile = 10, 150
    sheet = Image.new("RGB", (cols * tile, ((n + cols - 1) // cols) * tile), "black")
    dr = ImageDraw.Draw(sheet)
    for i in range(n):
        im = Image.new("RGB", (sz, sz)); im.putdata(rgb(data, i * sz * sz * 2, sz * sz))
        sheet.paste(im.resize((tile, tile)), ((i % cols) * tile, (i // cols) * tile))
        dr.text(((i % cols) * tile + 3, (i // cols) * tile + 2), f"{ts[i] - ts[0]}ms", fill=(120, 120, 120))
    sheet.save(a.out)
    print(f"frames {n} span {ts[-1] - ts[0]} ms -> {a.out}")

if __name__ == "__main__":
    main()
