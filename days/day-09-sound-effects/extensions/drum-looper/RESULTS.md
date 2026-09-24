# Drum Looper contract results

The generalized state suite generated three disposable reducer shapes from `tests/contract.json`.
Each passed 331,172 assertions covering defaults, bounds, deterministic nonmutation, play/pause/resume, valid and stale tap-tempo input, independently anchored 256-step replacement windows, same-step hit collapse, paused audition, loop wrap, replacement completion and restart, reset, Stopwatch metadata, round-screen layout, and 10,000 generated actions.
A fourth reducer with its first transition removed failed as required.

The reference frame was rendered at 468 × 466 from the declared paused state.
It defines visual intent only.

A disposable candidate then passed sanitizer-backed domain checks for bitsets, replacement, quantization ties, tempo estimation, deterministic PCM, non-silence, and the 256 KiB audio limit.
The pinned Arduino ESP32 3.3.10, M5Unified 0.2.19, M5GFX 0.2.26, and LVGL 9.3.0 toolchain compiled it for ESP32-S3 with OPI PSRAM.
The application used 917,115 bytes of its 3,145,728-byte slot and 31,656 bytes of static RAM.

An app-partition-only update loaded the candidate onto the attached M5Stack Stopwatch without replacing NVS.
The device retained shared touch map version 2, generation 2, and reported the required 468 × 466 geometry.
Injected-device checks established:

- independent Kick and Snare replacement across a complete 256-step window;
- paused audition without pattern edits and whole-track override on the next playing tap;
- tap-tempo update and reset preserving the chosen BPM;
- 100 live audio starts at 30 µs p95 and 888 µs maximum request-to-accepted latency;
- one real-time 16-bar pass with no step reported more than 3 ms late;
- 100 accelerated loop passes with stable heap and PSRAM; and
- clean empty, stored-pattern, and hand-review framebuffers with the header, pads, footer, and bottom edge intact.

The final candidate was restarted to an empty paused loop at 120 BPM.
The retained framebuffer is published as the device-candidate evidence; the reference render remains separate.
Physical pusher feel, physical touch alignment, audible balance, groove, and simultaneous-hit quality still require the hand review.
