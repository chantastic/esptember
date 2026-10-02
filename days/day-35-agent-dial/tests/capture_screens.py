#!/usr/bin/env python3
"""Drive the attached dial through its screens with hub-shaped messages and save framebuffers.

Injected-device evidence: the real firmware renders real widgets from the same messages a hub
sends; nothing here proves touch, pushers, audio, or a live harness. Pause any hub first.

  python3 tests/capture_screens.py OUT_DIR [--port /dev/cu.usbmodemXXXX]
"""
import argparse, glob, json, struct, sys, time, zlib
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from check_device import Link

W, H = 468, 466


def png(path, rgb565):
    rows = bytearray()
    for y in range(H):
        rows.append(0)
        for x in range(W):
            v = struct.unpack_from("<H", rgb565, (y * W + x) * 2)[0]
            rows += bytes(((v >> 11) * 255 // 31, ((v >> 5) & 63) * 255 // 63, (v & 31) * 255 // 31))
    def chunk(t, d):
        return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d))
    Path(path).write_bytes(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", W, H, 8, 2, 0, 0, 0))
                           + chunk(b"IDAT", zlib.compress(bytes(rows), 9)) + chunk(b"IEND", b""))


def shot(link, path):
    time.sleep(0.4)
    link.send({"t": "diag.shot"})
    data = bytearray()
    for t, p in link.frames(20):
        if t == 5:
            data += p
        elif t == 1 and json.loads(p).get("t") == "diag.shot.end":
            break
    if len(data) != W * H * 2:
        sys.exit(f"short framebuffer for {path}: {len(data)} bytes")
    png(path, data)
    print("wrote", path)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--port")
    args = ap.parse_args()
    out = Path(args.out); out.mkdir(parents=True, exist_ok=True)
    port = args.port or (sorted(glob.glob("/dev/cu.usbmodem*")) or [None])[0]
    link = Link(port)
    act = lambda a: (link.send({"t": "diag.act", "act": a}), link.wait("diag.state"))
    link.send({"t": "diag.reset"}); link.wait("diag.state")
    link.send({"t": "welcome", "proto": 1, "app": "agent-dial", "host": {"name": "mac"}})
    shot(link, out / "overview-empty.png")
    pi = {"t": "agent", "id": "pi:esptember", "name": "esptember", "harness": "pi", "caps": ["stop", "prompt"]}
    cc = {"t": "agent", "id": "claude:chan-services", "name": "chan-services", "harness": "claude", "caps": ["approve"]}
    cli = {"t": "agent", "id": "cli:nightly", "name": "nightly-build", "harness": "cli", "caps": ["answer"]}
    link.send({"t": "agents.begin"}); [link.send(m) for m in (pi, cc, cli)]; link.send({"t": "agents.end", "total": 3})
    link.send({"t": "turn.started", "id": "pi:esptember", "text": "$ arduino-cli compile --fqbn esp32:esp32:esp32s3"})
    link.send({"t": "summary", "id": "cli:nightly", "text": "Nightly build passed: 214 tests, 0 failures, firmware 1.93 MB."})
    time.sleep(0.3)
    shot(link, out / "overview.png")
    act("right")
    shot(link, out / "tile-working.png")
    act("right")
    link.send({"t": "summary", "id": "claude:chan-services", "text": "Moved the speech route behind AuthKit device sessions and added tests for expired tokens."})
    shot(link, out / "tile-recap.png")
    link.send({"t": "question", "id": "claude:chan-services", "requestId": "toolu_1",
               "questions": [{"key": "decision", "q": "Allow Bash: $ npm run deploy -- --env production?", "multi": False, "options": ["Allow", "Deny"]}]})
    shot(link, out / "question-approval.png")
    act("back")
    act("swipe_down_top")
    shot(link, out / "drawer.png")
    act("back")
    act("left")
    act("back")
    shot(link, out / "confirm.png")
    act("confirm_no")
    act("enter")
    shot(link, out / "voice-recording.png")
    act("enter")
    shot(link, out / "voice-routing.png")
    link.send({"t": "voice.error", "message": "Set AGENT_DIAL_STT on your computer"})
    shot(link, out / "toast.png")
    act("swipe_up_bottom")
    act("open_settings")
    shot(link, out / "settings.png")
    act("back")
    link.send({"t": "diag.reset"}); link.wait("diag.state")
    act("right")
    shot(link, out / "tile-empty-disconnected.png")
    link.send({"t": "diag.reset"}); link.wait("diag.state")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
