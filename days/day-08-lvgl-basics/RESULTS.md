# Three-build comparison

Three ignored portable cores were built from the Day 08 contract.
They model the lesson's behavior without Arduino or LVGL so the prompt can be tested before a device adapter exists.

| Candidate | State approach | Contract result | Core source | Optimized object |
| --- | --- | ---: | ---: | ---: |
| A | Direct switch reducer | 140,147/140,147 | 75 lines / 3,371 bytes | 2,840 bytes |
| B | Action table | 140,147/140,147 | 22 lines / 2,268 bytes | 3,512 bytes |
| C | Composed action functions | 140,147/140,147 | 18 lines / 2,634 bytes | 3,200 bytes |

The measurements used Apple Clang with C++17, strict warnings, and `-Os` on September 23, 2026.
They compare the portable behavior only.

All candidates agree on defaults, wrapped focus, button editing, brightness limits, Count and Orange activation, About navigation, touch semantics, invalid-state rejection, panel-brightness conversion, exact control bounds, round-face containment, and 20,000 generated input steps.

Candidate A is easiest to review because each rule appears directly in the reducer.
Candidate B has the smallest source but produces the largest object.
Candidate C separates movement and activation cleanly and compiles between the other two.

A mutation changed the brightness button step from 10 to 20 in a disposable Candidate A copy.
Five of 140,147 assertions failed and the runner exited with status 1.

The suite does not prove LVGL drawing, M5Stack hardware, the saved touch record, or finger alignment.
Those belong to the integration and physical checks in `SPEC.md`.

## Regenerated device candidate

An ignored device candidate was generated from the prompt and installed on the attached M5Stack Stopwatch as an app update, preserving NVS.
It compiled for the ESP32-S3 with LVGL 9.3.0 and loaded the saved Day 07 map as version 2 generation 2.

The automated device harness passed Count, brightness at both limits and an intermediate value, Orange mode in both states, About and Back, focus and brightness editing through the button recognizer, framebuffer captures, and 20 navigation round trips.
The final status reported 277,968 bytes of free heap and 7,499,828 bytes of free PSRAM, unchanged across the navigation loop.
The calibration map remained version 2 generation 2.

This run establishes that a generated Day 08 adapter can consume the device-owned nonlinear map without baking its coefficients into the sketch.
Finger alignment, physical button feel, focus clipping on glass, and the bottom edge remain hands-on acceptance checks.
