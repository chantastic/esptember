# Follow-on exercise: Trap Looper

## The assignment

Turn the four-pad Day 09 instrument into a four-measure drum looper: 4/4 with 16 quarter notes total.
The trap palette is Kick, Snare, an automatic Hi-Hat, and Triple Hi-Hat.
The regular Hi-Hat pad toggles a steady eighth-note track ON or OFF, avoiding the need to finger every hat.
One Triple Hi-Hat event produces a tight, tempo-locked three-stroke roll while occupying one recorded step.

Start playback, then tap Kick, Snare, or 3X Hi-Hat to replace that instrument's track.
The first tap clears only that event track's old pattern; every tap during the next full four measures becomes its replacement pattern.
The other three tracks keep playing untouched.

There is no record mode.
While the transport is playing, pad taps are heard immediately and recorded automatically.
Playback also supplies an audible quarter-note click, with a higher accent on beat one, so the player can perform against the grid before any track exists.

## Reference frame

![Round-screen acceptance reference for the Trap Looper](https://esptember.com/images/day-09-sound-effects/drum-looper-reference.png)

The reference keeps Day 09's large four-pad view and adds only tempo, transport, bar position, and compact track-state cues.
It defines visual intent rather than device proof.

## Controls

Hold the Stopwatch with the lanyard down:

- **Thumb · BtnA/left:** tap tempo.
- **Index · BtnB/right:** play or pause.
- **Hold both for 600 ms:** clear every track, stop, and return to bar 1 without changing the tempo.
- **Touch Kick, Snare, or 3X Hi-Hat while playing:** hear it immediately and write it into that pad's replacement track.
- **Touch Kick, Snare, or 3X Hi-Hat while paused:** audition it without changing the loop.
- **Touch Hi-Hat:** toggle its automatic eighth notes ON or OFF in either transport state.

## The build prompt

```text
Build the Trap Looper follow-on exercise for ESPtember Day 09 on the M5Stack Stopwatch Dev Kit (C152).

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

- Use one fixed 4/4 loop containing four measures and 16 quarter notes total.
- Quantize each track to sixteenth notes: 16 steps per measure, 64 steps per loop.
- Keep three independent 64-bit event tracks named Kick, Snare, and Triple Hi-Hat, plus one Boolean regular Hi-Hat track.
- Tapping the regular Hi-Hat toggles it ON or OFF in either transport state. When ON, schedule one regular closed-hat hit on every even sixteenth step—steps 0, 2, 4, …, 62—for exactly 32 eighth notes per loop. Turning it OFF suppresses future hits immediately.
- Do not open a replacement window or write pattern bits for the regular Hi-Hat. When turned ON while playing, begin at the next eighth-note boundary; when turned ON while paused, play one immediate preview and wait for transport before continuing.
- Use resident deterministic PCM for all four drum sounds and one immediate-retrigger speaker voice per sound or a bounded mixer that can start coincident hits without blocking the UI.
- Treat each Triple Hi-Hat bit as one compound event. Start its first closed-hat stroke immediately, then start strokes two and three at one-third and two-thirds of the current sixteenth-note duration. Derive all three deadlines from the event timestamp so rounding and a late loop cannot accumulate drift.
- Record and replace the Triple Hi-Hat as one event per quantized step. Do not write three pattern bits or delay its first live stroke. Use separate prepared mixer voices for the three strokes so their short tails can overlap.
- Keep a short resident metronome click on its own mixer channel. Sound it on every quarter note while playing, with a clearly higher accent on beat one of each bar. Do not click while paused.
- Schedule from an absolute transport anchor. Never advance musical time by repeatedly delaying for one step.
- At loop wrap, increment the pass counter. Each event track's replacement window continues until 64 steps have elapsed from that track's own first replacement tap.

Automatic per-track replacement

- Kick, Snare, and 3X Hi-Hat always audition immediately on touch-down.
- While paused, audition only; do not change any track.
- While playing, quantize the touch timestamp to the nearest sixteenth step with a deterministic tie rule.
- The first touch of an event pad when that track is not already replacing clears all 64 old steps for only that pad, opens a 64-step replacement window from the quantized hit, and writes the new hit.
- Later touches of the same pad while its replacement window remains active add hits without clearing again.
- The first touch of another pad independently clears and replaces only that other track.
- When one track's 64-step window ends, its pattern becomes stored. Its next playing tap starts another whole-track replacement.
- Multiple touches quantized to the same track and step store one hit.
- The live audition is not delayed to the quantized boundary. Quantization controls future playback only.

Physical controls

- BtnA/left is tap tempo. The first tap arms measurement. From two through five valid taps, set tempo from the median of recent intervals.
- Accept intervals from 300 through 1,000 ms, corresponding to 200 through 60 BPM. An invalid interval or a gap over two seconds restarts the tap sequence without changing tempo.
- Clamp tempo to 60–200 BPM. Default to 120 BPM.
- A valid tempo update keeps the current step and uses the new period for the next boundary.
- BtnB/right toggles play and pause. Pause retains the current step and fractional phase; resume continues there.
- Hold both pushers for 600 ms at runtime to stop, clear the three event tracks, turn the automatic Hi-Hat OFF, clear replacement flags, and return to step 0. Preserve the current tempo.
- Consume individual button clicks when a two-button hold is recognized. Ignore a short two-button chord.

Layout

- Keep four 132 × 118 colored pads at the Day 09 positions with small labels underneath: KICK, SNARE, HI-HAT OFF or HI-HAT ON, and 3X HI-HAT.
- Show TRAP LOOPER, current BPM, PLAYING or PAUSED, and BAR 1–4 with beat/subdivision progress above the pads.
- A compact dot by each event-track label is dim for empty, white for a stored pattern, and red while replacing. The regular Hi-Hat dot is dim when OFF and white when ON; it never turns red.
- Flash only the touched pad briefly. Do not animate the grid or delay its sound.
- The footer shows A TEMPO, B PLAY or B PAUSE, and HOLD BOTH: CLEAR.
- Keep every focus/pressed state inside the radius-226 safe area and leave no bottom strip.

Diagnostics and acceptance

- Prefix serial lines with D09L_ and implement status, reset, action, advance, capture, touchlog, and calibrate commands.
- Report hardware, geometry, touch-map version/generation, transport, BPM, step, bar, beat, subdivision, pass, the regular Hi-Hat ON/OFF state, three event-track hit counts, replacement flags and remaining steps, timing lateness, Triple Hi-Hat burst/stroke counts and maximum stroke lateness, heap, PSRAM, stack, and the last named error.
- Keep the performance path independent from USB readership. Do not print a line for each tick, click, pad touch, or scheduled drum hit. Command responses and bounded error summaries must not stall touch, rendering, audio, or the musical clock when no serial monitor is attached.
- Domain-test four-measure wrap, 64-step indexing, nearest-step quantization at both sides of a boundary and exact ties, pause/resume phase, tempo estimation, invalid tempo taps, independent replacement, duplicate-hit collapse, reset, and scheduling across timer rollover.
- Prove one Triple Hi-Hat event produces exactly three strokes at offsets 0, one-third, and two-thirds of a sixteenth note at 60, 120, and 200 BPM. Its first live stroke follows the same latency limit as every other pad, and scheduled follow-up strokes are no more than 3 ms late under normal UI load.
- Prove one track can be replaced without changing the other three and that two different tracks can be replaced during the same pass.
- Prove Hi-Hat ON schedules exactly 32 regular-hat hits across one 64-step loop, Hi-Hat OFF schedules none, toggling it changes no event-track pattern, and reset returns it to OFF.
- Over 100 injected pad touches, touch-to-audition p95 is at most 30 ms and no touch exceeds 50 ms.
- Over four complete loop passes (16 measures), no scheduled hit begins more than 3 ms late under normal UI load; report rather than hide overruns.
- With an empty loop at 120 BPM, 12 seconds of playback produces 24 audible quarter-note clicks and advances 96 sixteenth steps. Repeat with USB connected but unread and with no monitor attached; neither condition may slow the clock.
- Run 100 loop passes and repeated clear/rebuild cycles without memory loss or changes to the saved touch map.
- Capture paused, playing, stored-pattern, and simultaneous-replacement frames.
- Physically review sound balance, thumb/index ergonomics, tap-tempo feel, live response, rhythmic alignment, clipping, touch alignment, and the bottom edge.

Record host-contract, compile, injected-device, framebuffer, and physical evidence separately.
Do not describe injected touches or scheduled timestamps as physical rhythm or listening proof.
```

## What this adds

The Sound Effects Board plays isolated clips.
The Trap Looper adds musical time, quantization, independent tracks, and a replacement gesture that needs no record screen or arm button.
The player performs Kick, Snare, and 3X Hi-Hat into independent four-measure replacements while the regular Hi-Hat supplies optional eighth notes.
