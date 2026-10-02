#!/usr/bin/env python3
"""Replay tests/contract.json on an attached Agent Dial over USB (injected-device evidence).

Speaks the fork's frames directly (no hub; pause it first). For each scenario it sends
diag.reset, then diag.act for every step, and compares the device's diag.state with the
contract's cumulative expectation. Standard library only.

  python3 tests/check_device.py [--port /dev/cu.usbmodemXXXX] [--shot out.rgb565]
"""
import argparse, glob, json, os, select, sys, termios, time
from pathlib import Path

HERE = Path(__file__).resolve().parent


def crc16(data):
    crc = 0xFFFF
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


def frame(t, payload):
    body = bytes([1, t]) + len(payload).to_bytes(2, "little") + payload
    return b"\xa5\x45" + body + crc16(body).to_bytes(2, "little")


class Link:
    def __init__(self, path):
        self.fd = os.open(path, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
        attrs = termios.tcgetattr(self.fd)
        attrs[0] = attrs[1] = attrs[3] = 0
        attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
        attrs[4] = attrs[5] = termios.B115200
        termios.tcsetattr(self.fd, termios.TCSANOW, attrs)
        self.buf = bytearray()
        self.logs = []

    def send(self, msg):
        data = frame(1, json.dumps(msg, separators=(",", ":")).encode())
        while data:
            try:
                n = os.write(self.fd, data)
                data = data[n:]
            except BlockingIOError:
                time.sleep(0.005)

    def frames(self, timeout):
        end = time.time() + timeout
        while time.time() < end:
            r, _, _ = select.select([self.fd], [], [], max(0, end - time.time()))
            if r:
                try:
                    self.buf += os.read(self.fd, 65536)
                except BlockingIOError:
                    pass
            while True:
                i = self.buf.find(b"\xa5\x45")
                if i < 0:
                    del self.buf[: max(0, len(self.buf) - 1)]
                    break
                del self.buf[:i]
                if len(self.buf) < 6:
                    break
                n = int.from_bytes(self.buf[4:6], "little")
                if n > 8192:
                    del self.buf[:1]
                    continue
                if len(self.buf) < n + 8:
                    break
                if crc16(self.buf[2:6 + n]) != int.from_bytes(self.buf[6 + n:8 + n], "little"):
                    del self.buf[:1]
                    continue
                t, p = self.buf[3], bytes(self.buf[6:6 + n])
                del self.buf[: n + 8]
                yield t, p

    def wait(self, want, timeout=3.0):
        for t, p in self.frames(timeout):
            if t == 4:
                self.logs.append(p.decode("utf8", "replace"))
            elif t == 1:
                try:
                    m = json.loads(p)
                except ValueError:
                    continue
                if m.get("t") == want:
                    return m
        return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port")
    ap.add_argument("--shot", help="write a diag.shot framebuffer here, then exit")
    args = ap.parse_args()
    port = args.port or (sorted(glob.glob("/dev/cu.usbmodem*")) or [None])[0]
    if not port:
        sys.exit("no /dev/cu.usbmodem* port")
    link = Link(port)
    if args.shot:
        link.send({"t": "diag.shot"})
        data = bytearray()
        for t, p in link.frames(20):
            if t == 5:
                data += p
            elif t == 1 and json.loads(p).get("t") == "diag.shot.end":
                break
        Path(args.shot).write_bytes(data)
        print(f"wrote {len(data)} bytes to {args.shot}")
        return 0
    contract = json.loads((HERE / "contract.json").read_text())
    defaults = {k: v["default"] for k, v in contract["state"].items()}
    checks = failures = 0
    for sc in contract["scenarios"]:
        if sc.get("given"):
            continue
        link.send({"t": "diag.reset"})
        got = link.wait("diag.state")
        if got is None:
            sys.exit("device did not answer diag.reset (is the hub paused?)")
        expect = dict(defaults)
        for k in expect:
            if got.get(k) != expect[k]:
                print(f"FAIL {sc['name']} reset: {k}={got.get(k)!r} expected {expect[k]!r}")
                failures += 1
        for i, st in enumerate(sc["steps"]):
            link.send({"t": "diag.act", "act": st["action"]})
            got = link.wait("diag.state")
            expect |= st["expect"]
            if got is None:
                print(f"FAIL {sc['name']} step {i}: no diag.state")
                failures += 1
                break
            for k, v in expect.items():
                checks += 1
                if got.get(k) != v:
                    print(f"FAIL {sc['name']} step {i} {st['action']}: {k}={got.get(k)!r} expected {v!r}")
                    failures += 1
    print(f"agent-dial device replay: {checks} checks, {failures} failures, {len(contract['scenarios'])} scenarios")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
