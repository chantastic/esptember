#!/usr/bin/env python3
"""Check an STL for printability: shells, genus, non-manifold edges, size, overhangs.

Usage: mesh_check.py part.stl [--overhang 45]

Reads ASCII or binary STL with the standard library only. Output is meant for
recording as CAD evidence:
  - shells: each edge-connected shell's triangle count, genus and bounding box.
    A single printable part should be one shell. Genus counts through-holes
    (a tunnel is 1; an open groove is 0).
  - non-manifold edges: edges not shared by exactly two triangles. Must be 0.
    OpenSCAD can report "NoError" while the exported STL still has these,
    usually where two coincident surfaces with different facet counts meet.
  - inconsistently wound edges: edges both of whose faces run the same way.
    Must be 0. A mesh can have a positive volume and no bad edges and still
    fail this, for example a polyhedron whose end caps are inverted. OpenSCAD
    then warns "PolySet -> Manifold conversion failed" and booleans with it
    go wrong.
  - signed volume: positive when faces point outward. A negative value
    means a hand-built polyhedron has its winding reversed.
  - near-duplicate vertices: distinct vertices closer than 1e-4 mm. They are
    usually harmless slivers from boolean seams, but a rising count after an
    edit points at coincident surfaces worth rebuilding.
  - overhangs: downward-facing area by angle from vertical, ignoring the bed
    layer, and the worst regions above the threshold.
"""
import collections
import math
import re
import struct
import sys


def load(path):
    data = open(path, "rb").read()
    if data[:5] == b"solid" and b"facet" in data[:400]:
        v = re.findall(rb"vertex\s+(\S+)\s+(\S+)\s+(\S+)", data)
        pts = [tuple(float(c) for c in p) for p in v]
        return [tuple(pts[i:i + 3]) for i in range(0, len(pts), 3)]
    n = struct.unpack("<I", data[80:84])[0]
    tris = []
    for i in range(n):
        f = struct.unpack("<12f", data[84 + i * 50:84 + i * 50 + 48])
        tris.append((f[3:6], f[6:9], f[9:12]))
    return tris


def key(p):
    return tuple(p)  # exact coordinates, as a slicer reads them


def main():
    path = sys.argv[1]
    limit = float(sys.argv[sys.argv.index("--overhang") + 1]) if "--overhang" in sys.argv else 45.0
    tris = load(path)
    edges = collections.defaultdict(list)
    for i, t in enumerate(tris):
        k = [key(p) for p in t]
        for a, b in ((0, 1), (1, 2), (2, 0)):
            edges[frozenset((k[a], k[b]))].append(i)
    bad = [e for e, l in edges.items() if len(l) != 2]

    parent = list(range(len(tris)))

    def find(a):
        while parent[a] != a:
            parent[a] = parent[parent[a]]
            a = parent[a]
        return a

    for l in edges.values():
        for j in l[1:]:
            parent[find(l[0])] = find(j)
    shells = collections.defaultdict(list)
    for i in range(len(tris)):
        shells[find(i)].append(i)

    allp = [p for t in tris for p in t]
    lo = [min(p[k] for p in allp) for k in range(3)]
    hi = [max(p[k] for p in allp) for k in range(3)]
    print(f"{path}: {len(tris)} triangles")
    print("size mm: " + " x ".join(f"{hi[k] - lo[k]:.1f}" for k in range(3)),
          "| bbox " + " ".join(f"{'xyz'[k]}[{lo[k]:.1f},{hi[k]:.1f}]" for k in range(3)))
    for s in shells.values():
        V = {key(p) for i in s for p in tris[i]}
        E = {frozenset((key(tris[i][a]), key(tris[i][b]))) for i in s for a, b in ((0, 1), (1, 2), (2, 0))}
        chi = len(V) - len(E) + len(s)
        print(f"shell: {len(s)} triangles, genus {1 - chi // 2}")
    vol = sum((t[0][0] * (t[1][1] * t[2][2] - t[1][2] * t[2][1])
               - t[0][1] * (t[1][0] * t[2][2] - t[1][2] * t[2][0])
               + t[0][2] * (t[1][0] * t[2][1] - t[1][1] * t[2][0])) / 6 for t in tris)
    print(f"signed volume: {vol:.0f} mm³ ({'faces point outward' if vol > 0 else 'INSIDE OUT: flip the face winding'})")
    print(f"non-manifold edges: {len(bad)}")
    directed = collections.Counter()
    for t in tris:
        k = [key(p) for p in t]
        for a2, b2 in ((0, 1), (1, 2), (2, 0)):
            directed[(k[a2], k[b2])] += 1
    flipped = sum(1 for (u, w), c in directed.items() if c > 1 or directed.get((w, u), 0) != 1)
    print(f"inconsistently wound edges: {flipped}" + ("  (some faces point the wrong way)" if flipped else ""))
    cells = collections.defaultdict(list)
    for v in {key(p) for t in tris for p in t}:
        cells[tuple(round(c, 4) for c in v)].append(v)
    near = sum(len(vs) - 1 for vs in cells.values() if len(vs) > 1)
    print(f"near-duplicate vertices (<1e-4 mm): {near}")
    for e in bad[:10]:
        print("  at", [tuple(round(c, 2) for c in p) for p in e])

    hist = collections.Counter()
    worst = collections.defaultdict(float)
    for t in tris:
        a = [t[1][k] - t[0][k] for k in range(3)]
        b = [t[2][k] - t[0][k] for k in range(3)]
        n = [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]
        area2 = math.sqrt(sum(c * c for c in n))
        if area2 == 0 or min(p[2] for p in t) < lo[2] + 0.05:
            continue
        nz = n[2] / area2
        if nz >= -1e-6:
            continue
        ang = math.degrees(math.asin(min(1.0, -nz)))  # 0 = vertical wall, 90 = flat ceiling
        hist[int(ang // 10) * 10] += area2 / 2
        if ang > limit:
            c = [sum(p[k] for p in t) / 3 for k in range(3)]
            worst[(int(ang // 5) * 5, round(c[0] / 5) * 5, round(c[1] / 5) * 5, round(c[2] / 5) * 5)] += area2 / 2
    print("downward area mm² by angle from vertical: " +
          ", ".join(f"{k}-{k + 10}°: {v:.0f}" for k, v in sorted(hist.items())))
    if worst:
        print(f"regions steeper than {limit:.0f}° (angle, x, y, z, area):")
        for (ang, x, y, z), area in sorted(worst.items(), key=lambda kv: -kv[1])[:8]:
            print(f"  {ang}°+ near ({x}, {y}, {z}): {area:.1f} mm²")


if __name__ == "__main__":
    main()
