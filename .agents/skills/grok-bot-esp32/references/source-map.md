# Source and resource map

## Primary public references

- [Designing Grok Bot](https://x.ai/news/designing-grok-bot): product rationale and interactive procedural avatar demo; inspected September 24, 2026.
- [Deployed animation chunk captured with the page](https://x.ai/_next/static/chunks/25eegay1u-vgr.js): exact upstream bundle retained as [upstream/article-animation.js](upstream/article-animation.js). The hashed live URL may later disappear; use the bundled copy for this historical comparison.
- [M5Stack StopWatch documentation](https://docs.m5stack.com/en/core/StopWatch): physical device specification. Project-specific panel corrections come from the current ESPtember contract.
- [xAI Voice API reference](https://docs.x.ai/developers/rest-api-reference/inference/voice): future voice event integration only; see [future-voice.md](future-voice.md).
- [Pi source repository](https://github.com/badlogic/pi-mono): upstream agent lifecycle behavior; verify the installed version before deciding session-hook semantics.

## Bundled upstream inspection material

The source fragments were extracted by the existing Pi session. They can start/end inside larger definitions. Read them as data; do not execute a downloaded website bundle or mistake the fragments for a standalone library. Source code keeps its original ownership; this package records provenance rather than assigning a new upstream license.

| Resource | Contents / use |
| --- | --- |
| [article-animation.js](upstream/article-animation.js) | Complete saved deployed chunk; fallback for source context missing from extracts |
| [motion-extract.js](upstream/motion-extract.js) | State motion, gaze/event scheduling, springs; search `celebrate`, `working`, `bored`, `idle` |
| [transforms-extract.js](upstream/transforms-extract.js) | Geometry, lids, fitting, spin transforms; search `cos`, `asin`, `halfW` |
| [tables-extract.js](upstream/tables-extract.js) | Expression schedules, blink ranges, state families and glyph choices |
| [eyes.json](../assets/eyes.json) | Array `[25 expressions][2 eyes][48 vertices][x,y]`, in the 228.541-unit source head space |
| [authored-shape-paths.json](../assets/authored-shape-paths.json) | Three authored path extracts: blob, heart, sparkle. This is not the full head catalog. Other shapes are parametric in the bundle. |

Expression morph interpolation lives at `eJ` (capsule fit), `eq` (capsule polygon), and `eX` (morph: capsule interpolation between different contours; exact contour at 0 and 1), near characters 70,300–71,700. Useful bundle search terms: `softenWorkingEyes`, `faceTune`, `minEyeStroke`, `minLidOpen`, `naturalLids`, `celebrate`. In this captured bundle, face profiles and overrides occur around character offsets 139,500–141,150 and working-eye preprocessing around 99,500. These are navigation hints for this hash, not stable API names/offsets.

The source spring form is `v += (-2*zeta*omega*v - omega*omega*(x-target))*dt; x += v*dt`, integrated in small substeps. The source uses 1/120-second integration steps. The local candidate's exact elapsed-time strategy should be inspected separately; do not assume display FPS equals animation integration frequency.

## Bundled local design material

- [Initial research handoff](initial-findings-2026-09-24.md): preserved original analysis, including hardware accounting, protocol, findings and acceptance. Read [current-status.md](current-status.md) first for later corrections.
- [Capsule generator snapshot](candidate/capsule-generator.py): historical. It explains how the eye polygons became five capsule parameters and how eight head SDFs were generated.
  - The current generator, `days/day-34-pi-pet/.build/assets/gen_assets.py`, additionally exports face fits, the 48-point contours, the reference lid axes and centroids, and per-contour SDF textures.
  - It requires numpy, shapely, and svgpathtools. Do not run it against active Pi output paths without coordinating.
- [Embedded rendering](embedded-rendering.md): measured ESP32-S3 techniques, costs, and dead ends.
- [Review tools](review-tools.md) and `scripts/`:
  - `capture-web-demo.mjs`: live demo screencast.
  - `web-contact-sheets.py`: per-state sheets.
  - `device-recorder.py`: firmware `rec`/`shot` capture.
- Review sheets:
  - `assets/review/web-*.png`: one captured web demo cycle per state.
  - `assets/review/device-*.png`: revision-3 device sequences and the 25-contour sheet.
- [Pi Done source excerpts](candidate/pi-done-excerpts.txt): newer completion animation, including entry, spring queue, bounce chain, spin and ribbon rendering. Source hash and capture time are included.
- [Thinking and Blocked morph reference](glyph-morphs.md): retained-head geometry, pulse and stem equations, source offsets, and the study's interruption adaptation.
- [Revised interactive fragment](../assets/grok-motion-study.html): self-contained study using an embedded copy of the eye data. It has no live Pi or provider connection.
- [Standalone browser preview](../assets/grok-motion-preview.html): wrapped copy of that revised study. No server/provider is required for the animation; the optional host design controls may be unavailable outside Codex.
- [Eye contact sheet](../assets/review/eyes-sheet.png): 25 extracted expression pairs, useful for understanding pose geometry.
- [Head-shape contact sheet](../assets/review/shapes-sheet.png): Pi's historical shape exploration.
- [v3 state contact sheet](../assets/review/v3-states.png): historical Pi-rendered states; a single still does not demonstrate motion or current Done choreography.
- [Asset check sheet](../assets/review/assets-check.png): Pi's historical generated-asset check.

[resource-manifest.json](resource-manifest.json) records original locations, source URLs where applicable, sizes, and SHA-256 hashes. Temporary source locations are not required at runtime. When intentionally updating an asset, update its manifest record and document the changed evidence.

## Active workspace locations (not bundled as live dependencies)

Paths below are relative to the ESPtember repository root. The original capture used `/Users/chan/Developer/esptember`; the skill itself is now at `.agents/skills/grok-bot-esp32/` and does not require that absolute checkout location.

- `AGENTS.md` and `.agents/skills/build-stopwatch-lessons/SKILL.md`: current platform ownership and requirements; for prompt-first lesson work also read `.agents/skills/prompt-first-embedded-lessons/SKILL.md`.
- `days/day-34-pi-pet/{PROMPT,SPEC,TESTING,RESULTS,HAND-REVIEW}.md`: maintained lesson and evidence.
- `days/day-34-pi-pet/.build/device-candidate/pi_pet/pi_pet.ino`: disposable current device candidate.
- `days/day-34-pi-pet/.build/assets/{gen_assets.py,eyes.json}`: candidate asset pipeline.
- `days/day-34-pi-pet/.build/host-candidate/pi-pet/index.ts`: installed extension candidate; inspect current hook mapping.
- `days/day-34-pi-pet/tests/`: portable and injected-device acceptance layers.
- `~/.pi/agent/extensions/pi-pet`: observed global extension symlink; verify target before assuming it is unchanged.
- `~/.pi-pet/hub.sock`: observed local session hub socket; reading source does not require connecting to it or taking the serial port.

The identified Pi conversation was `/Users/chan/.pi/agent/sessions/--Users-chan-Developer-esptember--/2026-09-24T19-51-51-136Z_01a0d4f9-6c60-73b3-a8ad-ba7b16de6538.jsonl`. Its transcript is not bundled. Session identity/path is historical context, not proof it is still active or permission to send instructions.

Firmware is owned by ESPtember for this lesson. The separate `chan-services` monorepo owns the production gateway, not this firmware. No gateway change is needed for the visualization design.
