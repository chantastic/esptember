#!/usr/bin/env python3
"""Generate grok_assets.h: head-shape SDF textures and eye-expression capsules.

Head shapes follow the Grok Bot avatar definitions (x.ai "Designing Grok Bot"):
each is built in a 228.54-unit box centred at 114.27, then normalised to a
228.44 box like the web component. Eyes are the 25 web expressions, each eye a
48-point polygon, fitted here as capsules (centre, axis angle, half length,
half width) so the device can draw and morph them analytically.
"""
import json, math, sys
import numpy as np
from shapely.geometry import Polygon, Point, MultiPoint
from shapely.ops import unary_union
from shapely import vectorized  # noqa: F401  (shapely>=2 provides contains_xy)
import shapely
from svgpathtools import parse_path

C = 114.2705
BLOB = "M228.541 114.228C228.541 130.133 225.184 145.994 218.738 160.534C212.674 174.217 203.904 186.669 193.065 196.988C155.933 232.34 99.497 238.596 55.5255 212.24C45.097 205.99 35.6851 198.072 27.7451 188.866C19.1926 178.953 12.3686 167.569 7.65781 155.351C2.60712 142.264 0 128.257 0 114.228C0 98.3219 3.35751 82.4611 9.80315 67.9215C15.8672 54.2382 24.6377 41.7862 35.4767 31.4668C72.6081 -3.88483 129.044 -10.1413 173.016 16.2153C183.444 22.4653 192.856 30.3829 200.796 39.5896C209.349 49.5018 216.173 60.8859 220.883 73.1037C225.934 86.1906 228.541 100.198 228.541 114.228Z"

def param(f, n=256):
    return Polygon([f(i / n * 2 * math.pi) for i in range(n)])

def rounded_regular(r, sides, corner, rot):
    pts = [(C + math.cos(rot + k / sides * 2 * math.pi) * r, C + math.sin(rot + k / sides * 2 * math.pi) * r) for k in range(sides)]
    return Polygon(pts).buffer(-corner, 64).buffer(corner, 64)

def shapes():
    out = {}
    p = parse_path(BLOB)
    out["blob"] = Polygon([(z.real, z.imag) for z in (p.point(t) for t in np.linspace(0, 1, 400, endpoint=False))])
    def pebble(e):
        t = 108 * (1 + .075 * (.6 * math.sin(2 * e + 1.1) + .4 * math.sin(3 * e - 1.1)))
        return (C + math.cos(e) * t, C + math.sin(e) * t * .98)
    out["pebble"] = param(pebble)
    def sq(e, a=107, b=107, n=3.2):
        c, s = math.cos(e), math.sin(e)
        return (C + math.copysign(abs(c) ** (2 / n), c) * a, C + math.copysign(abs(s) ** (2 / n), s) * b)
    out["squircle"] = param(sq)
    out["tablet"] = unary_union([Point(74.2705, C).buffer(74, 64), Point(154.2705, C).buffer(74, 64),
                                 Polygon([(74.2705, 40.2705), (154.2705, 40.2705), (154.2705, 188.2705), (74.2705, 188.2705)])])
    out["wedge"] = rounded_regular(130, 3, 60, -math.pi / 2)
    out["hex"] = rounded_regular(114, 6, 20, math.pi / 6)
    out["cloud"] = unary_union([Point(x, y).buffer(r, 64) for x, y, r in
                                [(52.2705, 140.2705, 56), (176.2705, 140.2705, 54), (114.2705, 148.2705, 62), (90.2705, 84.2705, 62), (152.2705, 88.2705, 54)]])
    tip = MultiPoint([(C, 0.2705)]).union(Point(C, 140.2705).buffer(88, 128)).convex_hull
    out["teardrop"] = tip.buffer(-18, 64).buffer(18, 64)
    # Normalise like the web component: fit a 228.44 box, scale clamped to [0.9, 1.35].
    for k, poly in out.items():
        x0, y0, x1, y1 = poly.bounds
        s = min(max(228.44 / max(x1 - x0, y1 - y0), .9), 1.35)
        cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
        out[k] = shapely.affinity.affine_transform(poly, [s, 0, 0, s, C - cx * s, C - cy * s])
    return out

N, LO, HI = 128, -20.0, 248.54      # texture covers head units LO..HI on both axes
STEP = (HI - LO) / (N - 1)
QUANT = 4.0                          # int8 = distance * 4 (quarter-unit steps, clamps at ±31.75)

def sdf(poly):
    xs = LO + np.arange(N) * STEP
    X, Y = np.meshgrid(xs, xs)
    pts = shapely.points(X.ravel(), Y.ravel())
    dist = shapely.distance(poly.exterior, pts)
    inside = shapely.contains_xy(poly, X.ravel(), Y.ravel())
    d = np.where(inside, -dist, dist).reshape(N, N)
    return np.clip(np.round(d * QUANT), -127, 127).astype(np.int8)

def capsule(poly):
    P = np.array(poly)
    c = P.mean(axis=0)
    Q = P - c
    w, v = np.linalg.eigh(Q.T @ Q)
    major = v[:, 1]
    if major[1] < 0: major = -major
    minor = np.array([-major[1], major[0]])
    a = np.abs(Q @ major).max(); b = np.abs(Q @ minor).max()
    ang = math.atan2(major[0], major[1])      # device: along = qx*sin + qy*cos
    return [float(c[0]), float(c[1]), ang, max(a - b, 0.0), b]

TA = [1 / 1.45, .8, 1 / 1.12, 1, 1.12, 1.25, 1.45]

def face(poly):
    """Port of the web component's face fit: the roomiest ellipse-ish spot for the eyes."""
    ring = np.array(poly.exterior.coords)[:-1]
    step = max(1, round(len(ring) / 110))
    pts = ring[::step]
    best = {"score": -1, "x": C, "y": C, "a": 1, "b": 1}
    def trial(x, y, r):
        d2 = ((pts[:, 0] - x) ** 2 + ((pts[:, 1] - y) * r) ** 2).min()
        s = math.sqrt(d2); l = s / r
        score = s * l * (1 - .0018 * abs(y - C) - .004 * abs(x - C))
        if score > best["score"]:
            best.update(score=score, x=x, y=y, a=s, b=l)
    for y in np.arange(58.2705, 170.2706, 8):
        for x in np.arange(98.2705, 130.2706, 8):
            for r in TA: trial(x, y, r)
    bx, by = best["x"], best["y"]
    for y in np.arange(by - 8, by + 8.001, 2):
        for x in np.arange(bx - 8, bx + 8.001, 2):
            for r in TA: trial(x, y, r)
    sx = min(max(best["a"] / C, .3), 1); sy = min(max(best["b"] / C, .3), 1)
    eye = min(max((.7 * min(sx, sy) + .3 * max(sx, sy)) * 1.12, .64), 1)
    return [best["x"] - C, best["y"] - C, sx, sy, eye]

def main(eyes_json, out_h):
    heads = shapes()
    order = ["blob", "pebble", "squircle", "tablet", "wedge", "hex", "cloud", "teardrop"]
    eyes = json.load(open(eyes_json))
    lines = ["// Generated by gen_assets.py from the Grok Bot avatar definitions. Do not edit.",
             "#pragma once", "#include <stdint.h>",
             f"static constexpr int kHeadCount = {len(order)};",
             f"static constexpr int kSdfN = {N};",
             f"static constexpr float kSdfLo = {LO}f, kSdfStep = {STEP:.6f}f, kSdfQuant = {QUANT}f;",
             "static const char *const kHeadNames[kHeadCount] = {" + ", ".join(f'"{k}"' for k in order) + "};",
             "static const int8_t kHeadSdf[kHeadCount][kSdfN * kSdfN] = {"]
    for k in order:
        d = sdf(heads[k]).ravel()
        lines.append("  {" + ",".join(str(int(x)) for x in d) + "},")
    lines.append("};")
    lines.append("// Per head: face offset x, y, scale x, y, eye scale (head units, web face fit).")
    lines.append("static const float kHeadFace[kHeadCount][5] = {")
    for k in order:
        f = face(heads[k])
        if k == "wedge": f[0] += 0  # web sets leftDX=-6 for wedge's left eye; applied on device
        lines.append("  {" + ", ".join(f"{x:.3f}f" for x in f) + "},  // " + k)
    lines.append("};")
    lines.append(f"static constexpr int kExprCount = {len(eyes)};")
    lines.append("// Per expression, per eye: centre x, centre y, axis angle, half length, half width (head units).")
    lines.append("static const float kExpr[kExprCount][2][5] = {")
    for e in eyes:
        caps = [capsule(eye) for eye in e]
        lines.append("  {" + ", ".join("{" + ", ".join(f"{x:.3f}f" for x in cp) + "}" for cp in caps) + "},")
    lines.append("};")
    open(out_h, "w").write("\n".join(lines) + "\n")
    print("wrote", out_h)

if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
