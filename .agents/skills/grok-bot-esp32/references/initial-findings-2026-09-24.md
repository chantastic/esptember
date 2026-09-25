# Grok-inspired Pi Pet: design and implementation handoff

> **Path note (added later):** Pi Pet moved from `days/day-17-tamagotchi/extensions/pi-pet/` to `days/day-34-pi-pet/` when it became ESPtember Day 34. Paths below are as captured.

Design review, September 24, 2026. Three agents reviewed the visual reference, ESP32 renderer, and Pi lifecycle integration. This is a proposal based on a read-only snapshot of active Pi work; it does not supersede the project's maintained contract or authorize firmware flashing or service deployment.

## Recommendation

Continue the existing Pi Pet in ESPtember. Keep its Pi extension, session hub, USB protocol, roster, M5GFX renderer, and touch calibration. Concentrate the next pass on the eye transform pipeline and state-specific motion. No framework migration is needed for this design.

The actual target is the M5Stack StopWatch with ESP32-S3, 8 MB PSRAM, and a nominal 466×466 AMOLED. ESPtember's current panel correction is **468×466, rotation 0, x offset 6**. Use that current project geometry. [Vendor specifications](https://docs.m5stack.com/en/core/StopWatch), [current Pi Pet contract](/Users/chan/Developer/esptember/days/day-17-tamagotchi/extensions/pi-pet/SPEC.md).

The [xAI design article](https://x.ai/news/designing-grok-bot) uses the avatar as both identity and lifecycle feedback. Its showcased motion system is procedural SVG, not a video. The six shown states are Idle, Working, Waiting, Blocked, Thinking, and Done. Sleep is an embedded extension.

## Interactive design study

`grok-motion-study.html` in this task's output directory is an offline design study. It provides the six reference lifecycle states plus sleep, normal/quarter-speed playback, a repeatable 12-second timeline, pause, single-frame advance, and a touch glance. The host design controls expose three head shapes, color, full eye contours versus capsule approximation, a reference gaze spring versus proposed crisper glances, and face-to-symbol transitions.

The full contours come from the existing Pi extraction of the article's 25 expressions. The study uses independently written rendering and a deterministic schedule. **It is not a pixel-perfect reference port, the firmware, a live Pi connection, or evidence of hardware performance.** It simplifies head silhouettes, spin/celebration, face-to-glyph morphing, fitting, and working-eye softening. Its default crisper gaze is a proposal, not a measured feature of the article.

The capsule switch compares representations within this study, not a complete emulation of the current device renderer. Use it to inspect shape transitions, then validate the selected behavior on the embedded candidate separately.

## Findings to address in order

### 1. Restore per-state face tuning

The deployed reference supplies tuning that the current candidate omits. Idle/Working use size 1, gap 1, eye width 1.2, and eye height 1.16. The alternate profile starts at size .86, gap 1.18, width .96, and height .92, with state overrides. Waiting/Bored uses size .92; Done/Celebrate uses size .74, width .84, and height .84. Preserve the source's per-head face fitting and compose these adjustments independently of gaze and lids.

### 2. Restore the Working and Waiting overrides

- Working's reference preprocessing softens expressions 7 and 16 relative to neutral eye axes, normalizes proportions, and matches the two eyes' dimensions. Applying raw poses misses this behavior.
- The article maps Waiting to Bored and explicitly fixes expression 4, matches eye stroke to at least 8.6 source units, and sets minimum eyelid openness .8. The candidate currently cycles 4, 22, and 0.
- Pi's `waiting` means user input is needed. The bored appearance is faithful to the article but may understate that need. Keep this as an explicit product decision: reference-style waiting, or an attentive adaptation with a stable attention cue. Do not silently change the wire meaning or unseen/acknowledgment policy.

### 3. Retain corresponding eye contours

The current asset generator fits each original 48-point eye outline to a capsule: center, axis angle, length, and width. This loses curvature and changes the intermediate morphs. The review found maximum contour-to-fit deviations of 5.58 source units for expression 20/right, 4.41 for 2/right, and 4.11 for 11/left. These are numerical geometry comparisons, not physical display measurements.

All 25 expressions × 2 eyes × 48 points × 2 coordinates × 2 bytes = **9,600 bytes** using int16 coordinates. A fixed-point scale of 64 would preserve approximately 1/64-unit precision over the observed coordinate range, subject to a range check in asset generation. Memory alone does not justify discarding the contours.

Preserve vertex correspondence, winding, and starting point across poses. Interpolate from the currently rendered contour when a transition is interrupted. Use a bounded eye-region scanline rasterizer or local mask, while retaining the existing head SDF pipeline. Do not calculate distance to all 48 edges for every full-screen pixel. Validate the actual CPU cost before selecting antialiasing resolution.

### 4. Match timing before adding more movement

The candidate's main spring update already follows the source's damped spring form. Its gaze spring uses frequency 13 rad/s and damping ratio 1, also present in the source. Approximately 300 ms to 90% settling is a continuous-model estimate; it is not by itself proof of an incorrect port.

The larger fidelity differences are expression construction, transform composition, entry behavior, and timing. Reference state entries usually trigger a blink and use faster initial expression convergence. Candidate entries schedule a later blink and use lower expression frequencies.

Idle gaze targets are all zero in the candidate. That is relevant to the earlier contract's promised wandering eyes, but expression changes can themselves move the eyes and the source also keeps idle gaze centered. Add nonzero gaze only as a deliberate adaptation. Likewise, the proposed crisper preset (frequency 28, damping .88) needs visual review; it is not a recovered reference constant.

For a more responsive adaptation, choreograph gaze movement → fixation hold → a smaller body follow → settle. Avoid piling unrelated random motion on every channel. Use a shared clock and seed so a selected sequence can be replayed and compared.

### 5. Fit the complete transformed eye

Reference confinement evaluates the eye boundary against the head's horizontal spans. The candidate primarily nudges an eye's center. An eye can have a safe center while part of its outline leaves the intended face region. Fit the full transformed contour continuously, after local shape, scale, gaze, and lid transforms. Clipping can prevent drawing outside the head, but is not a substitute for natural placement.

## Preserve the existing Pi integration

| Wire state | Presentation | Meaning |
| --- | --- | --- |
| `idle` | Idle | Ready |
| `thinking` | Thinking | Agent processing |
| `tool` | Working | Tool executing |
| `error` | Brief surprise, then Blocked | Failure |
| `done` | Done, then Waiting if unseen, eventually Idle | Turn complete / your turn |
| `waiting` | Waiting | Needs input |
| `sleeping` | Sleeping | Resting |

Keep `pet <id> <state> <style> <name>|<detail>` and the current hub ownership of serial. Keep left/right paging, short-both acknowledgment, long-both sleep, eight-session limits, reconnect roster replay, redaction, and shared calibration rules.

Current Pi hooks emit idle/thinking/tool/error/done. Waiting and sleeping are accepted wire states, not proof that Pi approvals are already integrated. Acknowledgment is not tool approval. The extension's session replacement path deserves a separate fake-Pi replay: on `session_start`, it changes ID and calls `connect()`, but an existing socket may prevent a fresh hello and old-session cleanup. Confirm the real Pi lifecycle before changing it.

Treat transport health separately from activity. A lost host should not leave an indefinitely believable Working face; select and document a stale-connection indication without interpreting disconnect as task success or sleep. Audio remains a later phase; the display study needs neither cloud credentials nor paid speech calls.

## Acceptance and evidence

1. Record deterministic reference and candidate sequences for Idle, entry blink, Thinking, Working, Waiting, Blocked, Done, and interrupted expression changes. Compare onset, 70, 150, 300, 480, and 1,000 ms, plus complete multi-second clips. Include off-center gaze and the narrowest head.
2. Measure eye boundary, closure, reopening, fixation, body lag, and glyph timing separately. Confirm the pair neither clips nor swaps orientation during morphs.
3. Retain current portable lifecycle/roster tests. Add only meaningful regressions for chosen timing invariants and session replacement; visual style itself needs visual review.
4. Use **30 fps sustained / 33.3 ms per frame** as an initial design budget. Profile rendering and transfer separately with status text and multiple sessions. Forty fps is a stretch budget. Browser playback does not establish device throughput.
5. A 468×466 RGB565 framebuffer costs 436,176 bytes (~426 KiB); eight 128×128 int8 head textures cost 128 KiB, plus 32 KiB for two active internal-RAM copies. Preserve memory headroom for existing device services.
6. Keep compile, injected-device, and physical acceptance separate. Pi's session reports newer ~39 fps idle; maintained RESULTS records the earlier 25–30 fps version. Neither was independently remeasured in this review.
7. Before calling the embedded product finished, resolve the documented missing runtime recalibration path, physically verify tilt direction, reconcile SPEC/HAND-REVIEW with the newer eight-style/state implementation, and complete hand review. No physical testing or flashing occurred here.

The browser study was checked for all seven state selections, pause, one-frame advance, seeking, and persistence after reload. Responsive layout was inspected at standard and 320-pixel browser widths; the narrow root had equal client/scroll widths. No browser error/warning logs were reported during that check.

## Relevant current files

- [Firmware candidate](/Users/chan/Developer/esptember/days/day-17-tamagotchi/extensions/pi-pet/.build/device-candidate/pi_pet/pi_pet.ino): state definitions near line 110, entry near 232, Pi mapping near 282, scheduler near 442, springs near 488, eye rendering near 636.
- [Asset generator](/Users/chan/Developer/esptember/days/day-17-tamagotchi/extensions/pi-pet/.build/assets/gen_assets.py): capsule conversion near line 82.
- [Eye contours](/Users/chan/Developer/esptember/days/day-17-tamagotchi/extensions/pi-pet/.build/assets/eyes.json).
- [Pi extension](/Users/chan/Developer/esptember/days/day-17-tamagotchi/extensions/pi-pet/.build/host-candidate/pi-pet/index.ts): session registration and event mapping.
- [Maintained contract](/Users/chan/Developer/esptember/days/day-17-tamagotchi/extensions/pi-pet/SPEC.md), [results](/Users/chan/Developer/esptember/days/day-17-tamagotchi/extensions/pi-pet/RESULTS.md), and [hand review](/Users/chan/Developer/esptember/days/day-17-tamagotchi/extensions/pi-pet/HAND-REVIEW.md).

The active Pi session has temporary reference extracts under `/tmp/grokbot/gb/`, including `avatar_seg.js`, `ag.js`, and the deployed script `js/25eegay1u-vgr.js`. Those are temporary inspection evidence, not durable dependencies. Revalidate the active candidate before editing; Pi may have changed it since this review.
