#!/usr/bin/env python3
"""Convert the saved internet media for ESPtember days 03–06.

Requires FFmpeg. Run from any directory: python3 scripts/make-media.py
Sources and attribution: assets/media/README.md. No network requests at build time.
"""
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ROOT / "assets" / "media"


def main_dir(slug):
    path = ROOT / "days" / slug / "firmware" / "main"
    path.mkdir(parents=True, exist_ok=True)
    return path


def convert(*args):
    subprocess.run(["ffmpeg", "-hide_banner", "-loglevel", "error", "-y", *map(str, args)], check=True)


def main():
    carousel = main_dir("day-04-carousel")
    for name, source in [("github", "chan-github.jpg"),
                         ("react_conf", "chan-react-conf.jpg"),
                         ("react_advanced", "chan-react-advanced.jpg")]:
        convert("-i", SOURCES / source, "-frames:v", "1",
                "-vf", "scale=240:240:force_original_aspect_ratio=increase,crop=240:240,setsar=1",
                "-c:v", "rawvideo", "-pix_fmt", "rgb565le", "-f", "rawvideo",
                carousel / f"{name}.rgb565")
    shutil.copyfile(carousel / "github.rgb565", main_dir("day-03-static-graphic") / "avatar.rgb565")

    # LVGL 9.5 clears transparent delta pixels instead of retaining the
    # previous frame. Supply complete opaque frames, including unchanged fills.
    # Move the crop window right 26px to shift the picture left by 7% of 368px.
    convert("-i", SOURCES / "homer-bushes-original.gif",
            "-filter_complex", "fps=10,scale=368:448:force_original_aspect_ratio=increase:flags=area,crop=368:448:x=(iw-ow)/2+26:y=(ih-oh)/2,setsar=1,format=rgb24,split[a][b];[a]palettegen=max_colors=128:reserve_transparent=0[p];[b][p]paletteuse=dither=none",
            "-gifflags", "0", "-loop", "0", main_dir("day-05-gif") / "homer.gif")
    subprocess.run([sys.executable, ROOT / "scripts/check-gif-frames.py",
                    main_dir("day-05-gif") / "homer.gif"], check=True)

    # The saved MP4 is the 01:00–01:03 excerpt of Big Buck Bunny, without audio.
    convert("-i", SOURCES / "big-buck-bunny-clip.mp4", "-t", "3", "-an",
            "-vf", "fps=10,scale=368:224:force_original_aspect_ratio=decrease,pad=368:224:(ow-iw)/2:(oh-ih)/2:black,setsar=1",
            "-c:v", "rawvideo", "-pix_fmt", "rgb565le", "-f", "rawvideo",
            main_dir("day-06-movie") / "movie.rgb")


if __name__ == "__main__":
    main()
