#!/usr/bin/env python3
"""Generate the original geometric artwork for ESPtember days 03–06.

Python standard library only for stills/movie; FFmpeg is needed for the GIF.
Run from any directory: python3 scripts/make-media.py
"""
import math
from pathlib import Path
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ORANGE = (255, 91, 4)


def pixels(size, shape, phase=0):
    for y in range(size):
        for x in range(size):
            dx, dy = (x + 0.5 - size / 2) / size, (y + 0.5 - size / 2) / size
            radius = math.hypot(dx, dy)
            if shape == "circle":
                lit = 0.27 < radius < 0.37 or radius < 0.075
            elif shape == "square":
                lit = 0.26 < max(abs(dx), abs(dy)) < 0.36
            elif shape == "triangle":
                lit = -0.35 < dy < 0.30 and abs(dx) < (dy + 0.35) * 0.58
            else:
                angle = phase * 2 * math.pi
                dot_x, dot_y = 0.29 * math.sin(angle), -0.29 * math.cos(angle)
                lit = math.hypot(dx - dot_x, dy - dot_y) < 0.075
                if 0.28 < radius < 0.30 and not lit:
                    yield (48, 54, 61)
                    continue
            yield ORANGE if lit else (0, 0, 0)


def rgb565(colors):
    return b"".join(struct.pack("<H", ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))
                    for r, g, b in colors)


def main_dir(slug):
    path = ROOT / "days" / slug / "firmware" / "main"
    path.mkdir(parents=True, exist_ok=True)
    return path


def main():
    static = main_dir("day-03-static-graphic")
    carousel = main_dir("day-04-carousel")
    for shape in ("circle", "square", "triangle"):
        data = rgb565(pixels(240, shape))
        (carousel / f"{shape}.rgb565").write_bytes(data)
        if shape == "circle":
            (static / "circle.rgb565").write_bytes(data)

    gif_dir = main_dir("day-05-gif")
    with tempfile.TemporaryDirectory() as temp:
        for i in range(20):
            rgb = bytes(channel for color in pixels(160, "orbit", i / 20) for channel in color)
            (Path(temp) / f"{i:02}.ppm").write_bytes(b"P6\n160 160\n255\n" + rgb)
        subprocess.run([
            "ffmpeg", "-hide_banner", "-loglevel", "error", "-y",
            "-framerate", "10", "-i", f"{temp}/%02d.ppm",
            "-filter_complex", "split[a][b];[a]palettegen[p];[b][p]paletteuse",
            "-loop", "0", str(gif_dir / "orbit.gif"),
        ], check=True)

    movie_dir = main_dir("day-06-movie")
    # A raw RGB565 movie, 184 × 224 at 10 fps, 3 seconds long.
    # Letterbox our 184px square with 20 black rows above and below.
    border = bytes(184 * 20 * 2)
    with (movie_dir / "movie.rgb").open("wb") as out:
        for i in range(30):
            out.write(border + rgb565(pixels(184, "orbit", i / 30)) + border)


if __name__ == "__main__":
    main()
