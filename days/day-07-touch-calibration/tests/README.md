# Contract test

Run the suite against any candidate directory containing `touch_calibration_core.h` and `touch_calibration_core.cpp`:

```sh
days/day-07-touch-calibration/tests/run-candidate.sh PATH_TO_CANDIDATE
```

The runner uses C++17 with `-Wall -Wextra -Werror -pedantic`.
It does not require Arduino, M5Unified, M5GFX, a serial port, or attached hardware.

The test intentionally compiles against the candidate's public header.
A missing symbol, changed structure, incompatible constant, warning, or behavioral mismatch fails the candidate.

Read `../TESTING.md` for the checks that belong above this portable layer.
