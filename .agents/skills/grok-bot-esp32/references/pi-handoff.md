# Pi reference handoff — September 24, 2026

**Outcome (recorded afterward):** the user pasted this handoff into the ESPtember Pi session. Pi first reported eleven remaining Thinking/Blocked differences without changing anything. The user then authorized implementation ("do the best you can"), asked for a black background, and later for contour-accurate eyes. The results are summarized in [current-status.md](current-status.md) and the lesson's `RESULTS.md` (revision 3). The text below is the handoff as prepared.

Prepared locally; not delivered. On September 24, the user asked about handing this work to Pi. Direct terminal access was denied because computer control of Ghostty is unavailable in this session. No message was injected into Pi and no session log was edited. The user can paste the message below into the active ESPtember Pi session.

---

Codex handoff for the Grok Bot / StopWatch Pi Pet work: please read `.agents/skills/grok-bot-esp32/SKILL.md` in this ESPtember repository, then its `references/current-status.md`, `references/glyph-morphs.md`, and `references/source-map.md`. All resources are bundled in that skill directory.

This packages the Grok Bot reference research, original contour data, upstream animation source extracts, historical review sheets, a revised browser study, and ESP32/Pi integration findings. The findings are dated snapshots from concurrent work, so compare with your current candidate before acting.

The user prefers your Done animation. Preserve its chained bounces, eye spins and orbiting ribbons. The first Codex browser study was missing these and is not the source of truth; the revised study follows your completion choreography more closely but still is not a complete renderer port.

The user approved the revised browser study after we corrected Thinking and Blocked. Both originally used a cross-dissolve in our study; this feedback was about Codex's simulation, not a confirmed defect in your current device renderer. Your latest session notes already report a direct engine port, tapered Blocked glyph, moving Thinking highlight, and improved performance, so compare with your current implementation before changing anything.

The saved reference formulas establish these specific transitions:

- Thinking: retain the head and round/contract it into the center dot; stagger the outer dots with overshooting spread; use a 1.4-second traveling Gaussian pulse affecting size, lift, and brightness together.
- Blocked: retain the opaque head and shrink/drop it into the lower dot; introduce the separately growing tapered stem with a downward entrance and decaying shake.
- Both: a frequency-14 critically damped morph spring, circle rounding completed by 62% of morph progress, cylindrical eye half-turns on entry/exit, and retained outgoing glyph geometry. Direct glyph-to-glyph changes use a frequency-11 style spring without an extra half-turn.
- The browser study preserves spring positions/velocities on interruption and uses separate pulse clocks to prevent visible outgoing pulses jumping. That clock separation is an explicit study adaptation, not an exact upstream behavior.

Exact equations, stem path, source offsets, and review evidence are in `references/glyph-morphs.md`. The full upstream bundle and original contour resources are included with checksums. `assets/grok-motion-preview.html` is the standalone study; browser checks do not establish hardware parity.

For any requested visual-fidelity pass, compare the reference's face profiles, working-eye softening, waiting override, morph geometry and blink scheduling. Full contours are about 9.4 KiB as int16 XY, but do not replace the current renderer without measuring visible benefit and cost. Frequency 13/damping 1 is also used by the reference; faster glances are optional design changes.

Keep the current session protocol, roster, unseen/ack rules, serial ownership, touch map and display correction. Revalidate older findings: the newer candidate already checks eye extents and has improved expression/spin behavior. A source difference is not automatically a product bug.

For this handoff, inspect and report current matches and actionable remaining differences. Preserve your existing work and follow the user's authorization in this Pi session for subsequent implementation or device operations; this reference handoff does not expand that scope.
