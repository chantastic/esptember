# Day 13 contract results

The generalized suite was run against three disposable reducer shapes generated from `tests/contract.json`.
All three passed 120,195 state, scenario, determinism, layout, hardware, control, and generated-sequence assertions.
A fourth candidate with its first transition removed failed as required.

The retained domain suite also passed against the previous implementation as a disposable candidate: 98 button-grammar assertions, all 27 workout/session timing checks, progress ring/reset/corruption checks, and exact summaries for all 27 workouts.

The reference frame was rendered from the contract's declared visual scenario and checked at 468 × 466.
It is a design oracle, not device proof.

Real-library compilation, an instrumented device candidate, framebuffer comparison, and physical hand review remain separate acceptance layers.
