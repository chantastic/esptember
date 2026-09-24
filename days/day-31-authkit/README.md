---
board: m5stack-stopwatch
day: 31
title: WorkOS AuthKit
toolchain: Arduino ESP32 3.3.10 + M5Unified 0.2.19 + M5GFX 0.2.26 + LVGL 9.3.0
summary: "OAuth device authorization on a keyboardless Stopwatch with explicit polling and secret boundaries."
verification: "Prompt contract and reference frame verified; generated hardware candidate awaits hand review"
---

## The assignment

Sign a real user in from a device with no keyboard or browser using the OAuth device flow.

The durable source is this prompt, [SPEC.md](https://github.com/chantastic/esptember/blob/main/days/day-31-authkit/SPEC.md), the machine-readable contract, and its layered acceptance criteria.
Generated firmware is a disposable candidate.

## Reference frame

![Round-screen acceptance reference for WorkOS AuthKit](https://esptember.com/images/day-31-authkit/reference.png)

This is a deterministic screenshot of the visual acceptance model.
It defines the intended hierarchy and safe-area use; only a later framebuffer capture from the attached Stopwatch can count as device rendering evidence.

## The build prompt

```text
Build ESPtember Day 31: WorkOS AuthKit for the M5Stack Stopwatch Dev Kit (C152).

Read these before writing code:
- .agents/skills/build-stopwatch-lessons/SKILL.md
- .agents/skills/prompt-first-embedded-lessons/SKILL.md
- days/day-07-touch-calibration/SPEC.md
- days/day-31-authkit/SPEC.md
- days/day-31-authkit/tests/contract.json

Treat the prompt, spec, tests, and acceptance criteria as source.
Put deterministic behavior in a portable core and keep Arduino, LVGL, M5Unified, storage, network, audio, and sensor adapters outside it.
Run scripts/prompt-contract/validate_day.sh days/day-31-authkit before compiling a device candidate.
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

- Provision only the public client id; never ship a client secret or WorkOS API key to the device.
- Request a device code, show a scannable QR plus human code, and poll at the server-provided interval.
- Handle authorization_pending, slow_down, access_denied, expired_token, network failure, and success by name.
- Left/right focus Retry and Cancel; Enter acts; Back cancels polling and returns to setup.
- Redact tokens from logs and captures; display only safe identity claims after signature/trust checks appropriate to the flow.

Diagnostics and acceptance

- Prefix serial protocol lines with D31_ and implement status, reset, action, capture, and touchlog commands.
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

- Provision only the public client id; never ship a client secret or WorkOS API key to the device.
- Request a device code, show a scannable QR plus human code, and poll at the server-provided interval.
- Handle authorization_pending, slow_down, access_denied, expired_token, network failure, and success by name.
- Left/right focus Retry and Cancel; Enter acts; Back cancels polling and returns to setup.
- Redact tokens from logs and captures; display only safe identity claims after signature/trust checks appropriate to the flow.

## Recorded evidence

The shared contract runner validates Stopwatch geometry, touch-map ownership, control metadata, state types and ranges, deterministic scenarios, round-face control bounds, and 10,000 generated actions against three disposable reducer shapes.
A deliberately mutated reducer must fail.

Historical implementation evidence from before the prompt-first Stopwatch migration follows.

**Recorded evidence · September 22, 2026:** The complete flow was verified live on hardware: the board requested a code pair from WorkOS (`D30_CODE user_code=TXVL-DDCF`), displayed the QR and code, and — after approval on a phone — reported `D30_AUTHORIZED` with the signed-in user's email, all observed over serial while it happened.

The reference screenshot is acceptance-model evidence.
The next hardware pass must replace or supplement it with a framebuffer capture and a hand-review record.
