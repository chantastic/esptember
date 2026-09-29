#!/usr/bin/env python3
"""Convert a 3MF (or merge its objects) to a binary STL that OpenSCAD can import.

Usage: mesh_convert.py model.3mf out.stl

OpenSCAD imports 3MF only in some builds; STL always works. This reads every
<object> mesh in 3D/3dmodel.model and applies no build transforms, which is
correct for single-body exports from Shapr3D, Fusion and Bambu Studio. Print
the object list first, and handle any <component> or transform by hand.
"""
import re
import struct
import sys
import zipfile


def main():
    src, dst = sys.argv[1], sys.argv[2]
    model = zipfile.ZipFile(src).read("3D/3dmodel.model").decode()
    if "<components>" in model or 'transform="' in model:
        print("warning: components or transforms present; positions may be wrong", file=sys.stderr)
    tris = []
    for oid, name, vs, ts in re.findall(
            r'<object id="(\d+)"[^>]*?name="([^"]*)".*?<vertices>(.*?)</vertices>\s*<triangles>(.*?)</triangles>',
            model, re.S):
        V = [tuple(map(float, m)) for m in re.findall(r'x="([^"]+)" y="([^"]+)" z="([^"]+)"', vs)]
        T = [tuple(map(int, m)) for m in re.findall(r'v1="(\d+)" v2="(\d+)" v3="(\d+)"', ts)]
        lo = [min(v[k] for v in V) for k in range(3)]
        hi = [max(v[k] for v in V) for k in range(3)]
        print(f"object {oid} {name!r}: {len(T)} triangles, bbox " +
              " ".join(f"{'xyz'[k]}[{lo[k]:.1f},{hi[k]:.1f}]" for k in range(3)))
        tris += [(V[a], V[b], V[c]) for a, b, c in T]
    with open(dst, "wb") as out:
        out.write(b"\0" * 80 + struct.pack("<I", len(tris)))
        for t in tris:
            out.write(struct.pack("<12fH", 0, 0, 0, *t[0], *t[1], *t[2], 0))
    print(f"wrote {dst}")


if __name__ == "__main__":
    main()
