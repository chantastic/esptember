---
board: m5stack-stopwatch
day: 15
title: Sound Level (dB)
toolchain: Arduino ESP32 3.3.10 + M5Unified 0.2.19 + M5GFX 0.2.26 + LVGL 9.3.0
summary: "An honest dBFS meter with smoothing, peak hold, and explicit limits on calibration."
verification: "Prompt contract and reference frame verified; generated hardware candidate awaits hand review"
---

## The assignment

Turn microphone samples into a stable sound-level display without pretending dBFS is calibrated SPL.

The durable source is this prompt, [SPEC.md](https://github.com/chantastic/esptember/blob/main/days/day-15-sound-level/SPEC.md), the machine-readable contract, and its layered acceptance criteria.
Generated firmware is a disposable candidate.

## Reference frame

![Round-screen acceptance reference for Sound Level (dB)](https://esptember.com/images/day-15-sound-level/reference.png)

This is a deterministic screenshot of the visual acceptance model.
It defines the intended hierarchy and safe-area use; only a later framebuffer capture from the attached Stopwatch can count as device rendering evidence.

## The build prompt

```text
Build ESPtember Day 15: Sound Level (dB) for the M5Stack Stopwatch Dev Kit (C152).

Read these before writing code:
- .agents/skills/build-stopwatch-lessons/SKILL.md
- .agents/skills/prompt-first-embedded-lessons/SKILL.md
- days/day-07-touch-calibration/SPEC.md
- days/day-15-sound-level/SPEC.md
- days/day-15-sound-level/tests/contract.json

Treat the prompt, spec, tests, and acceptance criteria as source.
Put deterministic behavior in a portable core and keep Arduino, LVGL, M5Unified, storage, network, audio, and sensor adapters outside it.
Run scripts/prompt-contract/validate_day.sh days/day-15-sound-level before compiling a device candidate.
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

Use the default grammar: BtnA/left is Previous or Decrease; BtnB/right is Next or Increase; short both is Enter; hold both for 600 ms is Back. Preserve the 80 ms join and overlap rules and consume chord clicks.
Touch must dispatch the same semantic actions as physical controls.

Lesson behavior

- Capture signed PCM, remove DC, compute RMS, and convert to dBFS with a named silence floor.
- Smooth the live level separately from the peak. Hold peak for three seconds, then release it toward live level.
- Focus Reset Peak and Integration. Enter resets peak or cycles Fast, Medium, and Slow smoothing.
- Show dBFS everywhere; never label the value dB SPL without a physical calibration procedure.
- Reject NaN, infinity, clipped buffers, and zero-length input without corrupting the display state.

Diagnostics and acceptance

- Prefix serial protocol lines with D15_ and implement status, reset, action, capture, and touchlog commands.
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

- Capture signed PCM, remove DC, compute RMS, and convert to dBFS with a named silence floor.
- Smooth the live level separately from the peak. Hold peak for three seconds, then release it toward live level.
- Focus Reset Peak and Integration. Enter resets peak or cycles Fast, Medium, and Slow smoothing.
- Show dBFS everywhere; never label the value dB SPL without a physical calibration procedure.
- Reject NaN, infinity, clipped buffers, and zero-length input without corrupting the display state.

## Recorded evidence

The shared contract runner validates Stopwatch geometry, touch-map ownership, control metadata, state types and ranges, deterministic scenarios, round-face control bounds, and 10,000 generated actions against three disposable reducer shapes.
A deliberately mutated reducer must fail.

Historical implementation evidence from before the prompt-first Stopwatch migration follows.

**Recorded evidence · September 21, 2026:** Live readings observed over serial on the installed firmware — quiet-room tone around -54 dBFS with peak-hold tracking at -47 dBFS and the level responding to ambient sound.

The reference screenshot is acceptance-model evidence.
The next hardware pass must replace or supplement it with a framebuffer capture and a hand-review record.
