# Day 09 contract results

The generalized suite was run against three disposable reducer shapes generated from `tests/contract.json`.
All three passed 80,151 state, scenario, determinism, layout, hardware, control, and generated-sequence assertions.
A fourth candidate with its first transition removed failed as required.

The reference frame was rendered from the contract's declared visual scenario and checked at 468 × 466.
It is a design oracle, not device proof.

## Generated candidate · September 24, 2026

The selected disposable state and synthesis core passed 300,046 focused host assertions.
The copied C25K recognizer passed its 98 portable button assertions.

The complete adapter compiled without project warnings using Arduino ESP32 3.3.10, M5Unified 0.2.19, M5GFX 0.2.26, and LVGL 9.3.0.
The application used 952,891 bytes of flash and 30,904 bytes of static RAM.

It was written only to the application partition at `0x10000`.
The device loaded the same `espt-touch` version-2 record at generation 2 before and after installation.

The injected-device harness passed:

- corrected 468 × 466 geometry and M5Stack Stopwatch identity;
- BtnA/BtnB direction, wrap, short-chord Enter, and held-chord Back through the production recognizer;
- LVGL hit-testing through injected touch points;
- all six deterministic PCM synthesis and speaker-start paths;
- one active voice with a single latest-wins pending slot;
- retained RGB565 capture of both pages;
- 20 page-two/back round trips with unchanged 260,704-byte free heap and 7,475,248-byte free PSRAM; and
- unchanged calibration version and generation.

The final device state is page one, Laser focused, no active or queued sound, and zero plays.
Physical sound quality, volume, button feel, and calibrated touch alignment remain for hand review.
