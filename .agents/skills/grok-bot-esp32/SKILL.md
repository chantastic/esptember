---
name: grok-bot-esp32
description: "Design, inspect, and refine Grok Bot-style procedural avatars on ESP32, especially the M5Stack Stopwatch Pi Pet. Use for eye geometry, lifecycle animation, frame-by-frame fidelity review, embedded rendering budgets, and Pi-session integration."
---

# Grok Bot ESP32

Build on the existing Pi Pet and use the saved reference material to avoid repeating the reverse engineering. The goal is recognizable agent state and convincing character motion on the actual display.

This is the repository copy at `.agents/skills/grok-bot-esp32/`. Its references, source extracts, review images, scripts, and browser study are bundled alongside this file. Resolve project paths from the ESPtember repository root.

## Start with the current evidence

Read [current-status.md](references/current-status.md) first. Its opening section states what the device candidate does now and what remains unverified. The rest is dated history. [initial-findings-2026-09-24.md](references/initial-findings-2026-09-24.md) is the first research snapshot: useful for hardware accounting, protocol, and acceptance, but several of its findings have since been fixed.

The primary workspace is this ESPtember repository, with Pi Pet under `days/day-34-pi-pet/`. Read its current `AGENTS.md` and the applicable platform and lesson skills before changing its prompt, contract, tests, or disposable firmware. Re-read the current candidate (`.build/device-candidate/pi_pet/`) and `RESULTS.md` before acting on any finding here.

The lesson source is prompt-first. Design work does not by itself request firmware implementation, building, flashing, service deployment, or provider calls. Follow the user's actual authorized scope and the current repository instructions. Keep hardware acceptance distinct from browser and host checks.

## Choose the relevant resources

- **Visual fidelity or motion debugging:** start with [review-tools.md](references/review-tools.md). Capture the live web demo and the device at matching times, then compare channel by channel. Use [source-map.md](references/source-map.md) to find the needed algorithm in the saved upstream bundle and extracts; they are inspection material, not runnable modules. Saved web and device sequences are in `assets/review/`.
- **Embedded rendering, performance, or memory:** read [embedded-rendering.md](references/embedded-rendering.md). It gives the measured techniques, costs, and dead ends: tile classification, RAM-resident textures, contour SDF textures with exact lid inversion, part compositing, and ribbon drawing.
- **Done/Celebrate:** preserve the user's preferred Pi choreography: chained tilted-axis eye spins, four decaying hops, and orbit-ribbon lanes with depth ordering. See [Pi's Done excerpts](references/candidate/pi-done-excerpts.txt) and the current candidate. Do not replace it with a single bounce or confetti.
- **Thinking/Blocked:** read [glyph-morphs.md](references/glyph-morphs.md).
  - The retained head contracts into Thinking's center dot or drops into Blocked's lower dot.
  - Keep the staggered satellites, traveling pulse, tapered stem entrance and shake, eye half-turns, and retained geometry on exit.
  - A cross-dissolve is not equivalent.
  - The device implements these; its section in that file lists its adaptations.
- **Integration:** use the initial findings' Pi state mapping and acceptance sections together with the lesson's `SPEC.md`, then inspect the current firmware and host extension.
- **Interactive design study:** start from [the revised study](assets/grok-motion-study.html). It uses simulated events and independently implemented rendering; it is not a fidelity oracle. Copy it into the task's output location before editing. [The standalone preview](assets/grok-motion-preview.html) opens locally.
- **Future speech:** read [future-voice.md](references/future-voice.md) only when voice is requested. Preserve the current USB protocol for visualization work.
- **Provenance:** use [source-map.md](references/source-map.md) and [resource-manifest.json](references/resource-manifest.json). Original `/tmp/grokbot` paths are provenance, not dependencies.

## Preserve the important distinctions

1. **Reference behavior, current Pi behavior, and proposed adaptations are different.** The article's demo is procedural SVG. Its showcased states are Idle, Thinking, Working, Waiting, Blocked, and Done. Pi adds embedded controls, sleep, a Thinking entry debounce, and a black background. Label any new choreography or timing as a proposal or adaptation.
2. **Eyes: contours at rest, capsules while morphing.**
   - The reference draws a settled expression's true 48-point contour, but interpolates capsule fits during expression morphs (`eX`/`eJ`/`eq`). A capsule-only renderer therefore loses curvature at rest, while a contour-only morph does not match the reference either.
   - Blinks squash the contour points along a lid axis, so pills close into thin dashes.
   - On the ESP32-S3, the exact polygon SDF was measured too slow. Per-contour SDF textures with exact lid inversion run at about 30 fps.
3. **Geometry comes before arbitrary spring tuning.** Compare face profiles, working-eye preprocessing, waiting's fixed expression and stroke, lid axis, morph interpolation, and complete-eye confinement. Frequency 13 with damping 1 is the reference gaze spring; faster glances are an optional adaptation.
4. **Completion has a choreography.** State completion, celebration animation, and unseen acknowledgment are independent. Preserve lifecycle timing unless the user requests a behavior change.
5. **Use current board geometry.** The ESPtember contract uses 468 × 466, rotation 0, x offset 6. Preserve the device-owned `espt-touch/record` and the shared calibration flow. Flash app-only; a merged image at `0x0` can erase calibration.
6. **Keep the current integration.** Pi extension → local hub → USB serial already exists. Preserve the session roster, redaction, reconnect replay, and the `idle|thinking|tool|error|done|waiting|sleeping` wire states. Acknowledge is not tool approval. Audio and provider credentials do not belong in display messages.
7. **The design is xAI's.** Head shapes, expressions, and motion constants come from xAI's public page. Candidates derive them at build time into ignored `.build/`; do not commit derived assets, and flag this before any public release.

## Review workflow

- **Use matched, reproducible sequences.** Use a fixed clock and seed where possible. Inspect entry, 70, 150, 300, 480, and 1,000 ms, and a complete multi-second cycle. Compare eye shape, gaze, blink, body movement, spin visibility, ribbon depth, and glyph geometry separately.
- **Stills are not enough.** A single screenshot or a successful state selector does not establish animation fidelity.
- **Check the hard cases:** transitions interrupted by a new state, direct Thinking ↔ Blocked, and the narrowest head with off-center eyes.
- **Profile separately.** Measure render and display transfer separately with the firmware's `GB_FPS` breakdown, and never while a blocking `shot` dump is running. 30 fps is the design budget.
- **Keep evidence layers separate:** source reading, compile, injected-device tests, device recordings, and physical review. Physical review of the latest transitions, contour eyes, black level, and tilt direction is still outstanding; check `RESULTS.md` for today's status.

## Handoff and communication

[pi-handoff.md](references/pi-handoff.md) is the September 24 handoff the user carried to Pi, with its outcome recorded. Reuse its structure for future handoffs after updating it for current source. Do not imply a message was delivered just because a handoff file exists.
