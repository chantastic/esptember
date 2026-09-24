---
name: build-stopwatch-lessons
description: Build or revise ESPtember lessons from Day 07 onward when they target the M5Stack Stopwatch, including its display geometry, physical controls, shared touch map, prompt-first source, and device acceptance workflow.
---

# Build M5Stack Stopwatch lessons

Use this skill for ESPtember Day 07 and every later lesson that runs on the M5Stack Stopwatch Dev Kit (C152).

Read the repository [AGENTS.md](../../../AGENTS.md) first. For any touch consumer, also read the current [Day 07 calibration contract](../../../days/day-07-touch-calibration/SPEC.md). Those files are authoritative when this summary differs.

## Preserve the source boundary

Treat the detailed human-readable prompt, behavior specification, generalized tests, and explicit acceptance criteria as the lesson source. Generated firmware is a disposable candidate used to evaluate that source. Do not turn a fix discovered in one candidate into a private implementation patch; express the rule in the prompt or contract and add an observable test when possible.

Keep portable contract tests, real-library compilation, instrumented device checks, and physical observations as separate evidence. Never describe compilation or injected input as physical verification.

## Configure the physical device

- Target `board_M5StopWatch` on ESP32-S3 with the lanyard loop at the bottom.
- Configure the display panel immediately after `M5.begin()` to width 468, height 466, `offset_x = 6`, and `offset_y = 0`, then set rotation 0.
- Keep drawing and layout inside rows 0–465. The unused rows otherwise appear as a distracting black bar.
- `M5.BtnA` is the physical left button. `M5.BtnB` is the physical right button.
- Default to the C25K grammar: left/A is Previous or Decrease; right/B is Next or Increase; a short two-button chord is Enter; a 600 ms two-button hold is Back. A chord joins within 80 ms, overlaps for at least 80 ms, consumes both individual clicks, and never fires Enter after a hold.
- A familiar activity may use a stronger local convention, such as Start/Stop in a stopwatch. State and test that exception.

## Consume the shared touch map

Calibration belongs to the device and changes at runtime. Never compile one device's coefficients into a lesson.

- Use Preferences namespace `espt-touch` and blob key `record`.
- Recognize the 40-byte version-1 affine record and the 440-byte version-2 record.
- Version 2 contains the affine seed plus a center residual and 16 residual vectors at radii 70, 120, and 170 pixels. Validate its geometry, checksum, finite values, and safety limits before use.
- Read raw controller coordinates and apply the complete shared warp for every application touch. Calling M5GFX affine conversion alone loses the local residual field.
- Feed the shared module's mapped point into LVGL or application hit testing. Add no lesson-specific offsets, axis swaps, scale factors, or calibration constants.
- Load the record after panel geometry is corrected and before creating a touch consumer.
- Expose the shared guided calibration as a runtime action. A successful commit applies immediately and increments generation; cancel preserves the active map.
- App-only and OTA updates preserve the namespace. A merged image written at address `0x0` may erase NVS and must be labeled as resetting calibration.

## Carry forward the Day 07 interaction findings

- Cardinal calibration shows one white center dot and one orange destination dot. Draw no connecting line. Use `Drag from the center {DIRECTION} to the orange dot` for UP, DOWN, LEFT, then RIGHT.
- Around the World is collection, not a tracing exam. Collect clockwise and counterclockwise separately, preserve progress across lifts, accept imperfect radii, and give the two passes equal weight.
- Asteroid Shooter starts on a separate black screen with `ASTEROID SHOOTER` and `Tap to start`. Discard the starting tap.
- During its 51 measurements, draw only the current target on black. Titles, instructions, progress, footers, impact marks, and miss vectors hide regions that need measurement.
- Include the 170-pixel target ring so the lower and outer face contributes data. Accept every finite tap as data, including visible misses.

## Verify a generated lesson

Run its portable suite against independent disposable candidates when the prompt itself is under evaluation. Compile the selected candidate with the repository's pinned Arduino ESP32, M5Unified, and M5GFX versions.

When the user has authorized loading the attached device, use an app update that preserves NVS unless a fresh install is the explicit test. Emit concise serial evidence for hardware identity, 468 × 466 geometry, touch-map version and generation, screen transitions, stack headroom, input events, and final status.

Exercise the largest-memory transition and the physical controls. Inspect every round-screen layout for clipped focus areas and bottom-edge artifacts. Record only the behavior actually observed.
