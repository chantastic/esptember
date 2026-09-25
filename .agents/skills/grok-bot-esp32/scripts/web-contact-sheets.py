#!/usr/bin/env python3
"""Contact sheets per state from capture-web-demo.mjs output (needs Pillow).

  python3 web-contact-sheets.py <captureDir> <outDir> [every=4] [count=48]

One PNG per state run (one full cycle), cropped around the enlarged avatar, each tile
stamped with milliseconds since that state began.
"""
import json, sys
from pathlib import Path
from PIL import Image, ImageDraw

cap, out = Path(sys.argv[1]), Path(sys.argv[2])
every = int(sys.argv[3]) if len(sys.argv) > 3 else 4
count = int(sys.argv[4]) if len(sys.argv) > 4 else 48
out.mkdir(parents=True, exist_ok=True)
box = json.loads((cap / "box.json").read_text())
rows = [l.split("\t") for l in (cap / "log.tsv").read_text().splitlines()]
x0, y0, size = int(box["x"]) - 40, int(box["y"]) - 40, int(box["width"]) + 80
runs, cur = [], None
for n, t, st in rows:
    if st != cur:
        runs.append([st, []]); cur = st
    runs[-1][1].append((int(n), int(t)))
seen = set()
for st, frames in runs[1:]:  # the first run is partial
    if st in seen or not st:
        continue
    seen.add(st)
    pick = frames[::every][:count]
    tw = 166
    sheet = Image.new("RGB", (tw * 8, tw * ((len(pick) + 7) // 8)), "black")
    d = ImageDraw.Draw(sheet)
    for k, (n, t) in enumerate(pick):
        im = Image.open(cap / f"f{n:05d}.jpg").crop((x0, y0, x0 + size, y0 + size)).resize((tw, tw))
        sheet.paste(im, ((k % 8) * tw, (k // 8) * tw))
        d.text(((k % 8) * tw + 3, (k // 8) * tw + 3), f"{t - frames[0][1]}ms", fill=(120, 120, 120))
    sheet.save(out / f"web-{st}.png")
    print(st, len(frames), "frames")
