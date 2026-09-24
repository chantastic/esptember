#!/usr/bin/env python3
"""Exercise installed Day 08 LVGL Basics firmware via USB.

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
            if line.startswith("D08_REJECTED"):
                raise RuntimeError(line)
            if line.startswith("D08_STATUS "):
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
            if header.startswith("D08_CAPTURE "):
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
        assert ending == "D08_CAPTURE_END", ending
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

    def check_buttons(self):
        self.expect(self.command("reset"), focus="count", editing=0)
        self.expect(self.command("button both"), presses=1, focus="count")
        self.expect(self.command("button right"), focus="brightness", editing=0)
        self.expect(self.command("button both"), focus="brightness", editing=1)
        self.expect(self.command("button left"), brightness=90, focus="brightness")
        self.capture("07-button-edit")
        for _ in range(9):
            self.command("button left")
        self.expect(self.status(), brightness=10, editing=1)
        self.expect(self.command("button right"), brightness=20, editing=1)
        # Holding both exits editing. Release must not re-enter it.
        self.expect(self.command("button hold"), brightness=20, editing=0,
                    focus="brightness", presses=1)
        self.expect(self.command("button left"), focus="count")
        self.expect(self.command("button left"), focus="about")
        self.expect(self.command("button both"), screen="about")
        self.expect(self.command("button hold"), screen="controls", focus="about",
                    presses=1, brightness=20)
        self.expect(self.command("button right"), focus="count")
        self.expect(self.command("button hold"), focus="count", presses=1)
        self.expect(self.command("button right"), focus="brightness")
        self.expect(self.command("button right"), focus="orange")
        self.expect(self.command("button both"), orange=1, focus="orange")
        self.capture("08-button-switch")
        self.expect(self.command("button both"), orange=0, focus="orange")
        self.expect(self.command("button left"), focus="brightness")
        self.expect(self.command("button both"), editing=1)
        self.expect(self.command("button right"), brightness=30)
        self.expect(self.command("button both"), editing=0, focus="brightness")
        self.expect(self.command("button right"), focus="orange")
        self.expect(self.command("button right"), focus="about")
        self.expect(self.command("button both"), screen="about")
        self.expect(self.command("button both"), screen="controls", focus="about")
        self.report.append({"check": "C25K button grammar: left/right direction, wrap, "
                            "select, slider editing and bounds, switch, and back; "
                            "chords do not leak clicks or select on hold release"})

    def run(self):
        self.check_buttons()
        # Reset only this demo's RAM state; it never writes NVS or device history.
        initial = self.command("reset")
        self.expect(initial, board="m5stack-stopwatch", screen="controls",
                    presses=0, brightness=100, orange=0, touch=1)
        assert initial["map_version"] in (1, 2), initial
        active_map = (initial["map_version"], initial["map_generation"])
        time.sleep(0.4)
        self.report.append({"initial": initial})
        self.capture("01-controls")

        # Positions checked against the actual 468 x 466 display captures.
        self.expect(self.command("tap 233 127"), presses=1)
        self.expect(self.command("tap 233 127"), presses=2)
        self.report.append({"check": "count button increments through hit-testing"})

        self.command("drag 360 249 103 249")
        self.expect(self.status(), brightness=10)
        self.capture("02-dim")
        self.command("drag 103 249 363 249")
        self.expect(self.status(), brightness=100)
        self.command("drag 363 249 220 249")
        state = self.status()
        assert 30 <= state["brightness"] <= 60, state
        saved_brightness = state["brightness"]
        self.report.append({"check": "brightness floor, ceiling, and intermediate value"})

        self.expect(self.command("tap 345 294"), orange=1)
        self.capture("03-orange")
        self.expect(self.command("tap 345 294"), orange=0)
        self.expect(self.command("tap 345 294"), orange=1)
        self.report.append({"check": "orange switch toggles both ways"})

        self.expect(self.command("tap 233 355"), screen="about")
        self.capture("04-about")
        self.expect(self.command("tap 233 362"), screen="controls", presses=2,
                    brightness=saved_brightness, orange=1)
        self.capture("05-back")
        self.report.append({"check": "all three values survive screen transitions"})

        # Repeated animations catch lifetime/allocation regressions, not just
        # a successful first trip. Check memory after warming up both screens.
        baseline = self.status()
        for _ in range(20):
            self.expect(self.command("tap 233 355"), screen="about")
            self.expect(self.command("tap 233 362"), screen="controls", presses=2,
                        brightness=saved_brightness, orange=1)
        stable = self.status()
        assert stable["uptime"] > baseline["uptime"], stable
        assert stable["flushes"] > baseline["flushes"], stable
        assert stable["heap"] >= baseline["heap"] - 2048, (baseline, stable)
        assert stable["psram"] >= baseline["psram"] - 2048, (baseline, stable)
        self.report.append({"check": "20 round trips without reset or growing memory use",
                            "before": baseline, "after": stable})

        # Leave the unit ready for the physical-touch check.
        self.command("reset")
        time.sleep(0.3)
        self.capture("06-ready")
        final = self.status()
        assert (final["map_version"], final["map_generation"]) == active_map, final
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
