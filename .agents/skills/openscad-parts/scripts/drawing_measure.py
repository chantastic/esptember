#!/usr/bin/env python3
"""Measure outline widths in a raster drawing, such as a manufacturer's size PDF page.

Usage: drawing_measure.py page.png X0 Y0 X1 Y1 [--step 8] [--mm-per-px 0.0437]

Scans each row of the box (full-resolution pixel coordinates) and prints the
leftmost and rightmost dark pixel and the width, in pixels and optionally in
mm. Use it to find tapers, steps and feature spans that a drawing shows but
doesn't dimension. Calibrate --mm-per-px from a dimensioned feature in the
same view, such as a diameter.

It needs no image libraries. It converts the PNG to an uncompressed BMP
with macOS `sips` and reads the pixels with the standard library.
Measure at full resolution: downscaled previews lose the 10–20 px
differences that matter.
"""
import os
import struct
import subprocess
import sys
import tempfile


def main():
    args = sys.argv[1:]
    step = int(args[args.index("--step") + 1]) if "--step" in args else 8
    scale = float(args[args.index("--mm-per-px") + 1]) if "--mm-per-px" in args else None
    png, x0, y0, x1, y1 = args[0], *map(int, args[1:5])
    with tempfile.TemporaryDirectory() as tmp:
        bmp = os.path.join(tmp, "page.bmp")
        subprocess.run(["sips", "-s", "format", "bmp", png, "--out", bmp], check=True, capture_output=True)
        d = open(bmp, "rb").read()
    off = struct.unpack("<I", d[10:14])[0]
    w, h = struct.unpack("<ii", d[18:26])
    bpp = struct.unpack("<H", d[28:30])[0]
    row = ((bpp * w + 31) // 32) * 4

    def dark(x, y):
        yy = y if h < 0 else h - 1 - y   # negative height = top-down rows
        o = off + yy * row + x * (bpp // 8)
        return sum(d[o:o + 3]) < 3 * 128

    print(f"{png}: {w} x {abs(h)} px, box x[{x0},{x1}] y[{y0},{y1}]")
    for y in range(y0, y1, step):
        xs = [x for x in range(x0, x1) if dark(x, y)]
        if not xs:
            continue
        width = xs[-1] - xs[0]
        mm = f"  {width * scale:.2f} mm" if scale else ""
        print(f"  y={y:5d}  x[{xs[0]},{xs[-1]}]  width {width} px{mm}")


if __name__ == "__main__":
    main()
