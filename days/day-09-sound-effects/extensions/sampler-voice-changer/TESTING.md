# Sampler + Voice Changer testing

Run the portable state contract with:

```sh
days/day-09-sound-effects/extensions/sampler-voice-changer/tests/run.sh
```

It generates list, table, and direct-function reducers from `tests/contract.json`, checks exact intermediate states and 10,000 generated actions for each, and requires a deliberately damaged reducer to fail.
This proves the declared workflow is internally consistent; it does not prove microphone capture, signal processing, the speaker, LVGL, PSRAM, or physical controls.

A generated candidate adds domain fixtures for:

- silence, below-threshold voice, valid sine, impulse, DC-offset, clipped, 300 ms, and three-second captures;
- 64-bit RMS math, clipping percentage, DC removal, 5/10 ms fades, normalization gain, rounding, and saturation;
- exact output length and a stable PCM hash for each of the eight effects;
- interpolation endpoints and proof that no transform reads outside its input;
- a maximum two-second preparation time and 2 MiB total PCM budget; and
- old-buffer zeroing after rerecord and errors.

On the instrumented Stopwatch, feed a known synthetic fixture into the real processing adapter without reading microphone data back over serial.
Measure all eight speaker-start paths, 100 pad touch-downs, page pushers, screenshots, and 20 process/clear cycles.
Confirm free memory returns to baseline and touch-map version/generation remain unchanged.

Finish with [HAND-REVIEW.md](HAND-REVIEW.md).
Only a person listening to a real recording can establish capture quality, effect character, feedback avoidance, and physical feel.
