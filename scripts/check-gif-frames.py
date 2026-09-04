#!/usr/bin/env python3
"""Check that a firmware GIF contains complete, opaque frames (stdlib only)."""
from collections import Counter
from pathlib import Path
import struct
import sys


def inspect(path):
    with Path(path).open("rb") as stream:
        def read(size):
            data = stream.read(size)
            if len(data) != size:
                raise ValueError("Truncated GIF")
            return data

        def skip_blocks():
            while size := read(1)[0]:
                read(size)

        if read(6) not in (b"GIF87a", b"GIF89a"):
            raise ValueError("Not a GIF")
        width, height, flags, _, _ = struct.unpack("<HHBBB", read(7))
        if flags & 0x80:
            read(3 * (1 << ((flags & 7) + 1)))
        frames = []
        transparent, disposal = False, 0
        while True:
            marker = read(1)[0]
            if marker == 0x3B:
                break
            if marker == 0x21:
                extension = read(1)[0]
                if extension == 0xF9:
                    if read(1)[0] != 4:
                        raise ValueError("Invalid graphic control extension")
                    control = read(4)
                    transparent = bool(control[0] & 1)
                    disposal = (control[0] >> 2) & 7
                    if read(1) != b"\0":
                        raise ValueError("Invalid extension terminator")
                else:
                    skip_blocks()
            elif marker == 0x2C:
                left, top, w, h, packed = struct.unpack("<HHHHB", read(9))
                frames.append({"full": (left, top, w, h) == (0, 0, width, height),
                               "transparent": transparent, "disposal": disposal})
                if packed & 0x80:
                    read(3 * (1 << ((packed & 7) + 1)))
                read(1)  # LZW minimum code size; image data follows in sub-blocks.
                skip_blocks()
                transparent, disposal = False, 0
            else:
                raise ValueError(f"Unexpected GIF marker: {marker:#x}")
        return width, height, frames


if __name__ == "__main__":
    width, height, frames = inspect(sys.argv[1])
    transparent = sum(frame["transparent"] for frame in frames)
    partial = sum(not frame["full"] for frame in frames)
    print(f"{width}x{height}: {len(frames)} frames, {transparent} transparent, "
          f"{partial} partial; disposal={dict(Counter(f['disposal'] for f in frames))}")
    if not frames or transparent or partial:
        sys.exit("Firmware GIF must contain complete, opaque frames")
