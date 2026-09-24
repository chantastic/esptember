# Day 09 contract results

The generalized suite was run against three disposable reducer shapes generated from `tests/contract.json`.
All three passed 70,202 state, scenario, determinism, layout, hardware, control, and generated-sequence assertions.
A fourth candidate with its first transition removed failed as required.

The reference frame was rendered from the contract's declared visual scenario and checked at 468 × 466.
It is a design oracle, not device proof.

## Generated candidate · September 24, 2026

The selected disposable state and synthesis core passed 300,065 focused host assertions.
Those checks cover deterministic nonmutation, immediate retriggering, empty slots, all six PCM hashes and sample bounds, sound-manifest completeness, recipe fields, and the total audio-memory budget.

The complete adapter compiled with Arduino ESP32 3.3.10, M5Unified 0.2.19, M5GFX 0.2.26, and LVGL 9.3.0.
The application used 914,419 bytes of flash and 31,352 bytes of static RAM.
The only build note came from a toolchain object and was not a project warning.

It was written only to the application partition at `0x10000`.
The device loaded the same `espt-touch` version-2 record at generation 2 before and after installation.
At startup it prepared all six configured clips into 94,372 bytes of PSRAM before enabling the pads.

The injected-device harness passed:

- corrected 468 × 466 geometry and M5Stack Stopwatch identity;
- BtnA/left previous-page and BtnB/right next-page paths, measured at 1,077 µs and 1,080 µs;
- LVGL hit-testing and audio start through 100 injected touch-downs, with a 334 µs median, 496 µs 95th percentile, and 1,334 µs maximum;
- immediate single-voice replacement: Laser began in 1,223 µs, then Coin and Explosion replaced it in 272 µs and 288 µs;
- all six prepared PCM paths started and completed;
- page-two empty slots produced no audio or play-count change;
- retained RGB565 capture of both pages; and
- 20 two-page round trips with unchanged 265,876-byte free heap and 7,352,348-byte free PSRAM.

The final device state is page one, no active sound, no last trigger, zero plays, and no error.
Physical sound quality, volume, pusher feel, pad responsiveness, and calibrated touch alignment remain for hand review.
