# Follow-on exercise: Drum Looper

## The assignment

Turn the four-pad Day 09 instrument into a 16-bar drum looper.
The pads are Kick, Snare, Hi-Hat, and Crash.

Start playback, then tap any pad to replace that instrument's track.
The first tap clears only that pad's old pattern; every tap during the next full 16 bars becomes its replacement pattern.
The other three tracks keep playing untouched.

There is no record mode.
While the transport is playing, pad taps are heard immediately and recorded automatically.

## Reference frame

![Round-screen acceptance reference for the Drum Looper](https://esptember.com/images/day-09-sound-effects/drum-looper-reference.png)

The reference keeps Day 09's large four-pad view and adds only tempo, transport, bar position, and compact track-state cues.
It defines visual intent rather than device proof.

## Controls

Hold the Stopwatch with the lanyard down:

- **Thumb · BtnA/left:** tap tempo.
- **Index · BtnB/right:** play or pause.
- **Hold both for 600 ms:** clear every track, stop, and return to bar 1 without changing the tempo.
- **Touch a pad while playing:** hear it immediately and write it into that pad's replacement track.
- **Touch a pad while paused:** audition it without changing the loop.

## The build prompt

```text
Build the Drum Looper follow-on exercise for ESPtember Day 09 on the M5Stack Stopwatch Dev Kit (C152).

Read these before writing code:
- .agents/skills/build-stopwatch-lessons/SKILL.md
- .agents/skills/prompt-first-embedded-lessons/SKILL.md
- days/day-07-touch-calibration/SPEC.md
- days/day-09-sound-effects/SPEC.md
- days/day-09-sound-effects/extensions/drum-looper/SPEC.md
- days/day-09-sound-effects/extensions/drum-looper/tests/contract.json

Treat the prompt, spec, tests, and acceptance criteria as source.
Generated firmware is a disposable candidate.
Put transport arithmetic, quantization, per-track replacement, scheduling, and tap-tempo estimation in a portable core.
Keep M5Unified, LVGL, touch, speaker, clocks, and serial adapters outside it.
Run the extension contract before compiling a candidate, and never weaken the contract to make a candidate pass.

Hardware and shared platform

- Target board_M5StopWatch on ESP32-S3 with OPI PSRAM.
- Configure the panel immediately after M5.begin() to 468 × 466, offset (6,0), rotation 0, and never draw rows 466 or 467.
- Load and apply the complete shared espt-touch/record map before creating the LVGL pointer input.
- Preserve calibration through app-only updates. Holding both pushers during boot enters the shared calibration flow.
- With the lanyard down, BtnA is physical left and the thumb tempo control; BtnB is physical right and the index play/pause control.

Loop and tracks

- Use one fixed 4/4 loop containing 16 bars.
- Quantize each track to sixteenth notes: 16 steps per bar, 256 steps per loop.
- Keep four independent 256-bit tracks named Kick, Snare, Hi-Hat, and Crash.
- Use resident deterministic PCM for all four drum sounds and one immediate-retrigger speaker voice per sound or a bounded mixer that can start coincident hits without blocking the UI.
- Schedule from an absolute transport anchor. Never advance musical time by repeatedly delaying for one step.
- At loop wrap, increment the pass counter. Each track's replacement window continues until 256 steps have elapsed from that track's own first replacement tap.

Automatic per-track replacement

- A pad touch always auditions its sound immediately on touch-down.
- While paused, audition only; do not change any track.
- While playing, quantize the touch timestamp to the nearest sixteenth step with a deterministic tie rule.
- The first touch of a pad when that track is not already replacing clears all 256 old steps for only that pad, opens a 256-step replacement window from the quantized hit, and writes the new hit.
- Later touches of the same pad while its replacement window remains active add hits without clearing again.
- The first touch of another pad independently clears and replaces only that other track.
- When one track's 256-step window ends, its pattern becomes stored. Its next playing tap starts another whole-track replacement.
- Multiple touches quantized to the same track and step store one hit.
- The live audition is not delayed to the quantized boundary. Quantization controls future playback only.

Physical controls

- BtnA/left is tap tempo. The first tap arms measurement. From two through five valid taps, set tempo from the median of recent intervals.
- Accept intervals from 300 through 1,000 ms, corresponding to 200 through 60 BPM. An invalid interval or a gap over two seconds restarts the tap sequence without changing tempo.
- Clamp tempo to 60–200 BPM. Default to 120 BPM.
- A valid tempo update keeps the current step and uses the new period for the next boundary.
- BtnB/right toggles play and pause. Pause retains the current step and fractional phase; resume continues there.
- Hold both pushers for 600 ms at runtime to stop, clear all four tracks, clear replacement flags, and return to step 0. Preserve the current tempo.
- Consume individual button clicks when a two-button hold is recognized. Ignore a short two-button chord.

Layout

- Keep four 132 × 118 colored pads at the Day 09 positions with small labels underneath: KICK, SNARE, HI-HAT, CRASH.
- Show DRUM LOOPER, current BPM, PLAYING or PAUSED, and BAR 01–16 with beat/subdivision progress above the pads.
- A compact dot by each label is dim for an empty track, white for a stored pattern, and red while that track is inside its independent 256-step replacement window.
- Flash only the touched pad briefly. Do not animate the grid or delay its sound.
- The footer shows A TEMPO, B PLAY or B PAUSE, and HOLD BOTH: CLEAR.
- Keep every focus/pressed state inside the radius-226 safe area and leave no bottom strip.

Diagnostics and acceptance

- Prefix serial lines with D09L_ and implement status, reset, action, advance, capture, touchlog, and calibrate commands.
- Report hardware, geometry, touch-map version/generation, transport, BPM, step, bar, beat, subdivision, pass, four track hit counts, replacement flags and remaining steps, timing lateness, heap, PSRAM, stack, and the last named error.
- Domain-test 16-bar wrap, 256-step indexing, nearest-step quantization at both sides of a boundary and exact ties, pause/resume phase, tempo estimation, invalid tempo taps, independent replacement, duplicate-hit collapse, reset, and scheduling across timer rollover.
- Prove one track can be replaced without changing the other three and that two different tracks can be replaced during the same pass.
- Over 100 injected pad touches, touch-to-audition p95 is at most 30 ms and no touch exceeds 50 ms.
- Over 16 complete bars, no scheduled hit begins more than 3 ms late under normal UI load; report rather than hide overruns.
- Run 100 loop passes and repeated clear/rebuild cycles without memory loss or changes to the saved touch map.
- Capture paused, playing, stored-pattern, and simultaneous-replacement frames.
- Physically review sound balance, thumb/index ergonomics, tap-tempo feel, live response, rhythmic alignment, clipping, touch alignment, and the bottom edge.

Record host-contract, compile, injected-device, framebuffer, and physical evidence separately.
Do not describe injected touches or scheduled timestamps as physical rhythm or listening proof.
```

## What this adds

The Sound Effects Board plays isolated clips.
The Drum Looper adds musical time, quantization, independent tracks, and a replacement gesture that needs no record screen or arm button.
The player keeps performing on the pads while each instrument independently captures one complete 16-bar replacement.
