#!/usr/bin/env python3
"""Slice a mesh at a height and report every outline, fitting circles to holes.

Usage: section_holes.py model.stl Z [Z ...]

Needs `openscad` on PATH. For each Z it cuts the mesh with
projection(cut = true), exports SVG, and prints each closed loop: point
count, centroid, mean radius and radius spread, and bounding box. Loops whose
radius spread is under 0.15 mm are tagged `circle`. Use this to reverse-
engineer hole patterns (magnets, screws, pegs) from a reference model. To
find flat seats and floors first, look for heights where large horizontal
faces sit; mesh_check.py's bbox and a z histogram are enough.

SVG export flips Y. This script flips it back, so coordinates match the model.
"""
import math
import os
import re
import subprocess
import sys
import tempfile


def main():
    stl = os.path.abspath(sys.argv[1])
    for z in sys.argv[2:]:
        with tempfile.TemporaryDirectory() as tmp:
            scad = os.path.join(tmp, "cut.scad")
            svg = os.path.join(tmp, "cut.svg")
            open(scad, "w").write(f'projection(cut = true) translate([0, 0, -{z}]) import("{stl}");\n')
            subprocess.run(["openscad", "-o", svg, scad], check=True, capture_output=True)
            s = open(svg).read()
        print(f"== z = {z}")
        for d in re.findall(r'd="([^"]+)"', s):
            for sub in re.split(r"(?=M)", d):
                pts = [(float(x), -float(y)) for x, y in re.findall(r"(-?[\d.]+),(-?[\d.]+)", sub)]
                if len(pts) < 3:
                    continue
                cx = sum(p[0] for p in pts) / len(pts)
                cy = sum(p[1] for p in pts) / len(pts)
                rs = [math.hypot(p[0] - cx, p[1] - cy) for p in pts]
                tag = "circle" if max(rs) - min(rs) < 0.15 else ""
                print(f"  n={len(pts):4d} centre=({cx:7.2f},{cy:7.2f}) r={sum(rs) / len(rs):6.2f} "
                      f"[{min(rs):.2f},{max(rs):.2f}] x[{min(p[0] for p in pts):.1f},{max(p[0] for p in pts):.1f}] "
                      f"y[{min(p[1] for p in pts):.1f},{max(p[1] for p in pts):.1f}] {tag}")


if __name__ == "__main__":
    main()
