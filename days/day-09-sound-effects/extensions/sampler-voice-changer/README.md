# Follow-on exercise: Sampler + Voice Changer

## The assignment

Record one sound with the Stopwatch microphone.
Then play that same recording through eight different effects on the colored pads.

The exercise extends Day 09 instead of displacing the existing Day 10 lesson.
It keeps the same two-page pad layout and immediate playback behavior, then adds microphone capture, sample validation, deterministic effect rendering, and safe replacement of a take.

## Reference frame

![Round-screen acceptance reference for the sampler effect pads](https://esptember.com/images/day-09-sound-effects/sampler-reference.png)

The reference defines the effect-pad screen after a successful take.
The recorder, countdown, recording, processing, and error states are specified in [SPEC.md](SPEC.md).

## The build prompt

```text
Build the Sampler + Voice Changer follow-on exercise for ESPtember Day 09 on the M5Stack Stopwatch Dev Kit (C152).

Read these before writing code:
- .agents/skills/build-stopwatch-lessons/SKILL.md
- .agents/skills/prompt-first-embedded-lessons/SKILL.md
- days/day-07-touch-calibration/SPEC.md
- days/day-09-sound-effects/SPEC.md
- days/day-09-sound-effects/extensions/sampler-voice-changer/SPEC.md
- days/day-09-sound-effects/extensions/sampler-voice-changer/tests/contract.json

Treat the prompt, spec, tests, and acceptance criteria as source.
Generated firmware is a disposable candidate.
Put deterministic state, take validation, PCM normalization, and every effect transform in a portable core.
Keep M5Unified microphone/speaker setup, LVGL, touch, PSRAM, and serial adapters outside that core.
Run the extension contract before compiling a device candidate, and never edit the contract to make a candidate pass.

Hardware and shared platform

- Target board_M5StopWatch on ESP32-S3 with OPI PSRAM.
- Configure the panel immediately after M5.begin() to 468 × 466, offset (6,0), rotation 0, and never draw rows 466 or 467.
- Load and apply the complete shared espt-touch/record map before creating the LVGL pointer input.
- Preserve calibration through app-only updates. Holding both pushers during boot enters the shared calibration flow.
- BtnA is physical left and BtnB is physical right with the lanyard down.
- Use the internal ES8311 microphone and internal speaker through M5Unified. Never monitor the live microphone through the speaker.
- Stop the speaker before capture and stop the microphone before playback or effect preparation.

Recording flow

- Boot to a black recorder screen titled MAKE A SAMPLE with one large red TAP TO RECORD circle and the note Up to 3 seconds.
- A tap starts a visible 3–2–1 countdown. A tap during the countdown cancels safely.
- After the countdown, record signed 16-bit mono PCM at 22,050 Hz into PSRAM.
- During recording show only a red recording dot, an elapsed timer, a bounded live waveform, and Tap or press a pusher to stop.
- Stop on the next touch-down, either pusher, or three seconds. Require at least 300 ms.
- Validate the take for capture success, duration, peak, RMS energy, and clipping. Name too-short, too-quiet, too-clipped, capture, and memory failures on screen and serial, then return to the recorder.
- Remove DC, apply a short edge fade, and normalize the accepted take to a -3 dBFS peak with gain capped at 8×.
- Keep the original capture and all derived audio in volatile PSRAM only. Do not write voice audio to flash, NVS, SD, network, logs, or crash output. Zero old buffers before a rerecord and on teardown.

Effect preparation

- Show PROCESSING after an accepted take and prepare all eight effects before enabling the pads.
- Use Q1.15 coefficients, a checked-in 1,024-entry Q1.15 sine table, 64-bit intermediates, defined rounding, and saturating signed 16-bit output. Given the same normalized PCM, every effect must produce the same length and PCM hash on every build.
- CLEAN: normalized input unchanged.
- CHIPMUNK: linear resampling with source advance 3/2 samples per output sample.
- MONSTER: linear resampling with source advance 2/3 samples per output sample, followed by a one-pole low-pass filter with coefficient 0.18.
- ROBOT: multiply the sample by a 70 Hz sine carrier, with output gain 0.85.
- ECHO: dry input plus 130 ms and 260 ms taps at gains 0.50 and 0.25.
- REVERSE: copy samples in reverse order.
- STUTTER: copy the first 300 ms, repeat a 90 ms slice centered at 40% of the take six times, then copy the final 300 ms; clamp source windows for short valid takes.
- ALIEN: keep the original duration; read through a 6 Hz sinusoidal ±18-sample delay using linear interpolation, then mix 55% dry with 45% of a 43 Hz ring-modulated signal.
- Cap every derived clip at four seconds and total capture-plus-effect PCM at 2 MiB. A preparation failure returns to the recorder without exposing partial pads.
- Complete preparation within two seconds for a maximum-length valid take.

Pad interaction

- Page 1 contains CLEAN, CHIPMUNK, MONSTER, and ROBOT.
- Page 2 contains ECHO, REVERSE, STUTTER, and ALIEN.
- Reuse Day 09's four 132 × 118 colored pads, small labels underneath, and A ◀ page/count ▶ B footer.
- BtnA/left shows the previous page. BtnB/right shows the next page. Pages wrap.
- Fire a pad on LV_EVENT_PRESSED from its prepared resident PCM.
- Keep one voice. The newest pad immediately stops and replaces the current effect; pressing the same pad restarts it.
- Hold both pushers for 600 ms from a pad screen to rerecord. This stops playback, zeros the capture and all effect buffers, clears play state, and returns to the recorder.
- Perform no effect processing, resampling, file I/O, allocation, release wait, or long logging in a pad callback.

Diagnostics and acceptance

- Prefix serial lines with D09S_ and implement status, reset, action, capture, and touchlog commands.
- Report hardware identity, corrected geometry, touch-map version and generation, screen, page, sample state, captured sample count, take count, active effect, last effect, play count, capture/effect bytes, heap, PSRAM, stack headroom, timing summaries, and the last named error.
- Do not expose recorded PCM through the normal serial protocol. A test-only synthetic input command may supply deterministic PCM directly to the portable processing path; it must not read the user's microphone buffer back out.
- Domain-test acceptance/rejection thresholds, normalization, fades, saturation, exact output lengths, deterministic hashes, out-of-bounds reads, total memory, and all eight transforms with silence, impulse, sine, clipped, minimum-length, and maximum-length fixtures.
- Measure maximum-length processing time and 100 injected pad triggers. Processing must finish within two seconds; pad-to-speaker-start p95 must be at most 30 ms and no trigger may exceed 50 ms.
- Capture the recorder and both pad pages, complete 20 record/process/rerecord cycles, and confirm memory returns to its baseline without changing the touch-map record.
- Physically review microphone intelligibility, every effect, feedback avoidance, stop controls, pusher direction, pad responsiveness, touch alignment, clipping, the lower edge, and the absence of a black bar.

Record host-contract, compile, injected-device, framebuffer, and physical evidence separately.
Do not describe synthetic input or automated playback checks as microphone or listening proof.
```

## The learning progression

Day 09 generates or imports independent sounds.
This exercise captures one personal sound, treats it as signal data, derives multiple versions, and makes preparation latency separate from performance latency.
The sampler should wait while it prepares; the pads should never make the player wait.
