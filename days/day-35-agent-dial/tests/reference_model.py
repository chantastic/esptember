#!/usr/bin/env python3
"""Executable reading of SPEC.md's Behavior section, used to audit contract.json.

The shared prompt-contract runner only replays scenario expectations. This model checks that
every scenario expectation is complete and agrees with the written rules, then fuzzes the rules
for invariants.
"""

from __future__ import annotations

import copy
import json
import random
import sys
from pathlib import Path

SLOTS = ("a", "b", "c")
SCREENS = ["overview", "agent", "question", "drawer", "confirm", "voice", "settings"]
PAGING_SCREENS = {"overview", "agent"}


def defaults(contract: dict) -> dict:
    return {name: field["default"] for name, field in contract["state"].items()}


def present(s: dict) -> list[str]:
    return [slot for slot in SLOTS if s[f"{slot}_state"] != "none"]


def step(s: dict, direction: int) -> str:
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


def land(s: dict, slot: str) -> None:
    """The user arrives on a tile: focus is reported and its finished-turn row clears."""
    s["focus"] = slot
    s["screen"] = "agent"
    if slot == "none":
        return
    s["emit"] = f"focus {slot}"
    if s[f"{slot}_unread"] == "done":
        s[f"{slot}_unread"] = "none"


def clear_slot(s: dict, slot: str) -> None:
    s[f"{slot}_state"] = "none"
    s[f"{slot}_asking"] = False
    s[f"{slot}_unread"] = "none"
    s[f"{slot}_stop"] = False
    s[f"{slot}_prompt"] = False


def beep(s: dict) -> None:
    if s["voice"] != "recording":
        s["sound"] = "beep"


def reduce(state: dict, action: dict) -> dict:
    s = copy.deepcopy(state)
    s["emit"] = ""
    s["sound"] = "none"
    s["toast"] = ""
    kind = action["type"]
    user = kind in ("left", "right", "enter", "back", "tap", "confirm_yes", "confirm_no", "answer",
                    "swipe_down_top", "swipe_up_bottom", "open_row", "open_settings")

    if user and not s["awake"]:
        s["awake"] = True                               # the waking input is swallowed
    elif kind == "welcome":
        s["connected"] = True
    elif kind == "silence_15s":
        s["connected"] = False
        for slot in SLOTS:
            clear_slot(s, slot)
        s["focus"] = "none"
        s["screen"] = "overview"
        s["voice"], s["voice_target"] = "off", "none"
    elif kind == "agent":
        slot = action["id"]
        if s[f"{slot}_state"] == "none":
            s[f"{slot}_state"] = "idle"
        s[f"{slot}_stop"] = bool(action["stop"])
        s[f"{slot}_prompt"] = bool(action["prompt"])
        if s["focus"] == "none":
            s["focus"] = slot                           # programmatic: no echo
    elif kind == "gone":
        slot = action["id"]
        if s[f"{slot}_state"] != "none":
            was_focus = s["focus"] == slot
            nxt = step(s, 1) if was_focus else s["focus"]   # the next session in slot order
            clear_slot(s, slot)
            if s["voice_target"] == slot:
                s["voice"], s["voice_target"] = "off", "none"
                if s["screen"] == "voice":
                    s["screen"] = "agent"
            if was_focus:
                s["focus"] = "none" if nxt == slot else nxt
                if s["screen"] in ("question", "confirm"):
                    s["screen"] = "agent"
    elif kind in ("turn_started", "turn_done", "turn_error", "summary", "question", "question_close",
                  "transcript"):
        slot = action["id"]
        if s[f"{slot}_state"] == "none":
            pass                                        # reports for unknown sessions are ignored
        elif kind == "turn_started":
            s[f"{slot}_state"] = "working"
            if s["voice"] == "routing" and s["voice_target"] == slot:
                s["voice"], s["voice_target"] = "off", "none"
                s["focus"], s["screen"] = slot, "agent"
        elif kind == "turn_done":
            s[f"{slot}_state"] = "idle"
        elif kind == "turn_error":
            s[f"{slot}_state"] = "idle"
            s["toast"] = "turn error"
        elif kind == "summary":
            s[f"{slot}_state"] = "idle"
            beep(s)
            viewing = s["awake"] and s["screen"] == "agent" and s["focus"] == slot
            if not action.get("quiet") and not viewing and s[f"{slot}_unread"] != "question":
                s[f"{slot}_unread"] = "done"
            if not s["awake"]:
                s["awake"] = True
                s["screen"] = "drawer"
        elif kind == "question":
            s[f"{slot}_asking"] = True
            s[f"{slot}_unread"] = "question"
            s["awake"] = True
            beep(s)
            s["emit"] = f"agent.open {slot}"
            if s["screen"] != "question" and s["voice"] == "off":
                s["focus"], s["screen"] = slot, "question"
        elif kind == "question_close":
            if s[f"{slot}_asking"]:
                s[f"{slot}_asking"] = False
                if s[f"{slot}_unread"] == "question":
                    s[f"{slot}_unread"] = "none"
                if s["screen"] == "question" and s["focus"] == slot:
                    s["screen"] = "agent"
        elif kind == "transcript":
            if s["voice"] == "routing":
                s["voice_target"] = slot
                s["focus"] = slot
    elif kind == "voice_error":
        if s["voice"] != "off":
            s["voice"], s["voice_target"] = "off", "none"
            s["screen"] = "agent" if s["focus"] != "none" else "overview"
            s["toast"] = "voice error"
    elif kind == "advance_60s":
        if s["voice"] == "routing":
            s["voice"], s["voice_target"] = "off", "none"
            s["screen"] = "agent" if s["focus"] != "none" else "overview"
    elif kind == "idle_5m":
        busy_tile = s["screen"] == "agent" and s["focus"] != "none" and s[f"{s['focus']}_state"] == "working"
        if s["voice"] == "off" and not busy_tile:
            s["awake"] = False
    # ---- user actions on an awake dial ----
    elif kind in ("left", "right"):
        if s["screen"] == "agent" and s["focus"] != "none":
            land(s, step(s, -1 if kind == "left" else 1))
        elif s["screen"] == "overview" and s["focus"] != "none":
            land(s, s["focus"])
    elif kind == "swipe_up_bottom":
        if s["screen"] == "agent":
            s["screen"] = "overview"
        elif s["screen"] == "overview" and s["focus"] != "none":
            land(s, s["focus"])
    elif kind == "enter" or (kind == "tap" and s["voice"] == "recording"):
        if s["voice"] == "recording":
            s["voice"] = "routing"
            s["emit"] = "voice.end"
        elif kind == "tap":
            pass
        elif s["screen"] == "agent" and s["focus"] != "none":
            f = s["focus"]
            if not s["connected"]:
                s["toast"] = "Not connected"
            elif not s[f"{f}_prompt"]:
                s["toast"] = "Can't send to this harness"
            elif not s[f"{f}_asking"]:
                s["voice"], s["voice_target"], s["screen"] = "recording", f, "voice"
                s["emit"] = f"voice.begin {f}"
        elif s["screen"] == "overview":
            if not s["connected"]:
                s["toast"] = "Not connected"
            elif present(s):
                s["voice"], s["voice_target"], s["screen"] = "recording", "routed", "voice"
                s["emit"] = "voice.begin"
    elif kind == "back":
        scr = s["screen"]
        if s["voice"] == "recording":
            s["voice"], s["voice_target"] = "off", "none"
            s["emit"] = "voice.abort"
            s["screen"] = "agent" if s["focus"] != "none" else "overview"
        elif scr == "question":
            s["screen"] = "agent"
        elif scr == "confirm":
            s["screen"] = "agent"
        elif scr in ("drawer", "settings"):
            s["screen"] = "agent" if scr == "drawer" and s["focus"] != "none" else "overview"
        elif scr == "agent" and s["focus"] != "none" and s[f"{s['focus']}_state"] == "working":
            if s[f"{s['focus']}_stop"]:
                s["screen"] = "confirm"
            else:
                s["toast"] = "Can't stop this from here"
    elif kind == "confirm_yes":
        if s["screen"] == "confirm":
            f = s["focus"]
            s["emit"] = f"turn.stop {f}"
            s[f"{f}_state"] = "idle"
            s["screen"] = "agent"
    elif kind == "confirm_no":
        if s["screen"] == "confirm":
            s["screen"] = "agent"
    elif kind == "answer":
        if s["screen"] == "question":
            f = s["focus"]
            s["emit"] = f"answer {f}"
            s[f"{f}_asking"] = False
            if s[f"{f}_unread"] == "question":
                s[f"{f}_unread"] = "none"
            s["screen"] = "agent"
    elif kind == "swipe_down_top":
        if s["screen"] in PAGING_SCREENS:
            s["screen"] = "drawer"
    elif kind == "open_row":
        slot = action["id"]
        if s["screen"] == "drawer" and s[f"{slot}_unread"] != "none":
            s["emit"] = f"agent.open {slot}"
            s["focus"] = slot
            if s[f"{slot}_asking"]:
                s["screen"] = "question"
            else:
                s[f"{slot}_unread"] = "none"
                s["screen"] = "agent"
    elif kind == "open_settings":
        if s["screen"] == "overview":
            s["screen"] = "settings"
    elif kind == "tap":
        pass

    s["badge"] = sum(1 for slot in SLOTS if s[f"{slot}_unread"] != "none")
    return s


def action_object(value):
    return {"type": value} if isinstance(value, str) else dict(value)


def random_actions() -> list[dict]:
    acts = [{"type": t} for t in ("welcome", "silence_15s", "voice_error", "advance_60s", "idle_5m",
                                  "left", "right", "enter", "back", "tap", "confirm_yes", "confirm_no",
                                  "answer", "swipe_down_top", "swipe_up_bottom", "open_settings")]
    for i in SLOTS:
        acts += [{"type": "agent", "id": i, "stop": st, "prompt": pr} for st in (False, True) for pr in (False, True)]
        acts += [{"type": t, "id": i} for t in ("gone", "turn_started", "turn_done", "turn_error", "question",
                                                "question_close", "transcript", "open_row")]
        acts += [{"type": "summary", "id": i, "quiet": q} for q in (False, True)]
    return acts


def main() -> int:
    path = Path(sys.argv[1] if len(sys.argv) > 1 else Path(__file__).with_name("contract.json"))
    contract = json.loads(path.read_text())
    base = defaults(contract)
    checks = 0
    for scenario in contract["scenarios"]:
        state = base | scenario.get("given", {})
        for index, st in enumerate(scenario["steps"]):
            action = action_object(st["action"])
            model = reduce(state, action)
            claimed = state | st["expect"]
            if model != claimed:
                diff = {k: (claimed[k], model[k]) for k in model if model[k] != claimed[k]}
                print(f"FAIL {scenario['name']} step {index} {action}: claimed vs model {diff}")
                return 1
            state = model
            checks += 1
    rng = random.Random(0x26092026)
    actions = random_actions()
    for _ in range(200):
        s = dict(base)
        for _ in range(200):
            s = reduce(s, rng.choice(actions))
            live = present(s)
            assert (s["focus"] == "none") == (not live), "focus exists iff a session exists"
            assert s["focus"] == "none" or s["focus"] in live, "focus points at a session"
            assert s["screen"] in SCREENS
            assert (s["voice"] == "off") == (s["voice_target"] == "none"), "voice and its target agree"
            assert s["voice"] != "recording" or s["sound"] == "none", "no sound while the mic runs"
            assert s["screen"] != "question" or (s["focus"] != "none" and s[f"{s['focus']}_asking"]), \
                "a question screen belongs to the focused asking session"
            assert s["screen"] != "confirm" or s["focus"] != "none", "confirm needs a session"
            assert (s["screen"] == "voice") == (s["voice"] != "off"), "the voice overlay is up exactly while voice runs"
            assert s["badge"] == sum(1 for x in SLOTS if s[f"{x}_unread"] != "none")
            for slot in SLOTS:
                if s[f"{slot}_state"] == "none":
                    assert not s[f"{slot}_asking"] and s[f"{slot}_unread"] == "none", "empty slot keeps no flags"
                assert s[f"{slot}_asking"] == (s[f"{slot}_unread"] == "question"), "a question row exists exactly while asking"
            checks += 1
    print(f"agent-dial reference model: {checks} checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
