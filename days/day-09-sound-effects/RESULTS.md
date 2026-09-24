# Day 09 contract results

The generalized suite was run against three disposable reducer shapes generated from `tests/contract.json`.
All three passed 70,215 state, scenario, determinism, layout, hardware, control, and generated-sequence assertions.
A fourth candidate with its first transition removed failed as required.

The reference frame was rendered from the contract's declared visual scenario and checked at 468 × 466.
It is a design oracle, not device proof.

## Generated candidate · September 24, 2026

The selected disposable state and synthesis core passed 300,080 focused host assertions.
Those checks cover deterministic nonmutation, immediate retriggering, both complete game kits, all eight PCM hashes and sample bounds, sound-manifest completeness, recipe fields, and the total audio-memory budget.

The complete adapter compiled with Arduino ESP32 3.3.10, M5Unified 0.2.19, M5GFX 0.2.26, and LVGL 9.3.0.
The application used 915,363 bytes of flash and 31,384 bytes of static RAM.
The only build note came from a toolchain object and was not a project warning.

It was written only to the application partition at `0x10000`.
The device loaded the same `espt-touch` version-2 record at generation 2 before and after installation.
At startup it prepared all eight configured clips into 149,498 bytes of PSRAM before enabling the pads.

The injected-device harness passed:

- corrected 468 × 466 geometry and M5Stack Stopwatch identity;
- BtnA/left previous-page and BtnB/right next-page paths, measured at 1,242 µs and 1,197 µs;
- LVGL hit-testing and audio start through 100 injected touch-downs, with a 329 µs median, 347 µs 95th percentile, and 542 µs maximum;
- immediate single-voice replacement: Jump began in 1,223 µs, then Coin and Mario Power Up replaced it in 426 µs and 272 µs;
- all eight prepared PCM paths started and completed;
- the Asteroid Crash and Power Up pads resolved to distinct clips;
- retained RGB565 capture of both pages; and
- 20 two-page round trips with unchanged 265,876-byte free heap and 7,352,348-byte free PSRAM.

The final device state is page one, no active sound, no last trigger, zero plays, and no error.
Physical sound quality, volume, pusher feel, pad responsiveness, and calibrated touch alignment remain for hand review.
