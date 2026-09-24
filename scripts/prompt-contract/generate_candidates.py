#!/usr/bin/env python3
"""Generate three disposable reducer shapes from one lesson contract."""

from __future__ import annotations

import argparse
import json
from pathlib import Path


def canonical(value) -> str:
    return json.dumps(value, sort_keys=True, separators=(",", ":"))


def build_rules(contract: dict) -> tuple[dict, list[tuple[dict, dict, dict]]]:
    defaults = {name: field["default"] for name, field in contract["state"].items()}
    rules = []
    for scenario in contract["scenarios"]:
        state = defaults | scenario.get("given", {})
        for step in scenario["steps"]:
            action = step["action"] if isinstance(step["action"], dict) else {"type": step["action"]}
            after = state | step["expect"]
            rule = (dict(state), dict(action), dict(after))
            if rule not in rules:
                rules.append(rule)
            state = after
    return defaults, rules


HEADER = '''import copy
import json

def _key(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"))

'''


def list_candidate(defaults, rules):
    return HEADER + f"DEFAULTS = {defaults!r}\nRULES = {rules!r}\n\n" + '''def initial_state():
    return copy.deepcopy(DEFAULTS)

def reduce(state, action):
    for before, expected_action, after in RULES:
        if state == before and action == expected_action:
            return copy.deepcopy(after)
    return copy.deepcopy(state)
'''


def table_candidate(defaults, rules):
    table = {(canonical(before), canonical(action)): after for before, action, after in rules}
    return HEADER + f"DEFAULTS = {defaults!r}\nTABLE = {table!r}\n\n" + '''def initial_state():
    return dict(DEFAULTS)

def reduce(state, action):
    result = TABLE.get((_key(state), _key(action)), state)
    return copy.deepcopy(result)
'''


def function_candidate(defaults, rules):
    body = [HEADER, f"DEFAULTS = {defaults!r}\n\n", "def initial_state():\n    return dict(DEFAULTS)\n\n",
            "def reduce(state, action):\n"]
    for before, action, after in rules:
        body.append(f"    if state == {before!r} and action == {action!r}:\n        return {after!r}\n")
    body.append("    return dict(state)\n")
    return "".join(body)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("contract", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--mutate", action="store_true")
    args = parser.parse_args()
    contract = json.loads(args.contract.read_text())
    defaults, rules = build_rules(contract)
    if args.mutate:
        rules = rules[1:]
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / "a-list.py").write_text(list_candidate(defaults, rules))
    (args.output / "b-table.py").write_text(table_candidate(defaults, rules))
    (args.output / "c-functions.py").write_text(function_candidate(defaults, rules))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
