#!/usr/bin/env python3
"""Injected-device evidence: replay contract scenarios over USB serial.

Drives the real firmware through the SPEC's host protocol (`pet`, `unpet`) and
semantic `act` inputs, then compares the device's `pets` report and emitted
GB_FOCUS / GB_ACK lines with each step's expectation. Scenarios that need a
`given` state or 30 s / 10 min clocks are skipped (host contract only).

Stop the pi-pet hub first; it owns the port. Usage:
  python3 check_device.py [/dev/cu.usbmodemXXXX]
"""

from __future__ import annotations

import glob
import json
import sys
import time
from pathlib import Path

import serial  # pyserial

IDS = {"a": "aaaa0001", "b": "bbbb0002", "c": "cccc0003"}
SLOT_BY_ID = {v: k for k, v in IDS.items()}
STYLE = {"a": 2, "b": 4, "c": 5}
LONG = {"advance_30s", "advance_10m"}


def open_port(path: str) -> serial.Serial:
    for _ in range(40):
        try:
            return serial.Serial(path, 115200, timeout=0.05)
        except serial.SerialException:
            time.sleep(0.25)
    raise SystemExit(f"cannot open {path}")


class Device:
    def __init__(self, path: str):
        self.path = path
        self.port = open_port(path)
        self.buf = b""

    def lines(self, seconds: float) -> list[str]:
        end, out = time.time() + seconds, []
        while time.time() < end:
            try:
                self.buf += self.port.read(4096)
            except serial.SerialException:  # opening the port can reset the board
                time.sleep(0.5)
                self.port = open_port(self.path)
            while b"\n" in self.buf:
                line, self.buf = self.buf.split(b"\n", 1)
                out.append(line.decode(errors="replace").strip())
        return out

    def send(self, line: str) -> None:
        self.port.write((line + "\n").encode())

    def report(self) -> dict:
        self.send("pets")
        for _ in range(20):
            for line in self.lines(0.1):
                if line.startswith("GB_PETS "):
                    return parse_pets(line)
        raise AssertionError("no GB_PETS reply")


def parse_pets(line: str) -> dict:
    parts = line.split()[1:]
    info = dict(p.split("=", 1) for p in parts if "=" in p)
    pets = {}
    for p in parts:
        if p.count(":") == 2:
            pid, state, unseen = p.split(":")
            pets[SLOT_BY_ID.get(pid, pid)] = (state, unseen == "1")
    return {"focus": SLOT_BY_ID.get(info["focus"], "none"), "mood": info["mood"],
            "manual_sleep": info["manual_sleep"] == "1", "demo_bot": int(info["demo_bot"]), "pets": pets}


def command(action) -> tuple[str | None, float]:
    if isinstance(action, str):
        if action == "advance_5s":
            return None, 5.3
        if action == "remove_all":
            return "unpet *", 0.3
        return f"act {action}", 0.3
    slot = action["id"]
    if action["type"] == "remove":
        return f"unpet {IDS[slot]}", 0.3
    return f"pet {IDS[slot]} {action['state']} {STYLE[slot]} pet-{slot}|", 0.3


def main() -> int:
    port = sys.argv[1] if len(sys.argv) > 1 else (glob.glob("/dev/cu.usbmodem*") or [None])[0]
    if not port:
        raise SystemExit("no Stopwatch serial port found")
    contract = json.loads(Path(__file__).with_name("contract.json").read_text())
    dev = Device(port)
    dev.lines(4.0)  # boot banner after a possible reset
    ran = skipped = checks = 0
    for scenario in contract["scenarios"]:
        if scenario.get("given") or any(isinstance(s["action"], str) and s["action"] in LONG for s in scenario["steps"]):
            skipped += 1
            continue
        dev.send("reset")
        dev.lines(0.4)
        expected = {k: f["default"] for k, f in contract["state"].items()}
        for index, step in enumerate(scenario["steps"]):
            line, wait = command(step["action"])
            if line:
                dev.send(line)
            window = dev.lines(wait)
            emitted = [l for l in window if l.startswith(("GB_FOCUS ", "GB_ACK "))]
            sounds = [l.split()[1] for l in window if l.startswith("GB_SOUND ")]
            expected |= step["expect"]
            got = dev.report()
            where = f"{scenario['name']} step {index} {step['action']}"
            emit = ""
            if emitted:
                verb, pid = emitted[-1].split()
                emit = f"{'focus' if verb == 'GB_FOCUS' else 'ack'} {SLOT_BY_ID.get(pid, pid)}"
            pairs = [("focus", got["focus"], expected["focus"]), ("mood", got["mood"], expected["mood"]),
                     ("manual_sleep", got["manual_sleep"], expected["manual_sleep"]),
                     ("emit", emit, expected["emit"]),
                     ("sound", sounds[-1] if sounds else "none", expected["sound"])]
            if expected["mode"] == "demo":
                pairs.append(("demo_bot", got["demo_bot"], expected["demo_bot"]))
            for slot in IDS:
                state = expected[f"{slot}_state"]
                pairs.append((f"{slot}_state", got["pets"].get(slot, ("none", False))[0], state))
                if state != "none":
                    pairs.append((f"{slot}_unseen", got["pets"][slot][1], expected[f"{slot}_unseen"]))
            for name, actual, want in pairs:
                checks += 1
                if actual != want:
                    print(f"FAIL {where}: {name}={actual!r}, expected {want!r}")
                    return 1
        ran += 1
        print(f"ok  {scenario['name']}")
    dev.send("reset")
    print(f"pi-pet injected-device: {ran} scenarios, {checks} checks passed, {skipped} skipped (host-only clocks/given).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
