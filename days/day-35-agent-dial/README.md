---
board: m5stack-stopwatch
day: 35
title: Agent Dial
toolchain: Arduino ESP32 3.3.10 + M5Unified 0.2.19 + M5GFX 0.2.26 + LVGL 9.5.0 + ArduinoJson 7.4.2 + Node hub (USB serial) + harness adapters (pi, Claude Code, CLI)
summary: "An honest fork of the OpenHarness dial for the Stopwatch: see every agent session, answer, approve, stop, and speak tasks, whatever harness runs them."
verification: "Contract, host bridge, and 31-scenario device replay verified; live pi and Claude Code approval driven on the attached board; physical hand review pending"
---

## The assignment

Fork the [OpenHarness device](https://github.com/autonomous-ai/openharness/tree/468979983431cbd86905d97fd211d588fa1d880a/devices/harness-device) onto the Stopwatch.

Upstream is a round USB dial for your desk.
It shows each agent's progress and last recap.
It lets you answer an agent's question, cancel a turn, or speak a new task.

It's also wired to one product: its daemon, account, desktop tabs, and speech backend.

This fork keeps what upstream's firmware actually ships:
- the screens;
- the touch gestures;
- the timings;
- the beep;
- the frame format;
- the message names.

It changes only what the hardware or harness neutrality forces, and [SPEC.md's provenance table](SPEC.md#provenance) lists each change with its reason.
A small local hub replaces the daemon.
Adapters connect pi, Claude Code, and any script, and each one declares what its harness actually lets it do.

Upstream is MIT-licensed, © 2026 Autonomous, Inc.
Its notice ships in [LICENSE-OPENHARNESS](LICENSE-OPENHARNESS).

The durable source is this prompt, [SPEC.md](SPEC.md), the machine-readable contract, and its layered acceptance criteria.
Generated firmware and host code are disposable candidates.

## Reference frames

![Device framebuffers: the Overview, a working pi tile, and a Claude Code recap](https://esptember.com/images/day-35-agent-dial/overview.png)

These are framebuffers captured from the attached Stopwatch, not renderings.
The dial draws them with LVGL, upstream's Geist faces, and upstream's icons, from the same messages a hub sends.

| | |
| --- | --- |
| ![A working pi tile: Working… with the running command](https://esptember.com/images/day-35-agent-dial/tile-working.png) | ![A Claude Code tile with its recap card](https://esptember.com/images/day-35-agent-dial/tile-recap.png) |
| ![A Claude Code approval: Allow or Deny](https://esptember.com/images/day-35-agent-dial/question-approval.png) | ![The notification drawer](https://esptember.com/images/day-35-agent-dial/drawer.png) |
| ![Cancel task? with Yes, cancel and Back](https://esptember.com/images/day-35-agent-dial/confirm.png) | ![The voice overlay while recording](https://esptember.com/images/day-35-agent-dial/voice-recording.png) |

## What each harness can do from the dial

Nothing reaches into a harness from outside: each one calls out through an extension point.
So the dial only offers what that extension point allows.

| Harness | See progress and recaps | Approve or deny a tool | Answer a question | Stop a turn | Speak a new task |
| --- | --- | --- | --- | --- | --- |
| pi | yes | opt-in (`AGENT_DIAL_APPROVE=1`) | no built-in question tool | yes | yes |
| Claude Code | yes | yes (`PermissionRequest` hook) | no documented hook | no documented hook | no documented hook |
| `agent-dial` CLI | yes | yes | yes (`agent-dial ask`) | caller's choice | caller's choice |

A tile hides Voice when its session can't take a prompt.
Back on a working tile that can't be stopped says so instead of pretending.

## Controls

Hold the Stopwatch with the lanyard down.
Touch works as it does upstream: swipe between sessions, swipe up from the bottom for the Overview, swipe down from the top for the drawer, and tap options, buttons, and rows.
The pushers use the default ESPtember grammar, mapped onto upstream's on-screen controls:

- **Left/BtnA · Right/BtnB:** previous/next session, like upstream's ‹ › chevrons. From the Overview, go to the last session.
- **Both briefly:** Voice, like upstream's tile button. While recording, stop and send.
- **Hold both for 600 ms:** upstream's BOOT key.
  - It dismisses a question.
  - On a working tile it asks `Cancel task?`.
  - It aborts a recording, or closes the drawer or Settings.

## The build prompt

```text
Build ESPtember Day 35: Agent Dial for the M5Stack Stopwatch Dev Kit (C152), plus its local hub and
harness adapters. It is an honest fork of the OpenHarness dial.

Read these before writing code:
- .agents/skills/build-stopwatch-lessons/SKILL.md
- .agents/skills/prompt-first-embedded-lessons/SKILL.md
- days/day-07-touch-calibration/SPEC.md
- days/day-13-c25k/SPEC.md (input grammar)
- days/day-35-agent-dial/SPEC.md and tests/contract.json
- upstream at 468979983431cbd86905d97fd211d588fa1d880a:
    devices/harness-device/firmware/main/{cable_frame,cable_link,cable_client,audio_client,ptt}.c
    devices/harness-device/firmware/main/ui/{ui_screens,touch}.c
    cli/src/cable/{cableSession,cableHost,cableFrame}.ts
  Follow the firmware, not upstream's docs/*.md or *.html, which describe unshipped designs.
- pi's docs/extensions.md, and Claude Code's hooks reference.

Treat the prompt, spec, tests, and acceptance criteria as source. Generated firmware and host code
are disposable candidates under this lesson's ignored .build/. Credit upstream and keep
LICENSE-OPENHARNESS with anything derived from it.

Device (.build/device-candidate/agent_dial/):
- Arduino ESP32 3.3.10, M5Unified 0.2.19, M5GFX 0.2.26; ESP32-S3, OPI PSRAM, 16 MB flash,
  app3M_fat9M_16MB. Correct the panel to 468 x 466, offset 6/0, rotation 0, right after M5.begin().
- Render with LVGL 9.5.0, as upstream does, on pure black: a 40-row partial buffer in internal RAM,
  flushed with M5.Display.pushImage and copied into a PSRAM framebuffer for diag.shot. Use upstream's
  styles, colors, and positions (+1 px in x), upstream's icons_voice and icons_settings images, and
  Montserrat only for LV_SYMBOL glyphs. Regenerate upstream's Geist faces from its vendored TTFs with
  its lv_font_conv options over Latin-1 and punctuation (reg 20/25/32/38, med 28/32/38, sem 24, and
  digits-only med 64) so they fit the 3 MB app partition. Keep lv_conf.h in the sketch and pass
  -DLV_CONF_INCLUDE_SIMPLE -I<sketch> to the C and C++ compiles.
- Measure elapsed time from any stamp an input handler may set with a signed difference against
  millis(), never against a `now` sampled earlier in the loop. Give the loop task a 16 KB stack.
- Port upstream's shipped screens per SPEC.md "Layout": Overview, tile (resting, recap card, working
  verb + elapsed + activity line, Waiting for you), question (single, multi + Done), notification
  drawer, voice overlay (bars, then sparkles), Cancel task?, toast, Not connected badge, Settings
  (voice language, swipe direction, brightness, touch calibration, version), brightness.
- Implement the portable state and every Behavior rule exactly; expose it as diag.state and accept
  diag.act with every contract action.
- Input: the C25K grammar (80 ms join, 80 ms overlap, 600 ms hold, chord consumes clicks) mapped
  per SPEC.md "Controls"; upstream's touch thresholds; load espt-touch/record after geometry and map
  raw touches through the full shared warp; run the shared calibration flow from Settings.
- USB: the fork's framing (A5 45) and vocabulary, hello cadence, 15 s silence, LOG frames after
  welcome, SHOT frames, zero CDC write timeout except during a dump.
- Voice: M5.Mic at 16 kHz s16le mono, streamed live in <= 1024-byte PCM frames; stop the speaker
  first; 10-minute cap; the beep only when the mic is off.
- Wi-Fi: join the day22 slot, advertise _agent-dial._tcp, serve POST /v1/exchange on 47838 with the
  bearer token from NVS namespace agent-dial; USB-only provisioning.
- Emit the AD_* evidence lines.

Host (.build/host-candidate/agent-dial/):
- hub.mjs: owns the serial port (vendor 303a), frame codec, welcome/ping/silence rules, a foreign
  board check, session merge with harness-prefixed ids, ttl expiry, field caps, capability-gated
  routing of answers and commands, close-on-exit for questions, speech-to-text via AGENT_DIAL_STT,
  voice routing, pairing and keychain Wi-Fi provisioning over USB, release/pause/resume/monitor/raw,
  and `status`. It exits after 5 idle minutes and never opens a network listener.
- lib/client.mjs: adapters' transport. Hub socket first (start the hub if a board is on USB), then
  direct Wi-Fi from ~/.agent-dial/device.json, otherwise silent retries. Exports redact(text).
- pi extension (index.ts): session state from pi events; stop via ctx.abort(); prompt via
  pi.sendUserMessage(); opt-in approvals via a blocking tool_call handler. Never throws into pi.
- agent-dial CLI (bin/agent-dial.mjs): `claude <event>` for Claude Code command hooks (SessionStart,
  UserPromptSubmit, PreToolUse, PostToolUse, PermissionRequest, Stop, SessionEnd), `ask`, `event`,
  `status`, `pair`, `wifi`, `release`, and `install-claude` which prints the hooks JSON.

Verify in layers and record only what ran:
- tests/run.sh (contract, mutation, reference model, frame vectors, host bridge);
- compile with the pinned toolchain;
- flash app-only at 0x10000 when the board already carries this partition table; a board with a
  different table (a factory unit) needs a merged image at 0x0, which is a fresh install that erases
  NVS: back up its flash first and say so;
- pause any other hub (Day 34's pi-pet hub also claims vendor 303a), then run tests/check_device.py;
- capture framebuffers of every screen with tests/capture_screens.py;
- drive one real approval from Claude Code and one voice-free turn from pi through the hub;
- leave the device on the Overview, connected, for HAND-REVIEW.md.
```

## Required behavior

- **Connecting:**
  - Plug the Stopwatch in and start any adapted session, and the hub starts on its own, pairs the board, and shows the session. No setup screen, account, or pairing code.
  - Unplugged, adapters can reach a paired board directly over Wi-Fi.
- **Sessions:**
  - Each session gets a tile with its name and harness.
  - A working tile shows upstream's rotating verb and elapsed time (`Cooking… 34s`) over the current command or file, redacted.
  - A finished turn replaces the recap card and beeps.
- **Questions and approvals** open full-screen with tappable options. A second question waits in the drawer.
  - Answering returns to that tile.
  - Back dismisses a question without answering it.
  - A question answered elsewhere closes itself.
- **Stop:** Back on a working tile asks `Cancel task?` when the harness allows stopping, and says `Can't stop this from here` when it doesn't.
- **Voice:**
  - Both pushers start recording to the tile, or from the Overview to the most recently active session that takes prompts.
  - Both again, or a tap, sends. Back aborts.
  - The hub transcribes with a command you choose. Nothing leaves your computer unless that command sends it.
- **Drawer:** it collects finished turns you weren't looking at, plus every open question. A sleeping dial wakes onto it.
- **Unplugged or hub quit:** after 15 s the dial clears to `Not connected`, and it reconnects on its own.

## Recorded evidence

- **Host contract:**
  - The shared runner validates three disposable reducer shapes against 183 scenario steps and 10,000 generated actions, and rejects a deliberate mutation.
  - The reference model audits every expectation and fuzzes 40,000 actions for invariants.
  - Two decoders check the frame vectors, including upstream's `A5 48` frames, which the fork ignores. So do the device's own C++ codec and the hub's codec.
  - The host bridge passes 133 checks against a fake board: the codec, redaction, merging, caps, ttl expiry, capability gates, close-on-exit, voice routing, and the Claude Code hook's decisions.
- **Compile evidence:** the LVGL candidate is 1,928,519 bytes (61 % of the app partition).
- **Injected-device evidence:**
  - The replay passes all 31 scenarios, with 4,575 checks, on the attached Stopwatch.
  - The frames above are its framebuffers.
  - Through the real hub over USB, on the attached board:
    - a CLI question opened on the dial with a beep, and the dial's answer came back to the blocked `agent-dial ask`;
    - a headless Claude Code session's `PermissionRequest` opened Allow/Deny on the dial, and the dial's Allow let the command run;
    - a headless pi session appeared, beeped on its summary, and left with `bye`.
  - The taps on the dial were injected over USB, not pressed.

Not yet observed:
- physical touch and pusher review;
- the microphone, voice through a real speech-to-text command, and the speaker's sound;
- direct Wi-Fi;
- battery life.

[RESULTS.md](RESULTS.md) records each run.
