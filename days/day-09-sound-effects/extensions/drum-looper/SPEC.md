# Drum Looper behavior contract

## Objective

Build a four-pad, four-track drum looper on the M5Stack Stopwatch.
The loop is 4/4, four measures (16 quarter notes total), and quantized to sixteenth notes.
Kick, Snare, Hi-Hat, and Crash are recorded and replaced independently without a dedicated record mode.

## Shared platform contract

Use the M5Stack Stopwatch at 468 × 466, rotation 0, display offset `(6,0)`, with rows 0–465 as the complete drawable area.
BtnA is physical left and BtnB is physical right with the lanyard down.
Load and validate `espt-touch/record` before touch input and apply its complete warp to every point.
Only the shared calibration flow may write that namespace.

## Localized controls

- **Thumb · BtnA/left:** tap tempo.
- **Index · BtnB/right:** toggle play and pause.
- **Both held 600 ms:** clear all tracks, stop, return to step 0, and preserve tempo.
- **Pad touch while playing:** audition immediately and record into that pad's current replacement pass.
- **Pad touch while paused:** audition only.
- **Both held during boot:** enter shared touch calibration.

A recognized both-button hold consumes both individual clicks.
A short both-button chord has no runtime action.

## Musical grid

The meter is fixed at 4/4.
The loop contains exactly four measures, four beats per measure, and four sixteenth subdivisions per beat.
There are 16 steps per measure and 64 steps per loop, indexed 0–63.

For step `s`:

- bar is `floor(s / 16) + 1`;
- beat is `floor((s mod 16) / 4) + 1`; and
- subdivision is `(s mod 4) + 1`.

Each track is a 64-bit set.
Memory and runtime must remain bounded regardless of performance length.

## Portable state

The normative fields and exact scenarios live in `tests/contract.json`.
Portable state includes transport, BPM, step, pass, four hit counts, four replacement flags, four replacement-step counters, last pad, total live taps, and a bounded error enum.

The real portable core additionally owns the four 64-bit patterns, the absolute transport anchor, fractional paused phase, tap-tempo interval window, and deterministic quantization helpers.
Reducers never mutate their input.

## Transport and scheduling

Default tempo is 120 BPM; valid tempo is 60–200 BPM.
One sixteenth-note duration is `60,000,000 / (BPM × 4)` microseconds using integer rational arithmetic rather than a repeatedly rounded millisecond delay.

Schedule from an absolute anchor and step index.
When the main loop wakes late, dispatch every due step in order, count and report lateness, and advance to the correct absolute deadline without stretching later steps.
Handle the platform timer's unsigned rollover with wrap-safe differences.

Play starts or resumes from the retained step and fractional phase.
Pause freezes both.
The first-ever play begins at step 0.
Advancing from step 63 wraps to step 0 and increments the pass counter.
It does not end a replacement merely because global step 0 was crossed.
Each track ends independently after 64 musical steps have elapsed from its own replacement start.

If two or more tracks contain a hit on one scheduled step, start them on that same boundary through a bounded mixer or independent prepared channels.
Track overlap must not serialize or drop a hit.

## Audible grid

While the transport plays, sound a short click on steps 0, 4, 8, and 12 of every bar.
Use a distinctly higher click on step 0 so beat one is recognizable without looking at the display.
Clicks stop immediately when paused and resume with the retained phase.
They use a prepared resident buffer and a dedicated mixer channel, so they can overlap a drum without delaying or replacing it.

At 120 BPM, 12 uninterrupted seconds must advance 96 sixteenth steps and schedule exactly 24 quarter-note clicks.
This remains true with USB connected but unread and with no serial monitor attached.

## Tap tempo

The first BtnA press arms tempo measurement without changing BPM.
Keep up to the four most recent valid intervals from a sequence of up to five taps.
Intervals from 300 through 1,000 ms are valid.
After at least one valid interval, set BPM to `round(60000 / median_interval_ms)` and clamp to 60–200.
For an even interval count, the median is the rounded arithmetic mean of the two center values.

An interval outside the valid range or a gap greater than 2,000 ms clears the interval window, treats the current press as a new first tap, and leaves BPM unchanged.
A valid change preserves the current step; the next boundary uses the new period.

## Per-track automatic replacement

Every touch-down starts the pad's resident drum sound immediately.
This live audition path does not wait for quantization, transport scheduling, release, synthesis, allocation, or storage.

While paused, no pattern changes.
While playing, quantize the touch timestamp to the nearest sixteenth boundary in the current 64-step loop.
An exact half-step tie resolves forward to the later step.
Quantization across the step-63 boundary may write step 0 of the next pass; that write belongs to the next pass and must use its replacement state.

For the selected track and quantized target tick:

1. If the track is not already replacing, clear all 64 old bits for that track only, mark it replacing, and set its exclusive end tick to `target_tick + 64`.
2. Set the quantized step bit.
3. If that bit was already set, keep one hit.
4. Leave every other track's pattern, hit count, and replacement flag unchanged.

Further taps on that track before its end tick add hits without clearing again.
Before scheduling the step at the exclusive end tick, mark that track stored; its recorded hit at the matching loop position can then play normally.
The next playing tap after completion begins another whole-track replacement.
Multiple tracks may be in replacement state simultaneously.

## Reset

Holding both pushers for 600 ms stops transport, clears all 1,024 pattern bits, clears all replacement flags and tap history, resets step and fractional phase to zero, and clears counters related to the loop contents.
It preserves the current BPM and the saved touch map.

## Sound contract

Prepare Kick, Snare, Hi-Hat, and Crash as deterministic 22,050 Hz signed 16-bit mono PCM before enabling input.
Keep every final buffer resident.
Each clip is at most 750 ms and the total PCM budget is at most 256 KiB.

- Kick: short low sine sweep with a transient click.
- Snare: explicitly seeded filtered noise plus a short tonal body.
- Hi-Hat: explicitly seeded high-passed noise with a fast decay.
- Crash: explicitly seeded metallic/noise blend with a longer decay.

Domain tests verify deterministic hashes, declared seeds, non-silence, sample bounds, duration, and memory.
The touch callback performs no synthesis, decoding, allocation, file access, or long logging.

## Visual contract

Reuse the four Day 09 pad bounds:

- `(84,88,132,118)` Kick;
- `(252,88,132,118)` Snare;
- `(84,246,132,118)` Hi-Hat; and
- `(252,246,132,118)` Crash.

Show `DRUM LOOPER` above the pads, followed by BPM, PLAYING or PAUSED, and `BAR nn / 4` with beat and subdivision.
Keep pad labels small and directly underneath their color fields.
A compact dot beside each label is dim for empty, white for a stored pattern, and red during that track's independent 64-step replacement window.
Flash the pressed pad briefly without moving or resizing it.

The footer shows `A TEMPO`, `B PLAY` or `B PAUSE`, and `HOLD BOTH: CLEAR`.
All content and pressed/focus states remain inside the radius-226 safe area.
No content uses rows 466–467 and no bottom bar remains visible.

## Diagnostics and failure behavior

Prefix serial lines with `D09L_`.
Provide `status`, `reset`, `action`, `advance`, `capture`, `touchlog`, and `calibrate`.
Status reports board, geometry, touch-map version/generation, transport, BPM, step, bar, beat, subdivision, pass, hit counts, replacement flags and remaining steps, last pad, live taps, late-step count/max lateness, heap, PSRAM, stack, and last error.

Ticks, metronome clicks, pad touches, and scheduled hits emit no unsolicited serial lines.
Diagnostic command responses are bounded and must not make musical time depend on whether a host is reading USB.
A transport delay larger than the bounded catch-up window reports `transport_overrun`; it never silently stretches later steps.

A missing sound disables only that pad and names it `sound_prepare`.
An impossible transport state stops playback and reports `transport_state` without corrupting patterns.
The `espt-touch` namespace remains read-only outside calibration.

## Required scenarios

- play, pause, and resume without losing loop position;
- valid and invalid tap-tempo sequences;
- first Kick tap clears only the old Kick pattern;
- later Kick taps in the same pass add without clearing;
- first Snare tap independently clears only Snare while Kick is already replacing;
- paused pad audition changes no pattern;
- duplicate hits on one quantized step collapse;
- step 63 wraps to step 0 without truncating a track's independent 64-step replacement window;
- both-button hold clears every track but preserves BPM; and
- simultaneous scheduled track hits share one musical boundary.
- an empty 120 BPM loop produces 24 clicks and 96 sixteenth steps in 12 seconds; and
- unread or disconnected USB does not slow touch, audio, rendering, or transport.

## Acceptance layers

1. **Host contract:** three reducer shapes pass exact scenarios and a deliberate mutation fails.
2. **Domain tests:** bitsets, quantization, tempo estimation, scheduling, rollover, mixer dispatch, deterministic PCM, and memory bounds pass.
3. **Compile evidence:** pinned Arduino ESP32, M5Unified, M5GFX, and LVGL compile the target adapter without project warnings.
4. **Injected-device evidence:** the real UI, touch hit testing, transport, mixer, capture, 100-touch latency run, 100 loop passes, and memory checks pass while calibration remains unchanged.
5. **Physical evidence:** a person judges sound balance, thumb/index controls, tap-tempo feel, rhythmic alignment, touch, clipping, and the round display.

Each layer records only what it directly observes.
