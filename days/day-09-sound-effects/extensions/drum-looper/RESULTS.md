# Drum Looper contract results

The generalized state suite generated three disposable reducer shapes from `tests/contract.json`.
Each passed 331,172 assertions covering defaults, bounds, deterministic nonmutation, play/pause/resume, valid and stale tap-tempo input, independently anchored 256-step replacement windows, same-step hit collapse, paused audition, loop wrap, replacement completion and restart, reset, Stopwatch metadata, round-screen layout, and 10,000 generated actions.
A fourth reducer with its first transition removed failed as required.

The reference frame was rendered at 468 × 466 from the declared paused state.
It defines visual intent only.

No firmware was generated, compiled, or loaded for this specification pass.
Bitset math, timer scheduling, audio mixing, device latency, framebuffer capture, and physical rhythm/listening remain acceptance work for a future disposable candidate.
