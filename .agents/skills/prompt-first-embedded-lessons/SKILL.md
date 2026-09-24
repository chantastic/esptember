---
name: prompt-first-embedded-lessons
description: Convert embedded-device projects into durable human-readable prompts, behavior contracts, generalized portable tests, layered acceptance evidence, and hand-review checklists while treating generated firmware as disposable.
---

# Build prompt-first embedded lessons

Use this skill when an ESPtember lesson or another embedded example should be reproducible from a specification instead of maintained as one privileged firmware implementation.

Read the repository `AGENTS.md` and any board-specific skill first.
For M5Stack Stopwatch lessons, `.agents/skills/build-stopwatch-lessons/SKILL.md` supplies the hardware, control, touch, and display rules.

## Extract the contract from history

Treat the existing implementation, logs, screenshots, and writeup as evidence.
Extract observable behavior, state, inputs, outputs, timing, bounds, failure behavior, persistence, and hardware assumptions.
Do not preserve a private implementation detail unless it explains a required behavior or a measured constraint.

Keep activity-specific decisions in that lesson's `SPEC.md`.
Move a decision into a shared skill or shared test only after it applies to at least two lessons or protects a project-wide invariant.

## Create four durable sources

Each prompt-first lesson contains:

1. `README.md` with the assignment, a copyable build prompt, a reference frame, required behavior, and scoped evidence.
2. `SPEC.md` with exact state, actions, limits, failure behavior, persistence, layout, and acceptance layers.
3. `tests/contract.json` plus a portable runner that can evaluate multiple disposable state cores.
4. `HAND-REVIEW.md` for physical observations and a place to record a reusable decision.

Add `TESTING.md` to explain what the suite proves and `RESULTS.md` to record actual runs.
Generated application code, build output, credentials, and device-local state are not durable lesson source.

## Make the portable contract useful

Define every state field with a type, default, and range or enum set.
Name semantic actions independently from GPIOs and UI libraries.
Write scenarios with expected intermediate state after every action, including cancellation, wrap, retry, stale data, malformed input, and boundary values where applicable.

The shared runner must check:

- exact defaults and valid state after every transition,
- deterministic output without mutating the input state,
- required scenarios against at least three reducer shapes,
- long generated action sequences,
- board and control metadata,
- layout and focus bounds,
- and a deliberate mutation that must fail.

Add domain tests beside the generalized contract for numerical algorithms, clocks, parsers, packet formats, storage records, audio pipelines, or sensor filters.
A generic state scenario does not replace those checks.

## Keep evidence layers separate

Use these labels precisely:

- **Reference frame:** deterministic acceptance-model rendering. It expresses visual intent only.
- **Host contract:** portable reducer and domain tests.
- **Compile evidence:** real libraries and target toolchain accept a generated adapter.
- **Injected-device evidence:** the real app, widgets, buffers, storage adapters, and captures respond to scripted input.
- **Physical evidence:** a person operated the actual controls, touch, sensors, speaker, motor, radios, or network flow.

Never promote one layer into another in prose.
Publish reference frames in the post, then replace or supplement them with device framebuffer captures when the generated candidate reaches that layer.

## Use hand review as feedback

Prepare the device in a deterministic initial state and leave the reviewer one short path through the important states.
The checklist covers physical control direction, touch across the face, clipping, bottom-edge artifacts, cues, readability, recovery, and the lesson's activity-specific behavior.

When review finds a problem:

1. decide whether it is lesson-specific, Stopwatch-wide, or prompt-first-process-wide;
2. change the narrowest durable source that owns the decision;
3. add an observable contract or acceptance check when possible;
4. regenerate a disposable candidate; and
5. record only the layer that was rerun.

## Commit in reviewable groups

Commit and push completed lesson groups after contracts, reference frames, and site checks pass.
Do not include unrelated drafts, credentials, generated candidates, or build directories.
