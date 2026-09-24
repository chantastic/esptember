# Trap Looper contract results

The generalized state suite generated three disposable reducer shapes from `tests/contract.json`.
Each passed 351,511 assertions covering defaults, bounds, deterministic nonmutation, play/pause/resume, the audible-grid state, valid and stale tap-tempo input, independently anchored 64-step replacement windows, the single-event Triple Hi-Hat track, same-step hit collapse, paused audition, loop wrap, replacement completion and restart, reset, Stopwatch metadata, round-screen layout, and 10,000 generated actions.
A fourth reducer with its first transition removed failed as required.

The reference frame was rendered at 468 × 466 from the declared paused state.
It defines visual intent only.

A disposable candidate then passed sanitizer-backed domain checks for bitsets, replacement, quantization ties, tempo estimation, deterministic PCM, non-silence, the 256 KiB audio limit, and exact Triple Hi-Hat stroke offsets at 60, 120, and 200 BPM.
The pinned Arduino ESP32 3.3.10, M5Unified 0.2.19, M5GFX 0.2.26, and LVGL 9.3.0 toolchain compiled it for ESP32-S3 with OPI PSRAM.
The trap application used 917,571 bytes of its 3,145,728-byte slot and 31,704 bytes of static RAM.
Its four deterministic drum buffers occupy 32,852 bytes; the Triple Hi-Hat uses a brighter 75 ms resident clip.

An app-partition-only update loaded the candidate onto the attached M5Stack Stopwatch without replacing NVS.
The device retained shared touch map version 2, generation 2, and reported the required 468 × 466 geometry.
Injected-device checks established:

- independent Kick and Snare replacement across a complete 64-step window;
- paused audition without pattern edits and whole-track override on the next playing tap;
- one paused Triple Hi-Hat event starting immediately and completing exactly three strokes with zero late follow-up strokes;
- tap-tempo update and reset preserving the chosen BPM;
- 100 live audio starts at 37 µs p95 and 900 µs maximum request-to-accepted latency, including 25 Triple Hi-Hat bursts that produced all 75 strokes with zero late follow-ups;
- 120 BPM playback advancing exactly 96 sixteenth steps, completing one four-measure pass, and scheduling exactly 24 quarter-note clicks in 12 seconds, with zero late steps;
- one real-time four-measure pass at 148 BPM scheduling 18 clicks through its five-step observation tail, with zero late steps;
- 100 accelerated loop passes with stable heap and PSRAM; and
- clean empty, stored-pattern, and hand-review framebuffers with the header, pads, footer, and bottom edge intact.

The completed run restarted the candidate to an empty paused loop at 120 BPM.
The retained framebuffer is published as the device-candidate evidence; the reference render remains separate.
Physical pusher feel, physical touch alignment, audible balance, groove, and simultaneous-hit quality still require the hand review.

One physical touch occurred during the nominal empty-loop clock window and recorded a regular Hi-Hat event.
The transport and click counts remained exact, but this run is not presented as untouched empty-loop evidence.
The harness now requires the physical-touch count and all four patterns to remain unchanged during that check; its immediate rerun was stopped when active hand testing was detected rather than resetting the board underneath the player.

The first device candidate emitted serial diagnostics from the performance path and supplied no click track.
With no host reader, its transport fell as much as four seconds behind and reported `transport_overrun`.
The durable prompt, specification, contract, testing notes, and hand review now require a beat-one-accented quarter-note click and prohibit tick, click, pad, and scheduled-hit logging from the hot path.
The regenerated candidate passed the unread-USB clock check and the complete injected-device suite before hand review resumed.

The first candidate treated 16 as the measure count; the intended musical length was 16 quarter notes: four measures of 4/4.
The source prompt, specification, portable contract, reference frame, generated candidate, and device checks were all revised to make 64 sixteenth-note positions the single loop boundary.
