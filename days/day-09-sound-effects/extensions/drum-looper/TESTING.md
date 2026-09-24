# Drum Looper testing

Run the portable state contract with:

```sh
days/day-09-sound-effects/extensions/drum-looper/tests/run.sh
```

The runner generates list, table, and direct-function reducers from `tests/contract.json`, checks exact intermediate states and 10,000 generated actions for each, and requires a deliberately damaged reducer to fail.
It proves the declared workflow is internally consistent; it does not prove musical timing, quantization math, speaker mixing, LVGL, touch, or physical control feel.

A generated candidate adds domain fixtures for:

- 16 bars × 4 beats × 4 subdivisions and exact 0–255 indexing;
- quantization immediately before, at, and after a half-step, including the step-255 wrap;
- four independent 256-bit tracks, duplicate-hit collapse, and independently anchored 256-step replacement windows;
- pause/resume fractional phase and absolute-deadline scheduling across timer rollover;
- median tap tempo with valid, invalid, stale, minimum, and maximum intervals;
- simultaneous multi-track dispatch on one boundary and explicit lateness accounting;
- deterministic Kick, Snare, Hi-Hat, and Crash PCM hashes, sample bounds, and memory budget; and
- reset clearing patterns while preserving BPM and the shared calibration record.

On an instrumented Stopwatch, exercise real LVGL hit testing and the audio mixer with injected timestamps.
Measure 100 immediate pad auditions, 16-bar scheduling under UI load, 100 complete loop passes, repeated replace/clear cycles, memory stability, framebuffer captures, and touch-map preservation.

Finish with [HAND-REVIEW.md](HAND-REVIEW.md).
Only a person playing and listening can establish groove, tap-tempo feel, drum balance, and thumb/index ergonomics.
