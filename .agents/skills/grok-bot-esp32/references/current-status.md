# Status and corrections — September 24, 2026

## Current state (latest; re-verify against the candidate)

The device candidate is `days/day-34-pi-pet/.build/device-candidate/pi_pet/`, revision 3 in the lesson's `RESULTS.md`. It is a port of the reference engine, not an imitation.

- **Roster and background:** eight roster heads with face fits, on a pure black background (the user's choice).
- **Eyes:** true contours when settled, via per-contour SDF textures; capsule interpolation during morphs, matching the reference; blink-through expression changes; lid squash into dashes; winks; eye confinement from squashed extents; eye-scale cap.
- **Motion:** per-state tables, springs at 1/120 s, the four-hop bounce chain, and tilted-axis eye spins with orbit-ribbon lanes.
- **Done:** the user's preferred choreography: chained spins, hops, and ribbons.
- **Thinking and Blocked:** retained-head transitions per [glyph-morphs.md](glyph-morphs.md), with part compositing. Thinking is debounced by 350 ms, a labeled adaptation.
- **Performance:** about 24–31 fps in every state, measured by the injected-device `GB_FPS` line.
- **Tests:** the host contract, the host bridge, and a 13-scenario device replay pass.

Still unverified or open:
- Physical review of the transitions, contour eyes, black-level smear, and ribbon look on the AMOLED.
- The IMU tilt direction.
- The shared runtime recalibration flow (the candidate loads the map but offers no recalibration).
- A possible Pi session-switch registration gap, never reproduced.
- Waiting (bored) versus Pi's "needs you" semantics, which is a product decision.

Techniques and measured costs are in [embedded-rendering.md](embedded-rendering.md); capture and comparison tools are in [review-tools.md](review-tools.md).

## History

Read this before the initial findings. The initial research was performed while another Pi process was changing the candidate. Findings about that earlier source must be revalidated against current files.

## User feedback and authority

The user reported that the browser study's Done state looked wrong and that Pi had done a good job. Preserve Pi's Done design as the preferred local baseline. The browser study is an exploratory comparison tool, not evidence that Pi's animation should be replaced.

The study originally omitted the actual eye-spin and ribbon choreography, used one sine bounce and falling confetti, and shrank the Done eyes using an alternate reference profile. That was an implementation simplification in the study, not a diagnosed ESP32 rendering failure. The revised study follows the current Pi candidate's larger eyes, four decaying hops, chained tilted-axis eye spins, and orbit ribbons drawn behind and in front of the body. It remains deterministic and simplified; it does not claim pixel-identical parity or hardware verification.

## Thinking and Blocked correction

The user subsequently identified the study's Thinking and Blocked transitions as cross-dissolves. Both shortcuts have been replaced using the saved upstream geometry. The head rounds and contracts into Thinking's center dot or drops into Blocked's lower dot. Thinking adds staggered outward-moving satellites and a traveling lift/size/brightness pulse. Blocked adds a separately growing tapered stem, downward entrance, and decaying shake. Eye half-turns and exit geometry are preserved, with spring state retained across selection changes.

Read [glyph-morphs.md](glyph-morphs.md) for source formulas, resources, and review evidence. The study uses separate outgoing/incoming pulse clocks for smooth interruptions; that is an explicit adaptation. Other study simplifications remain, and these browser corrections do not diagnose or change Pi's firmware.

## What changed since the first source read

The newer `pi_pet.ino` includes:

- `startSpin(2, ..., 9)` on Done entry, followed by two queued spins with newly chosen axes. Spin uses a critically damped frequency of 6.2 rad/s.
- Nine bounded orbit ribbon slots, parallel colored lanes, varied orbital planes, lifetimes around 2–2.6 seconds, and back/front drawing passes.
- Four parabolic hops: heights 48, 28, 14, 6 source units; durations .5, .382, .27, .177 seconds.
- Eye-center cylindrical rotation about a tilted axis, foreshortening, and back-facing eye suppression. Briefly hidden eyes during a spin can be intentional.
- Done eye scale and lid targets of 1.1.
- `changeExpr`/pending-expression logic rather than only the earlier direct `setExpr` entry.
- Confinement checks at four capsule extremes and inward correction. The initial finding that only the eye center was considered is no longer an accurate description of this newer code. Four-extreme checking still differs from the reference's full boundary/row-span method; assess visible behavior before changing it.

The bundled [Done source excerpts](candidate/pi-done-excerpts.txt) identify their source checksum and capture time. They document this read, not whatever Pi may subsequently implement.

## Keep these findings qualified

- The capsule representation genuinely drops contour information. *(Resolved later: settled eyes now render true contours; the reference itself uses capsules only mid-morph.)*
- Source gaze frequency 13/damping 1 is not a defect. Idle's zero gaze target does not mean the whole avatar is motionless; expressions, drift, and body motion contribute. The faster preset is a design alternative.
- The initial maintained SPEC and results described an older six-style/eight-mood version, while the candidate and reference model had moved on. Reconcile with the current owner; do not restore old implementation from prose alone.
- Source Waiting/Bored differs semantically from Pi's “needs user” state. An attentive variation is a proposed product decision, not an automatic correction.
- A possible session-switch registration issue was inferred from `session_start` and `connect()` code. It was not reproduced against Pi's actual session lifecycle.
- Full eye contours cost 9,600 bytes as int16 XY. The documented error figures are geometry calculations, frame-rate targets are budgets, and older device performance figures are reports. None establish current physical performance.

## Communication and changes

At skill creation, no findings or instructions had been sent to Pi. Research read its files/session; this work created a skill, a handoff, and a separate browser study. It did not modify Pi's candidate, connect to serial, build or flash hardware, deploy a gateway, or call a speech provider.

This is a dated status statement. After an authorized handoff, record the actual destination and delivery evidence separately.

Later on September 24, the user asked about handing the work to Pi. The handoff was refreshed with the approved Thinking/Blocked corrections and with acknowledgment of Pi's newer port. Direct delivery could not proceed: computer control of Ghostty was denied by the tool. The [prepared handoff](pi-handoff.md) remains undelivered; the user was given a paste-ready pointer. No Pi session message or log was written.

The user then requested the final skill in ESPtember and chose to pass the message to Pi themselves. The complete skill is now bundled at `.agents/skills/grok-bot-esp32/` in that repository, with the handoff pointing to this repository copy. Historical absolute paths in the provenance records remain capture evidence, not dependencies.

## Pi implementation after the handoff — September 24, 2026 (later)

Recorded separately from the snapshot above. After reading this handoff, Pi was authorized ("do the best you can") to implement the reported differences:

- Thinking/Blocked now use the retained-head formulas in [glyph-morphs.md](glyph-morphs.md): contraction and rounding by 62 %, staggered overshooting satellites, the Gaussian pulse, the lower-dot drop, the separate stem entrance and shake, eye half-turns (frequency 14), frequency-11 direct glyph blending, and decay of body motion with `1 − g`. Parts composite over one another.
- Thinking is debounced by 350 ms, a labeled adaptation replacing the earlier 1.2 s delay.
- Settled eyes render the true contours through exact per-contour SDF textures, with the lid squash inverted and gradient-corrected. Mid-morph shapes are capsules, as in the reference's own interpolation.
- The background is pure black at the user's request.
- Done is unchanged.

Evidence: device frame recordings and the injected scenario replay, recorded in the lesson's `RESULTS.md`. There is no physical review yet.
