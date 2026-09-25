#!/usr/bin/env python3
"""Executable reading of SPEC.md's pet roster, used to audit contract.json.

The shared prompt-contract runner only replays scenario expectations. This
model checks that every hand-written expectation is complete and consistent
with the written rules, then fuzzes the rules for invariants.
"""

from __future__ import annotations

import copy
import json
import random
import sys
from pathlib import Path

SLOTS = ("a", "b", "c")
MOODS = ["idle", "thinking", "working", "waiting", "blocked", "done", "surprised", "sleeping",
         "happy", "curious", "excited", "sad"]
# Demo Enter cycles these (sleeping is reached only by hold); after the last it wraps to idle.
DEMO_CYCLE = ["idle", "happy", "curious", "excited", "surprised", "thinking", "working",
              "waiting", "blocked", "done", "sad"]
STYLE_COUNT = 8
NEEDS_YOU = {"done", "error", "waiting"}
WORKING = {"thinking", "tool"}
AGES = ["fresh", "over_4s", "over_30s", "over_10m"]


def defaults(contract: dict) -> dict:
    return {name: field["default"] for name, field in contract["state"].items()}


def pet_mood(s: dict, slot: str) -> str:
    state, age = s[f"{slot}_state"], s[f"{slot}_age"]
    if s[f"{slot}_wince"] and state != "done":
        return "surprised"
    fixed = {"thinking": "thinking", "tool": "working", "error": "blocked", "waiting": "waiting", "sleeping": "sleeping"}
    if state in fixed:
        return fixed[state]
    if state == "done":
        if age == "fresh":
            return "done"
        if s[f"{slot}_unseen"] and age == "over_4s":
            return "waiting"
    return "sleeping" if age == "over_10m" else "idle"


def present(s: dict) -> list[str]:
    return [slot for slot in SLOTS if s[f"{slot}_state"] != "none"]


def step_focus(s: dict, direction: int) -> str:
    pets = present(s)
    if not pets:
        return "none"
    if s["focus"] not in pets:
        return pets[0]
    i = SLOTS.index(s["focus"])
    for k in range(1, len(SLOTS) + 1):
        candidate = SLOTS[(i + direction * k) % len(SLOTS)]
        if candidate in pets:
            return candidate
    return s["focus"]


def set_focus(s: dict, slot: str, by_user: bool) -> None:
    s["focus"] = slot
    if slot == "none":
        return
    if by_user:
        s[f"{slot}_unseen"] = False
    s["emit"] = f"focus {slot}"


def reduce(state: dict, action: dict) -> dict:
    s = copy.deepcopy(state)
    s["emit"] = ""
    kind = action["type"]

    if kind == "report":
        slot, new = action["id"], action["state"]
        fresh = s[f"{slot}_state"] == "none"
        changed = fresh or s[f"{slot}_state"] != new
        s[f"{slot}_state"] = new
        if new == "error":
            s[f"{slot}_wince"] = True
        if changed:
            s[f"{slot}_age"] = "fresh"
            if fresh:
                s[f"{slot}_unseen"] = False
            if new in NEEDS_YOU:
                s[f"{slot}_unseen"] = True
                focused = s["focus"]
                if focused != slot and (focused == "none" or s[f"{focused}_state"] not in WORKING):
                    set_focus(s, slot, by_user=False)
        if s["focus"] == "none":
            set_focus(s, slot, by_user=False)
    elif kind in ("remove", "remove_all"):
        targets = SLOTS if kind == "remove_all" else (action["id"],)
        for slot in targets:
            if s[f"{slot}_state"] == "none":
                continue
            s[f"{slot}_state"], s[f"{slot}_unseen"], s[f"{slot}_wince"], s[f"{slot}_age"] = "none", False, False, "fresh"
        if s["focus"] != "none" and s[f"{s['focus']}_state"] == "none":
            nxt = step_focus(s, 1)
            if nxt == "none":
                s["focus"] = "none"
            else:
                set_focus(s, nxt, by_user=False)
    elif kind.startswith("advance_"):
        order = {"advance_5s": 1, "advance_30s": 2, "advance_10m": 3}[kind]
        for slot in SLOTS:
            if s[f"{slot}_state"] == "none":
                continue
            s[f"{slot}_age"] = AGES[max(AGES.index(s[f"{slot}_age"]), order)]
            s[f"{slot}_wince"] = False  # every advance exceeds the 2.5 s wince
    elif s["focus"] != "none":  # pet mode buttons
        if kind in ("left", "right"):
            s["manual_sleep"] = False
            set_focus(s, step_focus(s, -1 if kind == "left" else 1), by_user=True)
        elif kind == "enter":
            s["manual_sleep"] = False
            s[f"{s['focus']}_unseen"] = False
            s["emit"] = f"ack {s['focus']}"
        elif kind == "hold":
            s["manual_sleep"] = not s["manual_sleep"]
    else:  # demo mode buttons
        if kind in ("left", "right") and s["demo_mood"] == "sleeping":
            s["demo_mood"] = "idle"  # a page press wakes a sleeping demo bot
        if kind == "left":
            s["demo_bot"] = (s["demo_bot"] - 1) % STYLE_COUNT
        elif kind == "right":
            s["demo_bot"] = (s["demo_bot"] + 1) % STYLE_COUNT
        elif kind == "enter":
            m = s["demo_mood"]
            s["demo_mood"] = "idle" if m not in DEMO_CYCLE else DEMO_CYCLE[(DEMO_CYCLE.index(m) + 1) % len(DEMO_CYCLE)]
        elif kind == "hold":
            s["demo_mood"] = "idle" if s["demo_mood"] == "sleeping" else "sleeping"

    s["mode"] = "pets" if s["focus"] != "none" else "demo"
    if s["focus"] == "none":
        s["mood"] = s["demo_mood"]
    else:
        s["mood"] = "sleeping" if s["manual_sleep"] else pet_mood(s, s["focus"])
    return s


def action_object(value):
    return {"type": value} if isinstance(value, str) else dict(value)


def main() -> int:
    path = Path(sys.argv[1] if len(sys.argv) > 1 else Path(__file__).with_name("contract.json"))
    contract = json.loads(path.read_text())
    base = defaults(contract)
    checks = 0
    for scenario in contract["scenarios"]:
        state = base | scenario.get("given", {})
        for index, step in enumerate(scenario["steps"]):
            action = action_object(step["action"])
            model = reduce(state, action)
            claimed = state | step["expect"]
            if model != claimed:
                diff = {k: (claimed[k], model[k]) for k in model if model[k] != claimed[k]}
                print(f"FAIL {scenario['name']} step {index} {action}: claimed vs model {diff}")
                return 1
            state = model
            checks += 1
    # Invariants over long random sequences.
    rng = random.Random(0x17092026)
    actions = [{"type": t} for t in ("left", "right", "enter", "hold", "advance_5s", "advance_30s", "advance_10m", "remove_all")]
    actions += [{"type": "report", "id": i, "state": st} for i in SLOTS
                for st in ("idle", "thinking", "tool", "error", "done", "waiting", "sleeping")]
    actions += [{"type": "remove", "id": i} for i in SLOTS]
    for _ in range(200):
        s = dict(base)
        for _ in range(200):
            s = reduce(s, rng.choice(actions))
            pets = present(s)
            assert (s["focus"] == "none") == (not pets), "focus must exist iff pets exist"
            assert s["focus"] == "none" or s["focus"] in pets, "focus must point at a pet"
            assert s["mood"] in MOODS
            for slot in SLOTS:
                if s[f"{slot}_state"] == "none":
                    assert not s[f"{slot}_unseen"] and not s[f"{slot}_wince"], "empty slot keeps no flags"
            checks += 1
    print(f"pi-pet reference model: {checks} checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
