---
board: m5stack-stopwatch
day: 24
title: Walkie-Talkie
toolchain: Arduino ESP32 3.3.10 + M5Unified 0.2.19 + M5GFX 0.2.26 + LVGL 9.3.0
summary: "Push-to-talk PCM between identical Stopwatches over ESP-NOW with sequencing and jitter control."
verification: "Prompt contract and reference frame verified; generated hardware candidate awaits hand review"
---

## The assignment

Use two M5Stack Stopwatches as a local voice link with one familiar push-to-talk control.

The durable source is this prompt, [SPEC.md](https://github.com/chantastic/esptember/blob/main/days/day-24-walkie-talkie/SPEC.md), the machine-readable contract, and its layered acceptance criteria.
Generated firmware is a disposable candidate.

## Reference frame

![Round-screen acceptance reference for Walkie-Talkie](https://esptember.com/images/day-24-walkie-talkie/reference.png)

This is a deterministic screenshot of the visual acceptance model.
It defines the intended hierarchy and safe-area use; only a later framebuffer capture from the attached Stopwatch can count as device rendering evidence.

## The build prompt

```text
Build ESPtember Day 24: Walkie-Talkie for the M5Stack Stopwatch Dev Kit (C152).

Read these before writing code:
- .agents/skills/build-stopwatch-lessons/SKILL.md
- .agents/skills/prompt-first-embedded-lessons/SKILL.md
- days/day-07-touch-calibration/SPEC.md
- days/day-24-walkie-talkie/SPEC.md
- days/day-24-walkie-talkie/tests/contract.json

Treat the prompt, spec, tests, and acceptance criteria as source.
Put deterministic behavior in a portable core and keep Arduino, LVGL, M5Unified, storage, network, audio, and sensor adapters outside it.
Run scripts/prompt-contract/validate_day.sh days/day-24-walkie-talkie before compiling a device candidate.
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

Use a localized control scheme because Push-to-talk must track a held physical pusher directly. BtnA/left: ptt hold. BtnB/right: next channel. Short both: channel select. Hold both: return idle.
Touch must dispatch the same semantic actions as physical controls.

Lesson behavior

- BtnA transmits only while held. BtnB advances channels 1–22; short both opens direct channel selection.
- Capture 8 kHz, 16-bit mono in fixed frames that remain under the ESP-NOW payload limit.
- Version packets, sequence them, reject other channels, and count loss, late, duplicate, and malformed frames.
- Use a bounded jitter buffer with a named prefill target and silence insertion for missing frames.
- Never play local microphone audio and never block UI/button/touch work during capture or playback.

Diagnostics and acceptance

- Prefix serial protocol lines with D24_ and implement status, reset, action, capture, and touchlog commands.
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

- BtnA transmits only while held. BtnB advances channels 1–22; short both opens direct channel selection.
- Capture 8 kHz, 16-bit mono in fixed frames that remain under the ESP-NOW payload limit.
- Version packets, sequence them, reject other channels, and count loss, late, duplicate, and malformed frames.
- Use a bounded jitter buffer with a named prefill target and silence insertion for missing frames.
- Never play local microphone audio and never block UI/button/touch work during capture or playback.

## Recorded evidence

The shared contract runner validates Stopwatch geometry, touch-map ownership, control metadata, state types and ranges, deterministic scenarios, round-face control bounds, and 10,000 generated actions against three disposable reducer shapes.
A deliberately mutated reducer must fail.

Historical implementation evidence from before the prompt-first Stopwatch migration follows.

**Recorded evidence · September 22, 2026:** A deterministic 1-second 440 Hz test tone was streamed board-to-board under script control: 66 frames sent, 66 received, 0 lost to sequence gaps, 0 dropped at the queue, codec open confirmed (`D23_STATUS`, both consoles). Perceived audio quality through the speaker awaits an ear, noted in NOTES.md.

The reference screenshot is acceptance-model evidence.
The next hardware pass must replace or supplement it with a framebuffer capture and a hand-review record.
