#!/usr/bin/env python3
"""Exercise installed C25K via USB. Accelerated sessions live only in RAM.

Requires pyserial and Pillow. Captures/report go to ignored .build/verification.
Production persistence check runs only with empty progress, then clears its own
synthetic entry through Reset Progress and restores sound/vibration settings.
"""
import argparse
import json
from pathlib import Path
import time

import serial
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]


class Device:
    def __init__(self, port):
        self.port = port
        self.serial = serial.Serial(port, 115200, timeout=0.5, write_timeout=2)
        self.report = []
        self.output = ROOT / ".build" / "verification"
        self.output.mkdir(parents=True, exist_ok=True)

    def line(self):
        return self.serial.readline().decode("utf-8", errors="replace").strip()

    def status(self):
        self.serial.write(b"status\n")
        deadline = time.monotonic() + 8
        while time.monotonic() < deadline:
            line = self.line()
            if line.startswith("C25K_STATUS "):
                fields = dict(item.split("=", 1) for item in line.split()[1:])
                return {k: int(v) if v.isdigit() else v for k, v in fields.items()}
        raise RuntimeError("No C25K status received")

    def command(self, value):
        self.serial.write((value + "\n").encode())
        deadline = time.monotonic() + 8
        while time.monotonic() < deadline:
            line = self.line()
            if line == "C25K_REJECTED":
                raise RuntimeError(f"Rejected command: {value}")
            if line.startswith("C25K_STATUS "):
                fields = dict(item.split("=", 1) for item in line.split()[1:])
                return {k: int(v) if v.isdigit() else v for k, v in fields.items()}
        raise RuntimeError(f"No command response: {value}")

    def expect(self, state, **expected):
        for key, value in expected.items():
            assert state[key] == value, (key, value, state)
        assert state["inputs"] == self.initial_inputs, "Physical input invalidates test"
        return state

    def capture(self, name):
        self.serial.write(b"capture\n")
        deadline = time.monotonic() + 12
        header = ""
        while time.monotonic() < deadline:
            header = self.line()
            if header.startswith("C25K_CAPTURE "):
                break
            if "REJECTED" in header:
                raise RuntimeError(header)
        parts = header.split()
        assert len(parts) == 4 and parts[3] == "RGB565LE", header
        width, height = int(parts[1]), int(parts[2])
        assert (width, height) == (468, 468)
        remaining = width * height * 2
        data = bytearray()
        while remaining and time.monotonic() < deadline:
            chunk = self.serial.read(remaining)
            data.extend(chunk)
            remaining -= len(chunk)
        assert remaining == 0, f"Truncated capture {name}: {remaining} bytes missing"
        ending = self.line()
        if not ending:
            ending = self.line()
        assert ending == "C25K_CAPTURE_END", ending
        pixels = bytearray(width * height * 3)
        for i in range(width * height):
            color = data[2 * i] | data[2 * i + 1] << 8
            r, g, b = (color >> 11) & 31, (color >> 5) & 63, color & 31
            pixels[i * 3:i * 3 + 3] = bytes(((r * 255) // 31, (g * 255) // 63, (b * 255) // 31))
        image = Image.frombytes("RGB", (width, height), bytes(pixels))
        # Match the panel aperture while retaining a rectangular artifact.
        mask = Image.new("L", (width, height), 0)
        ImageDraw.Draw(mask).ellipse((2, 2, width - 3, height - 3), fill=255)
        image = Image.composite(image, Image.new("RGB", image.size, "#151719"), mask)
        path = self.output / f"{name}.png"
        image.save(path)
        self.report.append({"capture": path.name})
        return path

    def reboot(self):
        self.serial.write(b"reboot\n")
        self.serial.close()
        # USB may disappear briefly; reconnect without changing modem lines.
        deadline = time.monotonic() + 15
        while time.monotonic() < deadline:
            time.sleep(0.6)
            try:
                self.serial = serial.Serial(self.port, 115200, timeout=0.5, write_timeout=2)
                state = self.status()
                self.initial_inputs = state["inputs"]
                return state
            except (OSError, serial.SerialException, RuntimeError):
                if self.serial.is_open:
                    self.serial.close()
        raise RuntimeError("C25K did not return after reboot")

    def run(self):
        initial = self.status()
        self.initial_inputs = initial["inputs"]
        self.expect(initial, screen="home", width=468, height=468, psram=8388608, storage=1, speaker=1)
        self.report.append({"initial": initial})
        self.command(f"time {int(time.time())}")
        rtc = self.status()["rtc"]
        assert abs(rtc - time.time()) < 4, (rtc, time.time())
        self.capture("01-home")
        try:
            self.expect(self.command("test begin"), test=1, logs=0, cursor=0)
            self.expect(self.command("prev"), page=27)
            self.expect(self.command("enter"), screen="history")
            self.capture("02-history-empty")
            self.command("escape")
            self.expect(self.command("escape"), screen="settings")
            self.capture("03-settings-sound")
            self.expect(self.command("enter"), sound=0)
            self.command("next")
            self.expect(self.command("enter"), vibration=0)
            self.command("next")
            self.expect(self.command("enter"), screen="set_time")
            self.command("next")
            self.capture("04-set-time")
            self.command("enter")
            self.command("prev")
            self.command("enter")
            self.capture("05-time-confirm")
            self.expect(self.command("escape"), screen="settings")
            self.command("next")
            self.expect(self.command("enter"), screen="reset")
            self.capture("06-reset-confirm")
            self.command("escape")
            self.command("next")
            self.capture("07-about")
            self.command("escape")
            self.expect(self.command("enter"), screen="workout", workout=0, segment=0)
            self.capture("08-warmup")
            self.command("advance 290000")
            state = self.command("advance 10000")
            self.expect(state, segment=1, partial=0)
            self.capture("09-run")
            paused = self.expect(self.command("enter"), paused=1)
            after = self.command("advance 10000")
            assert after["total_ms"] == paused["total_ms"]
            assert after["remaining"] == paused["remaining"]
            self.capture("10-paused")
            self.expect(self.command("next"), segment=2, paused=1, partial=1)
            self.expect(self.command("prev"), segment=2, paused=1)
            self.expect(self.command("prev"), segment=1, paused=1)
            self.command("enter")
            before = self.expect(self.command("escape"), screen="cancel")
            self.capture("11-cancel-overlay")
            state = self.command("advance 1000")
            assert state["total_ms"] >= before["total_ms"] + 1000
            self.expect(self.command("next"), screen="workout", segment=1)
            self.command("escape")
            self.command("advance 6000")
            self.expect(self.status(), screen="workout")
            self.command("escape")
            self.expect(self.command("escape"), screen="home", logs=0, cursor=0)
            self.report.append({"check": "pause, paused navigation, track-back, running overlay, timeout, cancel"})

            # Complete all 27 through the actual installed transition engine.
            totals = [1800]*3 + [1860]*3 + [1680]*3 + [1890]*3 + [1860,1860,1800,2040,1980,2100] + [2100]*3 + [2280]*3 + [2400]*3
            runs = [480]*3 + [540]*3 + [540]*3 + [960]*3 + [900,960,1200,1080,1200,1500] + [1500]*3 + [1680]*3 + [1800]*3
            for index in range(27):
                self.expect(self.command("enter"), screen="workout", workout=index)
                state = self.command(f"advance {totals[index] * 1000}")
                self.expect(state, screen="complete", logs=index+1, cursor=min(index+1,26), partial=0,
                            total_ms=totals[index]*1000, run_ms=runs[index]*1000)
                if index == 0:
                    self.capture("12-complete")
                self.command("enter")
            self.capture("13-program-complete")
            self.report.append({"check": "27 natural completions", "totals_sec": totals, "run_sec": runs})

            self.command("next")  # history landing
            self.expect(self.command("enter"), screen="history", logs=27)
            self.capture("14-history")
            self.command("next")
            self.command("prev")
            self.command("escape")
            # Repeating an earlier workout moves the cursor backwards.
            self.command("next")
            self.command("next")  # home0 via wrap
            self.command("enter")
            for _ in range(18):
                state = self.command("next")
            self.expect(state, screen="complete", partial=1, logs=28, cursor=1)
            self.capture("15-partial-complete")
            self.command("enter")
            self.command("prev")
            self.capture("16-repeat-counts")
            self.report.append({"check": "partial on final Next, backward cursor from repeat, stacked history"})
        finally:
            state = self.status()
            if state["test"]:
                self.command("test end")

        # Verify actual NVS writes and startup loading only on this fresh build.
        if initial["logs"] == 0 and initial["cursor"] == 0:
            self.command("escape")
            if initial["sound"]:
                self.command("enter")
            self.command("next")
            if initial["vibration"]:
                self.command("enter")
            self.command("escape")
            self.command("enter")
            for _ in range(18):
                state = self.command("next")
            self.expect(state, screen="complete", logs=1, cursor=1, partial=1, storage=1)
            state = self.reboot()
            self.expect(state, screen="home", logs=1, cursor=1, sound=0, vibration=0)
            self.command("escape")
            for _ in range(3):
                self.command("next")
            self.command("enter")
            self.expect(self.command("enter"), screen="settings", logs=0, cursor=0)
            for _ in range(3):
                self.command("prev")
            if initial["sound"]:
                self.command("enter")
            self.command("next")
            if initial["vibration"]:
                self.command("enter")
            self.command("escape")
            state = self.reboot()
            self.expect(state, screen="home", logs=0, cursor=0,
                        sound=initial["sound"], vibration=initial["vibration"], storage=1)
            self.report.append({"check": "real NVS completion/settings survive reboot; synthetic log cleared; settings restored"})
        self.capture("17-ready-home")
        final = self.status()
        self.report.append({"final": final})
        (self.output / "report.json").write_text(json.dumps(self.report, indent=2) + "\n")
        print(json.dumps({"result": "PASS", "final": final, "report": str(self.output / "report.json")}))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("port")
    args = parser.parse_args()
    device = Device(args.port)
    try:
        device.run()
    finally:
        device.serial.close()
