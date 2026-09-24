#!/usr/bin/env python3
"""Validate a disposable lesson core against a prompt-first contract."""

from __future__ import annotations

import argparse
import copy
import importlib.util
import json
import math
import random
from pathlib import Path


class Checks:
    def __init__(self) -> None:
        self.count = 0

    def that(self, condition: bool, message: str) -> None:
        self.count += 1
        if not condition:
            raise AssertionError(message)


def load_candidate(path: Path):
    spec = importlib.util.spec_from_file_location("lesson_candidate", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Cannot load candidate: {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def action_object(value):
    return {"type": value} if isinstance(value, str) else copy.deepcopy(value)


def full_state(contract: dict, partial: dict | None = None) -> dict:
    state = {name: field["default"] for name, field in contract["state"].items()}
    if partial:
        state.update(copy.deepcopy(partial))
    return state


def validate_state(checks: Checks, contract: dict, state: dict) -> None:
    checks.that(isinstance(state, dict), "state must be a dictionary")
    checks.that(set(state) == set(contract["state"]), "state fields differ from contract")
    for name, field in contract["state"].items():
        value = state[name]
        kind = field["type"]
        if kind == "enum":
            checks.that(value in field["values"], f"{name} has invalid enum value {value!r}")
        elif kind == "int":
            checks.that(isinstance(value, int) and not isinstance(value, bool), f"{name} must be int")
            checks.that(field["min"] <= value <= field["max"], f"{name} outside range")
        elif kind == "bool":
            checks.that(isinstance(value, bool), f"{name} must be bool")
        elif kind == "string":
            checks.that(isinstance(value, str), f"{name} must be string")
            checks.that(len(value) <= field["max_length"], f"{name} is too long")
        else:
            checks.that(False, f"unknown field type {kind}")


def check_metadata(checks: Checks, contract: dict) -> None:
    checks.that(contract["schema"] == 1, "unsupported contract schema")
    checks.that(contract["lesson"].startswith("day-"), "lesson slug is required")
    hardware = contract["hardware"]
    checks.that(hardware["board"] == "m5stack-stopwatch", "wrong board")
    checks.that(hardware["width"] == 468 and hardware["height"] == 466, "wrong panel geometry")
    checks.that(hardware["offset_x"] == 6 and hardware["offset_y"] == 0, "wrong panel offsets")
    checks.that(hardware["rotation"] == 0, "wrong rotation")
    checks.that(hardware["touch_map"] == "espt-touch/record", "shared touch map missing")

    controls = contract["controls"]
    checks.that(controls["left_button"] == "BtnA", "BtnA must be physical left")
    checks.that(controls["right_button"] == "BtnB", "BtnB must be physical right")
    checks.that(controls["profile"] in ("c25k", "localized"), "unknown control profile")
    if controls["profile"] == "c25k":
        checks.that(controls["left"] == "previous_or_decrease", "wrong left action")
        checks.that(controls["right"] == "next_or_increase", "wrong right action")
        checks.that(controls["enter"] == "short_both", "wrong Enter gesture")
        checks.that(controls["back"] == "hold_both_600ms", "wrong Back gesture")
    else:
        checks.that(bool(controls.get("rationale")), "localized controls need a rationale")

    visual = contract["visual"]
    checks.that(visual["scenario"] in {s["name"] for s in contract["scenarios"]},
                "visual scenario must be tested")
    checks.that(visual["evidence"] in ("reference", "device"), "visual evidence label required")


def check_layout(checks: Checks, contract: dict) -> None:
    center_x, center_y = 234.0, 233.0
    safe_radius = contract["layout"].get("safe_radius", 226)
    inset = contract["layout"].get("focus_inset", 3)
    for control in contract["layout"]["controls"]:
        x, y, width, height = control["bounds"]
        checks.that(width > 0 and height > 0, f"{control['id']} has empty bounds")
        checks.that(x >= 0 and y >= 0 and x + width <= 468 and y + height <= 466,
                    f"{control['id']} leaves the drawable panel")
        corners = ((x + inset, y + inset), (x + width - inset, y + inset),
                   (x + inset, y + height - inset),
                   (x + width - inset, y + height - inset))
        for point_x, point_y in corners:
            checks.that(math.hypot(point_x - center_x, point_y - center_y) <= safe_radius,
                        f"{control['id']} or its focus ring leaves the round safe area")


def check_candidate(checks: Checks, contract: dict, candidate) -> None:
    initial = candidate.initial_state()
    checks.that(initial == full_state(contract), "candidate defaults differ from contract")
    validate_state(checks, contract, initial)

    actions = []
    for scenario in contract["scenarios"]:
        state = full_state(contract, scenario.get("given"))
        validate_state(checks, contract, state)
        for index, step in enumerate(scenario["steps"]):
            action = action_object(step["action"])
            actions.append(action)
            before = copy.deepcopy(state)
            first = candidate.reduce(copy.deepcopy(state), copy.deepcopy(action))
            second = candidate.reduce(copy.deepcopy(state), copy.deepcopy(action))
            checks.that(first == second, f"{scenario['name']} step {index} is nondeterministic")
            checks.that(state == before, f"{scenario['name']} step {index} mutated its input")
            validate_state(checks, contract, first)
            for key, expected in step["expect"].items():
                checks.that(first[key] == expected,
                            f"{scenario['name']} step {index}: {key}={first[key]!r}, expected {expected!r}")
            state = first

    randomizer = random.Random(0x09232026)
    for _ in range(100):
        state = candidate.initial_state()
        for _ in range(100):
            action = copy.deepcopy(randomizer.choice(actions))
            state = candidate.reduce(state, action)
            validate_state(checks, contract, state)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("contract", type=Path)
    parser.add_argument("candidate", type=Path)
    args = parser.parse_args()
    contract = json.loads(args.contract.read_text())
    candidate = load_candidate(args.candidate)
    checks = Checks()
    check_metadata(checks, contract)
    validate_state(checks, contract, full_state(contract))
    check_layout(checks, contract)
    check_candidate(checks, contract, candidate)
    print(f"{contract['lesson']} contract: {checks.count} assertions passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
