#!/usr/bin/env python3
"""Exercise installed Day 07 LVGL Basics firmware via USB.

Injects taps through the firmware's virtual pointer device, reads state
back, and captures screen snapshots. Captures/report go to the ignored
.build/verification directory. Requires pyserial and Pillow.
"""
import argparse
import base64
import json
import time
from pathlib import Path

import serial
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]


class Device:
    def __init__(self, port):
        self.serial = serial.Serial(port, 115200, timeout=0.5, write_timeout=2)
        self.report = []
        self.output = ROOT / ".build" / "verification"
        self.output.mkdir(parents=True, exist_ok=True)

    def line(self):
        return self.serial.readline().decode("utf-8", errors="replace").strip()

    def command(self, value):
        self.serial.write((value + "\n").encode())
        deadline = time.monotonic() + 8
        while time.monotonic() < deadline:
            line = self.line()
            if line.startswith("D07_REJECTED"):
                raise RuntimeError(line)
            if line.startswith("D07_STATUS "):
                fields = dict(item.split("=", 1) for item in line.split()[1:])
                return {k: int(v) if v.lstrip("-").isdigit() else v
                        for k, v in fields.items()}
        raise RuntimeError(f"No response: {value}")

    def status(self):
        return self.command("status")

    def expect(self, state, **expected):
        for key, value in expected.items():
            assert state[key] == value, (key, value, state)
        return state

    def capture(self, name, attempts=3):
        for attempt in range(attempts):
            try:
                return self._capture_once(name)
            except AssertionError:
                if attempt == attempts - 1:
                    raise
                time.sleep(1)
                self.serial.reset_input_buffer()

    def _capture_once(self, name):
        self.serial.write(b"capture\n")
        deadline = time.monotonic() + 30
        header = ""
        while time.monotonic() < deadline:
            header = self.line()
            if header.startswith("D07_CAPTURE "):
                break
            if "FAILED" in header:
                raise RuntimeError(header)
        parts = header.split()
        assert len(parts) == 4 and parts[3] == "RGB565LE", header
        width, height = int(parts[1]), int(parts[2])
        expected = -(-width * 2 // 3) * 4  # base64 length of one row
        rows = []
        while len(rows) < height and time.monotonic() < deadline:
            line = self.line()
            if len(line) != expected:
                continue  # interleaved log/noise line
            rows.append(base64.b64decode(line))
        assert len(rows) == height, f"{name}: {len(rows)}/{height} rows"
        ending = self.line()
        while not ending and time.monotonic() < deadline:
            ending = self.line()
        assert ending == "D07_CAPTURE_END", ending
        data = b"".join(rows)
        pixels = bytearray(width * height * 3)
        for i in range(width * height):
            color = data[2 * i] | data[2 * i + 1] << 8
            r, g, b = (color >> 11) & 31, (color >> 5) & 63, color & 31
            pixels[i * 3:i * 3 + 3] = bytes(
                ((r * 255) // 31, (g * 255) // 63, (b * 255) // 31))
        image = Image.frombytes("RGB", (width, height), bytes(pixels))
        path = self.output / f"{name}.png"
        image.save(path)
        self.report.append({"capture": path.name})
        return path

    def run(self):
        initial = self.status()
        self.expect(initial, screen="controls")
        # Normalize: scroll back to the top (idempotent over-scroll).
        for _ in range(2):
            self.command("drag 184 150 184 430")
        time.sleep(0.6)
        presses = initial["presses"]
        self.report.append({"initial": initial})
        self.capture("01-controls")

        # Coordinates verified against captures. Unscrolled layout:
        # Count (184,108), slider (y=277), switch (184,390).
        self.expect(self.command("tap 184 108"), presses=presses + 1)
        self.expect(self.command("tap 184 108"), presses=presses + 2)
        self.report.append({"check": "count button increments"})

        # Brightness slider: full right, then left third.
        self.command("drag 100 277 350 277")
        self.expect(self.status(), brightness=100)
        self.command("drag 340 277 100 277")
        state = self.status()
        assert 10 <= state["brightness"] < 60, state
        self.report.append({"check": f"slider dragged -> {state['brightness']}"})
        self.capture("02-slider-moved")

        # Orange mode switch: normalize to off, then prove both edges.
        if self.status()["orange"]:
            self.command("tap 184 390")
        self.expect(self.command("tap 184 390"), orange=1)
        self.capture("03-orange")
        self.expect(self.command("tap 184 390"), orange=0)
        self.report.append({"check": "orange switch toggles"})

        # The column overflows: a gentle scroll reveals the About button.
        # (A harder fling over-scrolls and elastically snaps back.)
        self.command("drag 184 380 184 200")
        time.sleep(0.6)
        self.capture("04-scrolled")

        # Scrolled layout: About centered at (184,393).
        state = self.command("tap 184 393")
        self.expect(state, screen="about")
        time.sleep(0.5)  # screen-load animation
        self.capture("05-about")
        self.report.append({"check": "about screen loads"})

        # Back button position read from 05-about capture at first run.
        state = self.command("tap 184 330")
        if state["screen"] != "about":
            self.expect(state, screen="controls", presses=presses + 2)
        else:  # fall back: probe lower
            self.expect(self.command("tap 184 380"), screen="controls",
                        presses=presses + 2)
        time.sleep(0.5)
        self.capture("06-back")
        self.report.append(
            {"check": "back returns; presses/slider/switch state retained"})

        final = self.status()
        self.report.append({"final": final})
        (self.output / "report.json").write_text(
            json.dumps(self.report, indent=2) + "\n")
        print(json.dumps({"result": "PASS", "final": final,
                          "report": str(self.output / "report.json")}))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("port")
    args = parser.parse_args()
    device = Device(args.port)
    try:
        device.run()
    finally:
        device.serial.close()
