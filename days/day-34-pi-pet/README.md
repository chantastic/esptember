---
board: m5stack-stopwatch
day: 34
title: Pi Pet
toolchain: Arduino ESP32 3.3.10 + M5Unified 0.2.19 + M5GFX 0.2.26 + pi extension (TypeScript) + Node serial hub
summary: "One Grok Bot–style pet per running pi coding-agent session: it thinks, works, gets blocked, celebrates, and waits for you."
verification: "Contract, host bridge, and 13-scenario injected-device replay verified; device framebuffer and frame-sequence captures recorded; physical hand review pending"
---

## The assignment

Give every running [pi](https://github.com/badlogic/pi-mono) coding-agent session its own pet on the Stopwatch.

The pet thinks while the agent thinks, works while a tool runs, turns into a `!` when the session stops on an error, celebrates when it finishes, and then waits for you until you check in.
The pushers page between sessions like channels.

The character and motion port the avatar system from xAI's [*Designing Grok Bot*](https://x.ai/news/designing-grok-bot):
- simple head shapes with expressive eyes;
- one avatar that carries the whole lifecycle;
- no separate status indicator.

The durable source is this prompt, [SPEC.md](https://github.com/chantastic/esptember/blob/main/days/day-34-pi-pet/SPEC.md), the machine-readable contract, and its layered acceptance criteria.
Generated firmware and host code are disposable candidates.

## Reference frame

![Device framebuffer: a working pi session, with two more sessions paged by the dots](https://esptember.com/images/day-34-pi-pet/device.png)

This is a framebuffer captured from the attached Stopwatch, not a model rendering.
It shows a session named `esptember` running a compile, with two other sessions waiting on the page dots.

![Device framebuffers of the six lifecycle states](https://esptember.com/images/day-34-pi-pet/states.png)

Idle, Thinking, Working, Waiting, Blocked, and Done, each captured from the device.
Thinking and Blocked are the head itself, contracted into dots or dropped into a `!`.
Working and Done spin their eyes around the head, trailing orbit ribbons.

## Controls

Hold the Stopwatch with the lanyard down:

- **BtnA/left · BtnB/right:** previous/next session (demo: previous/next bot). A page counts as looking.
- **Both briefly:** acknowledge the focused pet; it bounces and stops waiting (demo: next state).
- **Hold both for 600 ms:** sleep; any page or acknowledge wakes it.
- **Touch:** tap the pet to bounce, drag it around, or tap elsewhere to make it look there.

## The build prompt

```text
Build ESPtember Day 34: Pi Pet for the M5Stack Stopwatch Dev Kit (C152), plus its host bridge
for the pi coding agent.

Read these before writing code:
- .agents/skills/build-stopwatch-lessons/SKILL.md
- .agents/skills/prompt-first-embedded-lessons/SKILL.md
- .agents/skills/grok-bot-esp32/SKILL.md
- days/day-07-touch-calibration/SPEC.md
- days/day-34-pi-pet/SPEC.md
- days/day-34-pi-pet/tests/contract.json
- pi's extension docs (docs/extensions.md in the pi-coding-agent package)

Treat the prompt, spec, tests, and acceptance criteria as source.
Generated firmware and host code are disposable candidates under this lesson's ignored .build/.

Device:
- Arduino ESP32 3.3.10, M5Unified 0.2.19, M5GFX 0.2.26; ESP32-S3, OPI PSRAM, 16 MB flash, app3M_fat9M.
- Port the avatar engine described in SPEC.md "Character and motion". At build time, derive the head
  shapes, face fits, and 25 eye-expression contours from the public page's avatar component into a
  generated header: head and contour signed-distance textures, lid axes, and capsule fits for morphs.
  Keep the generated header in .build/. Use a pure black background.
- Implement Thinking and Blocked with the retained-head transitions, not cross-dissolves.
- Draw into one full-screen RGB565 canvas in PSRAM with antialiased distance fields. Keep active
  textures in internal RAM. Use two-level tile classification so only edge pixels are shaded.
  Push only dirty rectangles. Sustain at least 24 fps in normal states.
- Implement the springs, blink and blink-through expression changes, winks, bounce chain, tilted
  spins with orbit-ribbon lanes, glyph morphs, badge, per-state motion, the eight roster styles,
  the status band, the page dots, and demo mode exactly as SPEC.md says.
- Implement the hub-to-device text protocol, the diagnostics (pets, act, reset, shot, rec/recdump),
  and the GB_* evidence lines.
- Load espt-touch/record after the panel geometry is corrected; map raw touches through the full
  shared warp.

Host:
- A pi extension that reports session state per SPEC.md's mapping, redacts secrets from the activity
  line, honours PI_PET=0 and PI_PET_DETAIL=0, exports redact(text), starts the hub on demand, never
  throws into pi, and registers /pet (status, release, raw poke).
- A hub that owns the serial port, merges sessions, sanitizes and caps fields, assigns distinct
  styles, removes a crashed session's pets, resyncs on GB_READY, supports release, and honours
  PI_PET_SOCKET and PI_PET_PORT=none.

Verify in layers and record only what ran:
- tests/run.sh (contract, mutation, reference model, host bridge);
- compile with the pinned toolchain;
- flash app-only at 0x10000 (never a merged image at 0x0), then run tests/check_device.py with the
  hub stopped;
- capture framebuffers of every state, and rec sequences of working, thinking, blocked, and done,
  and compare them frame by frame with the web demo (see the grok-bot-esp32 skill's review tools);
- run one headless pi session through the real extension and hub;
- leave the device in demo mode for HAND-REVIEW.md.
```

## Required behavior

- Each pi session reports its state through a local hub to the Stopwatch:
  - thinking → Thinking;
  - a running tool → Working;
  - an error → a brief Surprised, then Blocked if the session stops on it;
  - done → Done, then Waiting until you acknowledge it.
- The first session is focused automatically. A pet that finishes takes focus unless the focused pet is busy, and it stays unseen until you page to it or acknowledge it.
- The activity line shows the current command or file, with tokens, passwords, and bearer credentials redacted.
- A crashed session's pet disappears. The last pet leaving returns the device to its demo bot.
- The pet keeps the reference's motion:
  - contour eyes that blink into dashes and blink through expression changes;
  - glances and winks;
  - a decaying bounce chain;
  - eye spins trailing orbit ribbons;
  - Thinking's traveling highlight;
  - Blocked's tapered stem that drops in and shakes.

Wi-Fi transport, text-to-speech through an authenticated `devices.chan.dev/v1/speech`, and tool-call approvals are the roadmap in SPEC.md.

## Recorded evidence

- **Host contract:** the shared contract runner validates three disposable reducer shapes, 61 scenario steps, and 10,000 generated actions, and rejects a deliberate mutation. A reference model audits every expectation and fuzzes 40,061 checks. The host bridge passes redaction and hub-merge tests on a temporary socket.
- **Injected-device evidence:**
  - The replay passes 13 scenarios and 459 checks on the attached Stopwatch.
  - The shared touch map (version 2, generation 2) survives every app-only flash.
  - A headless pi run through the installed extension drove thinking, working, and the error wince live.
  - Frame-by-frame device recordings were compared with screencasts of the post's live demo.
  - The device renders at about 24–31 fps in every state.

The images above are device framebuffers: injected-device evidence, not physical review.
Physical review of the transitions, contour eyes, black level, tilt direction, and touch is still to come, as is the shared recalibration flow.
