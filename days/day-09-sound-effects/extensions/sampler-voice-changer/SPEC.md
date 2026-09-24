# Sampler + Voice Changer behavior contract

## Objective

Capture one short microphone sample on the M5Stack Stopwatch and play eight deterministic transformations of that sample from the established two-page drum-pad interface.
Recording and effect preparation may be deliberate, visible phases.
Once the pads appear, every effect is resident and responds immediately.

## Shared platform contract

Use the M5Stack Stopwatch at 468 × 466, rotation 0, display offset `(6,0)`, with rows 0–465 as the complete drawable area.
BtnA is physical left and BtnB is physical right with the lanyard down.
Load and validate the device-owned `espt-touch/record` before the touch consumer and apply its full warp to every point.
Only the shared calibration flow may write that namespace.

Use the internal ES8311 microphone and internal speaker through M5Unified.
Capture and playback are half-duplex: stop and mute the speaker before starting the microphone, then stop the microphone before preparing effects or starting playback.
Never route live microphone data to the speaker.

## Localized controls

### Recorder

- Touch the large red record target to start the countdown.
- Touch during the countdown to cancel.
- During recording, a touch-down, BtnA/left, or BtnB/right stops the take.
- Recording also stops automatically at three seconds.

### Pads

- BtnA/left selects the previous page and wraps.
- BtnB/right selects the next page and wraps.
- A pad fires on touch-down.
- Holding both pushers for 600 ms stops playback, securely clears the current take and its effects, and returns to the recorder.
- Holding both pushers during boot enters shared touch calibration.

Consume the individual pusher clicks when a runtime hold-both gesture is recognized.
Simultaneous pusher edges must not cause two page changes.

## Portable state

The normative fields, values, defaults, scenarios, and exact intermediate states are in `tests/contract.json`.
The state contains:

- `screen`: recorder, countdown, recording, processing, or pads;
- `page`: page1 or page2;
- `sample`: empty, captured, ready, or invalid;
- `recorded_samples`: 0–66,150 at 22,050 Hz;
- `active` and `last_trigger`: none or one of eight effect identifiers;
- bounded `takes` and `plays` counters; and
- a closed `error` enum.

State reduction is deterministic and never mutates its input.
Audio buffers, waveform pixels, clocks, and adapter handles are outside portable state.

## Recording contract

Capture mono signed 16-bit PCM at exactly 22,050 Hz.
Allocate the maximum 66,150-sample capture buffer in PSRAM before countdown completion.
Do not allocate in the microphone callback.

The countdown is `3`, `2`, `1`, each visible for 400 ms, followed by recording.
The stopping input is not part of the sample.
Accept durations from 6,615 through 66,150 samples inclusive, corresponding to 300 ms through 3 seconds.

Analyze raw samples before normalization:

- reject if capture setup or transfer failed;
- reject if duration is outside the accepted interval;
- subtract the signed arithmetic mean to remove DC;
- compute peak magnitude and 64-bit RMS energy after DC removal;
- reject as `too_quiet` when peak is below 512 or RMS is below 128;
- reject as `too_clipped` when at least 1% of raw samples have magnitude 32,760 or greater.

For an accepted take:

1. subtract DC with signed 32-bit intermediates;
2. apply a 5 ms linear fade-in and 10 ms linear fade-out, shortening each fade proportionally if necessary;
3. choose gain targeting peak magnitude 23,197 (-3 dBFS), capped at 8.0×; and
4. round deterministically and saturate to `[-32768, 32767]`.

The normalized capture becomes the sole source for all effects.
Never modify it while deriving an effect.

## Effect contract

All effects operate on normalized 22,050 Hz signed 16-bit mono PCM.
Represent gains and recursive coefficients as Q1.15 integers, use a checked-in 1,024-entry Q1.15 sine table with a 32-bit phase accumulator, keep products in signed 64-bit intermediates, and round divisions to nearest with ties away from zero.
Use integer numerators and denominators for resampling positions.
Saturate only at each declared output or recursive-filter step.
Do not use unseeded noise.
Cap every output at 88,200 samples, or four seconds.

| Pad | Transform | Exact rule |
| --- | --- | --- |
| Clean | Identity | Copy every normalized input sample. |
| Chipmunk | Pitch/speed up | Output length `ceil(N × 2 / 3)`; read source position `i × 3 / 2` with linear interpolation. |
| Monster | Pitch/speed down | Output length `min(88200, ceil(N × 3 / 2))`; read `i × 2 / 3`, then apply `y[n] = 0.18x[n] + 0.82y[n-1]`. |
| Robot | Ring modulation | Multiply each input by `sin(2π × 70 × n / 22050)` and gain 0.85. |
| Echo | Two-tap delay | Output length `min(88200, N + 5733)`; mix dry plus sample `n-2867` at 0.50 and `n-5733` at 0.25. |
| Reverse | Reversal | `y[n] = x[N-1-n]`. |
| Stutter | Slice repeat | Copy up to the first 6,615 samples; extract a 1,985-sample slice centered at `floor(0.40N)`, repeat it six times, then append up to the final 6,615 samples without exceeding 88,200. Clamp the slice inside the source. |
| Alien | Delay/ring modulation | Keep length N. Read a linearly interpolated source delayed by `18 × (1 + sin(2π × 6 × n / 22050)) / 2` samples, using zero before the source begins. Mix 0.55 dry plus 0.45 delayed multiplied by `sin(2π × 43 × n / 22050)`. |

Preparation is atomic.
Keep the pad screen unavailable until all eight buffers and metadata have completed.
If any allocation or transform fails, zero all partial derivatives, retain no playable partial set, show `effect_memory`, and return to the recorder.

The capture plus all effects must consume at most 2 MiB.
A maximum-length valid take must finish all eight transforms within two seconds on the target.

## Playback contract

The pad pages reuse Day 09's positions:

- `(84,88,132,118)`
- `(252,88,132,118)`
- `(84,246,132,118)`
- `(252,246,132,118)`

Page 1 is Clean, Chipmunk, Monster, Robot.
Page 2 is Echo, Reverse, Stutter, Alien.
Each pad is a colored square with a small label directly underneath.
The footer reads `A ◀`, the page count, and `▶ B`.

A press updates portable state and immediately asks speaker channel 0 to stop/restart from the prepared PCM.
There is one voice and no pending queue.
A repeated pad press restarts the effect at sample zero.
No effect processing, resampling, allocation, microphone access, storage access, release wait, or long serial write occurs in the pad callback.

Across 100 injected LVGL touch-downs, request-to-speaker-start p95 is at most 30 ms and the maximum is at most 50 ms.
Page state changes within 16 ms.

## Visual states

- **Recorder:** black background, `MAKE A SAMPLE`, a central red `TAP TO RECORD` circle, and `Up to 3 seconds`.
- **Countdown:** black background and one large numeral. No pad or record target remains active.
- **Recording:** black background, red dot, `RECORDING`, elapsed time, bounded waveform, and `Tap or press a pusher to stop`.
- **Processing:** black background, `MAKING EFFECTS`, effect count, and bounded progress. Inputs cannot trigger incomplete audio.
- **Pads:** the two round-safe pad pages and pager footer.
- **Error:** a short named reason and one large `TRY AGAIN` target that returns to the recorder.

All content remains within the radius-226 safe area centered at `(234,233)`.
The display has no unused bottom strip.

## Privacy and persistence

Voice audio is session-only.
Do not write raw or processed PCM to NVS, Preferences, flash, SD, network, serial logs, crash reports, or retained reset memory.
Store it only in volatile PSRAM.

Before rerecording, before returning from an error with captured data, and during normal teardown, overwrite the used capture and effect ranges with a non-optimizable clearing routine before freeing or reusing them.
A reboot starts with `sample=empty` and the recorder screen.

The only persistent data this exercise reads is the shared touch map.

## Diagnostics

Prefix serial protocol lines with `D09S_`.
Support `status`, `reset`, `action`, `capture`, and `touchlog`.
Status includes board, geometry, touch-map version/generation, screen, page, sample state, recorded sample count, takes, plays, active/last effect, capture/effect bytes, free heap/PSRAM, stack headroom, capture duration, processing duration, playback latency summary, and last named error.

Never stream user-recorded PCM through diagnostics.
For deterministic device automation, a test-only action may generate an internal fixture or feed known synthetic PCM into the same portable validation and effect pipeline.
Label that evidence synthetic.

## Required scenarios

The JSON contract makes these exact:

- successful countdown, capture, processing, and pad entry;
- safe countdown cancellation;
- quiet-take rejection and retry;
- left/right page wrapping;
- all four effects on each page;
- immediate replacement and same-pad retrigger; and
- rerecord clearing all derived state.

## Acceptance layers

1. **Host contract:** three reducer shapes pass the state scenarios and a deliberate mutation fails.
2. **Domain tests:** fixtures prove thresholds, DC removal, fades, normalization, interpolation, lengths, saturation, hashes, buffer bounds, memory budget, and all eight effects.
3. **Compile evidence:** pinned Arduino ESP32, M5Unified, M5GFX, and LVGL accept the target adapter without project warnings.
4. **Injected-device evidence:** a synthetic take traverses the real processing, widgets, PSRAM, speaker starts, captures, timings, and 20 clear/rebuild cycles while calibration remains unchanged.
5. **Physical evidence:** a person records a voice, listens to all eight variants, checks feedback avoidance and stopping behavior, and reviews the display and controls.

Each layer records only what it directly observes.
