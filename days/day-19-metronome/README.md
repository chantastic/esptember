---
board: m5stack-stopwatch
day: 19
title: Metronome
toolchain: Arduino ESP32 3.3.10 + M5Unified 0.2.19 + M5GFX 0.2.26 + LVGL 9.3.0
summary: "A drift-resistant musical clock with tempo, meter, tap tempo, sound, and haptics."
verification: "Prompt contract and reference frame verified; generated hardware candidate awaits hand review"
---

## The assignment

Build a metronome whose scheduling remains musical while UI and feedback run concurrently.

The durable source is this prompt, [SPEC.md](https://github.com/chantastic/esptember/blob/main/days/day-19-metronome/SPEC.md), the machine-readable contract, and its layered acceptance criteria.
Generated firmware is a disposable candidate.

## Reference frame

![Round-screen acceptance reference for Metronome](https://esptember.com/images/day-19-metronome/reference.png)

This is a deterministic screenshot of the visual acceptance model.
It defines the intended hierarchy and safe-area use; only a later framebuffer capture from the attached Stopwatch can count as device rendering evidence.

## The build prompt

```text
Build ESPtember Day 19: Metronome for the M5Stack Stopwatch Dev Kit (C152).

Read these before writing code:
- .agents/skills/build-stopwatch-lessons/SKILL.md
- .agents/skills/prompt-first-embedded-lessons/SKILL.md
- days/day-07-touch-calibration/SPEC.md
- days/day-19-metronome/SPEC.md
- days/day-19-metronome/tests/contract.json

Treat the prompt, spec, tests, and acceptance criteria as source.
Put deterministic behavior in a portable core and keep Arduino, LVGL, M5Unified, storage, network, audio, and sensor adapters outside it.
Run scripts/prompt-contract/validate_day.sh days/day-19-metronome before compiling a device candidate.
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

Use a localized control scheme because Start/stop and tap tempo have established musical meanings. BtnA/left: start stop. BtnB/right: tap tempo. Short both: edit selected. Hold both: return to tempo.
Touch must dispatch the same semantic actions as physical controls.

Lesson behavior

- BtnA starts and stops. BtnB taps tempo; use a robust median of recent intervals and ignore implausible gaps.
- Touch or a focused settings row changes 40–240 BPM, meter 2/4 through 6/8, sound, and vibration.
- Schedule beats from an absolute next-deadline sequence so rendering and feedback never accumulate drift.
- Accent beat one and keep the visual pendulum phase tied to the same clock.
- Changing tempo while running takes effect on the next beat without double-firing.

Diagnostics and acceptance

- Prefix serial protocol lines with D19_ and implement status, reset, action, capture, and touchlog commands.
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

- BtnA starts and stops. BtnB taps tempo; use a robust median of recent intervals and ignore implausible gaps.
- Touch or a focused settings row changes 40–240 BPM, meter 2/4 through 6/8, sound, and vibration.
- Schedule beats from an absolute next-deadline sequence so rendering and feedback never accumulate drift.
- Accent beat one and keep the visual pendulum phase tied to the same clock.
- Changing tempo while running takes effect on the next beat without double-firing.

## Recorded evidence

The shared contract runner validates Stopwatch geometry, touch-map ownership, control metadata, state types and ranges, deterministic scenarios, round-face control bounds, and 10,000 generated actions against three disposable reducer shapes.
A deliberately mutated reducer must fail.

Historical implementation evidence from before the prompt-first Stopwatch migration follows.

**Recorded evidence · September 22, 2026:** The beat engine, tap-tempo math, and transport state were verified over serial (`D18_STATUS` reporting) on the installed firmware. Timing feel and the reference-metronome comparison await a hands-on check, noted in NOTES.md.

The reference screenshot is acceptance-model evidence.
The next hardware pass must replace or supplement it with a framebuffer capture and a hand-review record.
