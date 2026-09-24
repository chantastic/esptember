#!/usr/bin/env python3
"""Run 125 seconds of real hardware timing across RUN/WALK boundaries."""
import importlib.util
import json
from pathlib import Path
import sys
import time

spec = importlib.util.spec_from_file_location("check_device", Path(__file__).with_name("check-device.py"))
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
device = module.Device(sys.argv[1])
try:
    initial = device.status()
    device.initial_inputs = initial["inputs"]
    device.command("test begin")
    # Exercise the widest timer first, without leaving a completion in history.
    device.command("prev")
    device.command("prev")
    device.command("enter")
    device.command("advance 300000")
    device.command("prev")
    device.capture("18-long-run")
    device.command("enter")
    device.command("prev")
    device.capture("19-long-paused")
    device.command("escape")
    device.command("escape")
    device.command("enter")
    # Advance to 20 seconds before the first real boundary, then use wall time.
    device.command("advance 280000")
    start_state = device.command("metrics reset")
    started = time.monotonic()
    observations = []
    for sample in range(25):
        time.sleep(5)
        state = device.status()
        elapsed_ms = round((time.monotonic() - started) * 1000)
        timer_ms = state["total_ms"] - start_state["total_ms"]
        assert abs(timer_ms - elapsed_ms) < 350, (elapsed_ms, timer_ms, state)
        device.expect(state, screen="workout", paused=0, partial=0, logs=0)
        observations.append({"host_elapsed_ms": elapsed_ms, "state": state})
        if sample % 5 == 4:
            print(json.dumps({"sample": sample + 1, "segment": state["segment"],
                              "host_elapsed_ms": elapsed_ms, "timer_elapsed_ms": timer_ms,
                              "loop_max_us": state["loop_max_us"]}), flush=True)
    device.expect(state, segment=2, run_ms=60000)
    device.capture("20-real-time-walk")
    (device.output / "soak.json").write_text(json.dumps(observations, indent=2) + "\n")
    device.command("test end")
    device.capture("21-final-home")
    final = device.status()
    device.expect(final, screen="home", logs=initial["logs"], cursor=initial["cursor"],
                  sound=initial["sound"], vibration=initial["vibration"], test=0)
    print(json.dumps({"result": "PASS", "final": final}), flush=True)
finally:
    try:
        if device.status()["test"]:
            device.command("test end")
    finally:
        device.serial.close()
