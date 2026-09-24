---
board: m5stack-stopwatch
day: 30
title: Trivia
toolchain: Arduino ESP32 3.3.10 + M5Unified 0.2.19 + M5GFX 0.2.26 + LVGL 9.3.0
summary: "A multi-Stopwatch quiz with host election, timestamped answers, dedupe, and visible verdicts."
verification: "Prompt contract and reference frame verified; generated hardware candidate awaits hand review"
---

## The assignment

Run a fair true/false quiz using identical M5Stack Stopwatches as host and buzzers.

The durable source is this prompt, [SPEC.md](https://github.com/chantastic/esptember/blob/main/days/day-30-trivia/SPEC.md), the machine-readable contract, and its layered acceptance criteria.
Generated firmware is a disposable candidate.

## Reference frame

![Round-screen acceptance reference for Trivia](https://esptember.com/images/day-30-trivia/reference.png)

This is a deterministic screenshot of the visual acceptance model.
It defines the intended hierarchy and safe-area use; only a later framebuffer capture from the attached Stopwatch can count as device rendering evidence.

## The build prompt

```text
Build ESPtember Day 30: Trivia for the M5Stack Stopwatch Dev Kit (C152).

Read these before writing code:
- .agents/skills/build-stopwatch-lessons/SKILL.md
- .agents/skills/prompt-first-embedded-lessons/SKILL.md
- days/day-07-touch-calibration/SPEC.md
- days/day-30-trivia/SPEC.md
- days/day-30-trivia/tests/contract.json

Treat the prompt, spec, tests, and acceptance criteria as source.
Put deterministic behavior in a portable core and keep Arduino, LVGL, M5Unified, storage, network, audio, and sensor adapters outside it.
Run scripts/prompt-contract/validate_day.sh days/day-30-trivia before compiling a device candidate.
Do not edit the contract or tests to make a candidate pass.

Hardware and shared platform

- Target board_M5StopWatch on ESP32-S3 with OPI PSRAM.
- Hold the device with the lanyard loop at the bottom: BtnA is physical left and BtnB is physical right.
- Immediately after M5.begin(), set panel width 468, height 466, offset_x 6, offset_y 0, then rotation 0.
- Never draw rows 466 or 467. Keep focus decoration inset and every interactive bound inside the round safe area.
- Load Preferences namespace espt-touch, blob key record, after display setup and before touch input.
- Accept and validate Day 07 version 1 and version 2 records. Read raw touch, apply the complete shared warp, clamp once, then feed the mapped point to the application.
- Provide the shared runtime calibration entry path. App-only installation must preserve the record.
- Allocate retained frames and large working buffers in PSRAM; fail visibly and over serial when required hardware or memory is unavailable.

Controls

Use a localized control scheme because True and False must be immediate one-button answers. BtnA/left: answer true. BtnB/right: answer false. Short both: host next. Hold both: leave game.
Touch must dispatch the same semantic actions as physical controls.

Lesson behavior

- Choose Host or Player at startup. Hosts own the deck and qid; players answer True on BtnA and False on BtnB.
- Timestamp the physical press locally, lock after the first answer, and reject duplicate or stale qids.
- Use versioned ESP-NOW question, answer, verdict, score, and presence messages between identical Stopwatches.
- Resolve ties by a documented clock-offset/round-trip method or declare them tied; never rank by packet arrival alone.
- Run all twelve baked questions, keep scores consistent, and recover a player that briefly loses the host.

Diagnostics and acceptance

- Prefix serial protocol lines with D30_ and implement status, reset, action, capture, and touchlog commands.
- Report hardware identity, corrected geometry, touch-map version and generation, current portable state, memory headroom, and last named error.
- Injected actions must use the same reducer, hit testing, callbacks, and control recognizer as physical input.
- Retain the exact RGB565 pixels sent to M5GFX and stream captures in bounded chunks without blocking the watchdog.
- Run every scenario in tests/contract.json, generated deterministic sequences, the deliberate mutation, and the lesson-specific checks in SPEC.md.
- Complete 20 largest-state round trips without reset or growing heap/PSRAM use.
- Confirm the active touch-map version and generation remain unchanged.
- Physically review touch alignment, BtnA/BtnB direction, chord behavior, cues, clipping, lower-edge use, and the absence of a black bar.

Record host, compile, injected-device, and physical evidence separately.
Never describe reference rendering or injected input as physical proof.
```

## Required behavior

- Choose Host or Player at startup. Hosts own the deck and qid; players answer True on BtnA and False on BtnB.
- Timestamp the physical press locally, lock after the first answer, and reject duplicate or stale qids.
- Use versioned ESP-NOW question, answer, verdict, score, and presence messages between identical Stopwatches.
- Resolve ties by a documented clock-offset/round-trip method or declare them tied; never rank by packet arrival alone.
- Run all twelve baked questions, keep scores consistent, and recover a player that briefly loses the host.

## Recorded evidence

The shared contract runner validates Stopwatch geometry, touch-map ownership, control metadata, state types and ranges, deterministic scenarios, round-face control bounds, and 10,000 generated actions against three disposable reducer shapes.
A deliberately mutated reducer must fail.

Historical implementation evidence from before the prompt-first Stopwatch migration follows.

**Recorded evidence · September 22, 2026:** A complete 12-question game was played under script control across both consoles — twelve questions broadcast, twelve answers ruled, twelve verdicts delivered with press timestamps, final score 10/12 exactly matching the two deliberately wrong answers injected by the script.

The reference screenshot is acceptance-model evidence.
The next hardware pass must replace or supplement it with a framebuffer capture and a hand-review record.
