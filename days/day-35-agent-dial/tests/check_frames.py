#!/usr/bin/env python3
"""Check tests/vectors/dial_frame.txt with a second, streaming decoder.

The generator decodes whole buffers. This one is fed in random-sized pieces, the way a serial
port delivers bytes, so a rule that only holds for whole buffers fails here.
"""
import random, sys
from pathlib import Path

def crc16(data):
    crc = 0xFFFF
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = (crc << 1) ^ 0x1021 if crc & 0x8000 else crc << 1
            crc &= 0xFFFF
    return crc

class Decoder:
    def __init__(self):
        self.buf, self.frames, self.discarded, self.corrupt = bytearray(), [], 0, 0
    def drop(self, n=1):
        del self.buf[:n]; self.discarded += n
    def feed(self, data):
        self.buf += data
        while self.buf:
            i = self.buf.find(b"\xa5\x45")
            if i < 0:
                keep = 1 if self.buf[-1] == 0xA5 else 0
                self.drop(len(self.buf) - keep); return
            if i: self.drop(i)
            if len(self.buf) < 6: return
            n = self.buf[4] | self.buf[5] << 8
            if n > 8192: self.drop(); continue
            if len(self.buf) < n + 8: return
            if crc16(self.buf[2:6 + n]) != (self.buf[6 + n] | self.buf[7 + n] << 8):
                self.corrupt += 1; self.drop(); continue
            self.frames.append((self.buf[3], bytes(self.buf[6:6 + n])))
            del self.buf[:n + 8]

def encode(t, p):
    body = bytes([1, t, len(p) & 0xFF, len(p) >> 8]) + p
    c = crc16(body)
    return b"\xa5\x45" + body + bytes([c & 0xFF, c >> 8])

assert crc16(b"123456789") == 0x29B1
path = Path(sys.argv[1] if len(sys.argv) > 1 else Path(__file__).with_name("vectors") / "dial_frame.txt")
rng, checks = random.Random(35), 0
for line in path.read_text().splitlines():
    if not line or line.startswith("#"): continue
    f = line.split("\t")
    if f[0] == "encode":
        assert encode(int(f[2]), bytes.fromhex(f[3])).hex() == f[4], f[1]
        checks += 1
    else:
        data = bytes.fromhex(f[2])
        want = [] if f[3] == "-" else [(int(x.split(":")[0]), bytes.fromhex(x.split(":")[1])) for x in f[3].split(",")]
        for trial in range(20):
            d, i = Decoder(), 0
            while i < len(data):
                n = 1 if trial == 0 else rng.randint(1, 64)
                d.feed(data[i:i + n]); i += n
            assert d.frames == want, f"{f[1]} frames (trial {trial})"
            assert (d.discarded, d.corrupt) == (int(f[4]), int(f[5])), f"{f[1]} counters {d.discarded},{d.corrupt} (trial {trial})"
            checks += 1
print(f"agent-dial frame vectors: {checks} checks passed.")
