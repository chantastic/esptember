# Day 08 testing model

The portable suite treats the interface as a reducer: the same starting state and input must always produce the same result.
It checks default state, focus wrap, count activation, brightness edit and limits, Orange mode, About navigation, touch behavior, state validity, brightness conversion, and round-screen bounds.

Run it against any candidate directory containing `lvgl_basics_core.h` and `lvgl_basics_core.cpp`:

```sh
days/day-08-lvgl-basics/tests/run-candidate.sh PATH_TO_CANDIDATE
```

Run the Day 07 suite against the candidate's copied touch-calibration core as a separate contract. This proves the coordinate transform and saved-record behavior; it does not prove the LVGL adapter read the controller correctly.

The real-library build catches LVGL, M5Unified, and M5GFX API drift. The serial device harness proves that injected input reaches real LVGL hit testing and callbacks and that screen transitions do not leak memory.

After installing a disposable candidate, run the reusable device harness with:

```sh
uv run --with pyserial --with pillow \
  days/day-08-lvgl-basics/scripts/check-device.py SERIAL_PORT
```

It also verifies that the active calibration-map version and generation stay unchanged, then writes framebuffer captures and its report under the ignored `.build/verification/` directory.

Only the attached Stopwatch can prove physical button mapping, finger alignment, perceived brightness, the absence of clipped focus rings, and the absence of a bottom-edge bar.
