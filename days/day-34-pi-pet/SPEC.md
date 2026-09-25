# Pi Pet behavior contract

## Objective

Give every running [pi](https://github.com/badlogic/pi-mono) coding-agent session a small expressive pet on the M5Stack Stopwatch.
The pet shows what its agent is doing (thinking, running a tool, stuck, finished, waiting on you) and lets you page between sessions with the pushers.
It is a Tamagotchi for agents: you check on it, it reacts, and it lets you know when it needs you.

The character and motion follow the avatar system in x.ai's [*Designing Grok Bot*](https://x.ai/news/designing-grok-bot): simple shapes, expressive eyes, and one avatar that carries the whole lifecycle — idle, working, waiting, blocked, thinking, and done — so no separate status indicator is needed.

## Shared platform contract

The target is the M5Stack Stopwatch with a corrected 468 × 466 drawable panel, rotation 0, x offset 6, and y offset 0.
BtnA is physical left and BtnB is physical right with the lanyard down.
Every touch consumer loads `espt-touch/record`, accepts safe version-1 and version-2 records, maps raw coordinates through the complete shared warp, and provides the shared runtime calibration flow.
No lesson-specific touch correction is permitted.
App-only updates preserve NVS; nothing in this exercise writes NVS.

## System

```text
pi session ─┐   JSON lines over          ┌──────────┐   USB serial text    ┌───────────┐
pi session ─┼─► ~/.pi-pet/hub.sock  ────►│ pet hub  │ ───────────────────► │ Stopwatch │
pi session ─┘   (one extension each)     └──────────┘ ◄─── GB_* events ─── └───────────┘
```

- **Extension:** one per pi process, loaded from `~/.pi/agent/extensions/pi-pet/`. It must never throw into pi or block an agent turn. `PI_PET=0` disables it.
- **Hub:** a single local process that owns the serial port and merges every session. The first extension that cannot connect starts it detached. It exits after 5 minutes with no clients.
- **Device:** renders the focused pet and drives mood from the reported state.

Wi-Fi transport and text-to-speech are later phases (see *Roadmap*). They must not change the state rules below.

## Host → hub protocol

Newline-delimited JSON on the Unix socket `~/.pi-pet/hub.sock`, overridable with `PI_PET_SOCKET`:

| Message | Meaning |
| --- | --- |
| `{"t":"hello","id","name","cwd"}` | Register or rename a session. The name defaults to the cwd basename. |
| `{"t":"state","id","state","detail"}` | Report a state and a short activity line. |
| `{"t":"bye","id"}` | The session ended. |
| `{"t":"status"}` | Hub replies with the port, readiness, and the merged roster. |
| `{"t":"release"}` | Close the serial port for 30 s so a flasher can use it. |
| `{"t":"monitor"}` / `{"t":"raw","line"}` | Diagnostics: receive device lines / send a device command. |

The hub derives a pet id from the last 8 alphanumerics of the session id.
It stores only printable ASCII: names are capped at 24 characters and details at 40, and `|` is replaced.
Unknown states become `idle`.
It assigns each live pet a distinct style (0–7) when possible and caps the roster at 8.
When a client socket closes, every pet it registered is removed, so a crashed session cannot leave a ghost.
On device `GB_READY` or port open, the hub clears the device and resends the full roster.

## Extension state mapping

| pi event | Reported state / detail |
| --- | --- |
| `session_start` | `hello`, then `idle` |
| `agent_start`, `turn_start` (unless a tool is running) | `thinking` |
| `tool_execution_start` | `tool`, with detail `$ <first line of command>`, `reading <file>`, `editing <file>`, `writing <file>`, or the tool name |
| `tool_execution_end` | `error` if `isError` or bash reports a non-zero exit; otherwise `thinking` |
| `agent_settled` | `done` / `your turn` (an error stays `error`) |
| `session_info_changed` | `hello` with the new name |
| `session_shutdown` | `bye` |

Identical consecutive reports are suppressed.
The activity line is visible to anyone near the desk.
Redact secrets before sending: `Bearer`/`Basic` credentials and the values of arguments or assignments whose key contains token, secret, password, passwd, api-key, or auth.
`PI_PET_DETAIL=0` reduces the line to the tool name.
Only the session name, state, and activity line leave pi. Prompts, file contents, and model output are never sent.

## Hub → device protocol

Text lines at 115200 over USB CDC:

- `pet <id> <state> <style> <name>|<detail>` upserts a pet.
- `unpet <id>` removes one pet; `unpet *` removes all.
- Diagnostics:
  - `pets` reports `GB_PETS n= focus= mood= manual_sleep= demo_bot=` plus `id:state:unseen` per pet.
  - `act left|right|enter|hold` is semantic input.
  - `reset`, `mood <name>`, `style <n>`, `expr <n>`, `look <x> <y>`, `bounce`, `spin`, `sound <pop|start|chime|wince|uhoh>`.
  - `shot` returns the raw RGB565 framebuffer.
  - `rec <frames> <x> <y> <size> <step>` / `recdump` records cropped frames at the loop rate for frame-by-frame review.

The device emits:

- `GB_READY` after boot.
- `GB_FOCUS <id>` whenever focus changes.
- `GB_ACK <id>` when the user acknowledges.
- Evidence lines `GB_HARDWARE`, `GB_TOUCHMAP`, `GB_SPEAKER`, `GB_FPS`, `GB_EVENT`, `GB_SOUND`.

## Portable state

`tests/contract.json` models three slots (`a`, `b`, `c`); the device holds eight with the same rules.
For each slot:

- `<slot>_state`: `none|idle|thinking|tool|error|done|waiting|sleeping`, default `none`
- `<slot>_unseen`: bool, default false. The pet finished, failed, or asked, and the user has not paged to it or acknowledged it.
- `<slot>_wince`: bool, default false. Set by every `error` report and cleared once 2.5 s have elapsed.
- `<slot>_age`: time in the current state, `fresh|over_4s|over_30s|over_10m`

Global fields:

- `focus`: `none|a|b|c`
- `mode`: `demo|pets`
- `manual_sleep`: bool
- `demo_bot`: 0–5
- `demo_mood`: mood
- `mood`: the displayed mood
- `emit`: the last device event of the action (`focus x`, `ack x`, or empty)
- `sound`: the transition sound the action plays (`none|pop|start|chime|wince|uhoh`)

Clock actions `advance_5s`, `advance_30s`, and `advance_10m` stand in for elapsed time.

## Behavior

1. **First pet.** The first report while nothing is focused focuses that pet (`GB_FOCUS`).
2. **State age.** A report that changes a pet's state resets its age. A repeated identical state keeps the age.
3. **Needs you.** Entering `done`, `error`, or `waiting` marks the pet unseen. If the focused pet is not working (`thinking`/`tool`), focus moves to it automatically. Automatic focus does **not** clear unseen.
4. **Paging.** Left and right page through pets in slot order and wrap. A user page clears the new pet's unseen flag and wakes manual sleep.
5. **Acknowledge.** A short both-button press clears the focused pet's unseen flag, wakes manual sleep, makes the pet hop, and emits `GB_ACK`.
6. **Manual sleep.** Holding both for 600 ms toggles manual sleep, which forces the sleeping state until a page or acknowledge.
7. **Removal.** Removing the focused pet focuses the next pet in slot order. Removing the last pet returns to demo mode and restores the demo bot and demo mood.
8. **Mood of the focused pet** (names follow the post's lifecycle):
   - Manual sleep → sleeping.
   - A wincing pet that is not `done` → surprised.
   - `thinking` → thinking; `tool` → working; `error` → blocked; `waiting` → waiting; `sleeping` → sleeping.
   - `done` → done (the celebration) while fresh; waiting if still unseen after 4 s; idle after 30 s.
   - `idle` or settled `done` → idle, or sleeping after 10 minutes.
9. **Demo mode** (no pets):
   - Left/right cycle the eight roster bots and wake a sleeping demo.
   - Short both cycles idle → happy → curious → excited → surprised → thinking → working → waiting → blocked → done → sad → idle; from sleeping it returns to idle.
   - Hold both toggles sleeping.
10. **Touch** uses the shared map:
    - Tap the pet to hop.
    - Drag the pet to move it; release springs it home with a wobble.
    - Tap elsewhere to make it glance there.
    - Any touch wakes manual sleep.
11. **Tilt:** the pet slides gently downhill and looks that way while wandering. A shake of more than 2.2 g makes it hop.

## Character and motion

The device ports the web avatar engine rather than imitating it. Measured behavior from the engine and from frame-by-frame captures of the post's live *Avatar motion system* demo:

- **Roster** (head shape, ink): blob `#1084FE`, pebble `#FF6700`, squircle `#00BCA6`, tablet `#FF263C`, wedge `#FF309B`, hex `#9159FE`, cloud `#FF9800`, teardrop `#97683D`.
  - Heads are built from the engine's definitions in a 228.54-unit box and normalized to a 228.44 box.
  - Each head has a fitted face region (offset, x/y scale, eye scale).
  - Style changes morph between heads.
- **Background** is pure black (`#000000`). AMOLED pixels are off there, which minimizes power and burn-in. If black smear (dark trailing behind moving edges) is visible on a particular panel, use near-black instead.
- **Eyes** are holes in the head, showing the background. Each of 25 expressions is a pair of 48-point contours positioned within the face region. Gaze offsets the eyes by up to ±15 / ±9 units.
  - A settled expression renders its true contour, curvature included, through an exact signed-distance texture of that contour.
  - While morphing between expressions, the reference itself interpolates capsule fits (center, axis, half length, half width); the device does the same.
  - Eye scale is capped so the two eyes never merge, and each eye's full extent stays inside its head.
- **Blink:** lid keyframes 0.05 at 0 ms, 0.05 at 70 ms, 1.08 at 150 ms, 1.0 at 300 ms, with a 14 % double blink.
  - The lid squashes the contour points along the reference's lid axis: the principal axis, blended toward vertical for round eyes. A closing pill therefore becomes a thin dash.
  - The device inverts that one-axis scale exactly and corrects the distance with the texture gradient, so squashed edges stay crisp.
- **Expression change:** blink through it. The eyes close to dashes, the expression swaps while shut, then they pop open.
- **Winks:** in idle, happy, curious, and excited, one eye dips for 320 ms every 4.5–10 s.
- **Springs** use damping-ratio form (ω, ζ) stepped at 1/120 s:
  - rotation (5, 0.9), x offset (3.5, 1), y offset (4, 1), vertical scale (10, 0.8);
  - lid (26, 1), eye scale (9, 0.85), gaze (13, 1);
  - body↔glyph morph (14, 1), head morph (10, 1), spin (6.2, 1).
- **Per-state motion:** each state sets targets for rotation, offsets, vertical scale, lid, and eye scale (for example, working bobs at 1.6 Hz; waiting sags half-lidded with an occasional sigh). It also picks its gaze range and interval, its expression set and interval, and its blink interval.
- **Bounce:** a decaying hop chain of heights 48, 28, 14, 6 units over 0.5, 0.382, 0.27, 0.177 s.
- **Spin:** the eyes travel around the head on a cylinder about a (possibly tilted) axis, foreshortening and disappearing around the back. Orbit ribbons trail the spin in parallel colored lanes.
  - Working spins every 6–9 s.
  - Done runs several tilted spins with ribbons, plus a bounce.
- **Glyph morphs** retain the head rather than cross-dissolving:
  - **Presence and style:** glyph presence `g` uses a frequency-14 critically damped spring. Style (dots ↔ `!`) uses a frequency-11 spring, so a direct Thinking ↔ Blocked change blends without an extra eye turn. With `bang = g·style` and `dots = g − bang`, body offsets, rotation, squash, and bounce scale by `1 − g`.
  - **Head:** the head rounds toward a circle, with cubic ease-in-out complete by 62 % of `g`. It contracts to scale `(1−g) + (r/C)·pop·g`, where `r` blends from 22 (Thinking) to 13 (Blocked). Its vertical offset is `−lift·dots + 58·bang·g`, and its opacity is `1 − (1 − tone)·dots`.
  - **Eyes:** the eyes contract with the head, turn cylindrically through half a turn on entry (frequency 14) with foreshortening and rear-face hiding, and are drawn only while `g < .5`. Exit adds the second half-turn in the same direction.
  - **Thinking pulse:** `phase = (t/1.4 + .119) mod 1`, and for each dot `s = exp(−d²/.045)`, where `d` is the cyclic distance to `j/3`. Then lift = `9·s·progress`, pop = `.84 + .22·s`, and tone = `.5 + .5·s`.
  - **Satellites:** progress `u = clamp((dots − .12·side)/(1 − .12·side))`. They grow with cubic ease-out and spread to ±62 with ease-out-back. Radius is `22·grow·pop·1.02` and opacity is `grow·tone`.
  - **Blocked stem:** a separate tapered stem (source path cap r 15 at −33, foot r 8.5 at +39.5) with `arrive = easeOut(1.1·bang)`, scale `clamp(1.2·bang)`, opacity `clamp(1.5·bang − .2)`, and drop `−26 − 70·(1 − arrive)`. Its shake is `2.2·sin(42t)·exp(−5.5·(t mod 2.2))` degrees about the cap. The dot pulses by `1 + .04·exp(−5.5·(t mod 2.2))·bang`.
  - **Compositing:** parts composite "over" each other with their own opacities.
  - **Pulse clocks:** each glyph keeps its own clock from its state's entry. This is a study adaptation so an outgoing pulse never jumps.
  - **Debounce:** glyph entry starts immediately in the reference. The device debounces Thinking by 350 ms so sub-second tool bursts don't flicker the face; this is a labeled adaptation.
- **Badge:** a blue dot (`#1D9BF0`) on the pet while it needs you.
- **Transition sounds:** synthesized at boot (22.05 kHz mono, about −7 dBFS, speaker volume 170/255), at most one per event, from any pet, not only the focused one:

  | Event | Sound |
  | --- | --- |
  | A new session appears | **pop** (70 ms upward sweep) |
  | A run starts: thinking/tool entered from idle, done, waiting, or sleeping | **start** (short tick) |
  | Done | **chime** (C6–E6–G6 bell arpeggio) |
  | Error | **wince** (short falling blip) |
  | Still in error when the 2.5 s wince ends | **uh-oh** (G4 → D4) |

  Flips between thinking and tool are silent, and manual sleep mutes all sounds. In demo mode, Enter auditions the new state's sound: done chime, blocked uh-oh, surprised wince, thinking/working start. Every played sound emits `GB_SOUND <name>`.
- **Sleeping:** closed-line expressions, drifting `z`, dim backlight.
- **Rendering:**
  - antialiased distance fields at every scale;
  - two-level tile classification with shading only near edges;
  - dirty-rectangle pushes;
  - at least 24 fps on the device during normal states.
- **Status band:**
  - Session name, bold, centered at y 390, inside [110, 376, 248, 28].
  - Activity line at y 417, trimmed with `...` to fit [116, 405, 236, 24]. When the detail is empty, it shows the state name.
  - One page dot per pet at y 438, inside [153, 430, 162, 16]. The focused dot is larger.
    - The fill is the pet's ink (identity), dimmed to 40 % while sleeping.
    - A ring shows state: working amber `#F5B13F`, thinking soft white `#D8D4CA`, error/blocked red `#FF4D4F` (pulsing while unseen), and finished/waiting-and-unseen pulsing blue `#1D9BF0`. Idle and seen pets have no ring.
  - The status band appears only in pet mode. In demo mode, a transient style or state label is shown instead.

The avatar shapes, expressions, and motion constants are xAI's design. Generated candidates derive them from the public page at build time; they are not committed to this repository.

## Failure behavior

- **No hub or board:** the extension retries quietly every 1.5 s and pi is unaffected. The hub rescans for an Espressif USB device (vendor `303a`) every 2 s.
- **Device resets** (opening the port may reset it): the hub resends the roster on `GB_READY`.
- **Flashing:** `/pet release` in pi, or `{"t":"release"}`, frees the port for 30 s. The device must be flashed app-only at `0x10000`, which preserves NVS and the touch map. A merged image at `0x0` is a fresh install that can erase calibration.
- **Malformed device lines** are rejected with `GB_ERROR` and do not change state. A full roster answers `GB_ERROR pets_full`.

## Roadmap (not in this contract)

- **Wi-Fi transport:** the same line protocol over a LAN socket, with the hub unchanged.
- **Text-to-speech:** a new `POST /v1/speech` route on `devices.chan.dev` backed by xAI.
  - The device authenticates with its AuthKit device session, as the other Devices routes do.
  - Short spoken lines such as "your turn" or "tests failed" play on the Stopwatch speaker.
  - This needs a `chan-services` change and its own contract.
- **Approvals:** pi's blocking `tool_call` answered by acknowledge (allow) or hold (deny).

## Acceptance layers

1. **Host contract:** `tests/run.sh`.
   - The shared runner checks three reducer shapes and rejects a mutation.
   - `tests/reference_model.py` audits every scenario against these rules and fuzzes invariants.
   - `tests/check_host.mjs` checks redaction and hub merging without a board.
2. **Compile evidence:** the pinned Arduino ESP32 3.3.10, M5Unified 0.2.19, and M5GFX 0.2.26 toolchain compiles the candidate for ESP32-S3 with OPI PSRAM.
3. **Injected-device evidence:**
   - `tests/check_device.py` replays every scenario that has no `given` state and no long clock on the real board.
   - Framebuffer captures cover every state, the demo, a working pet, and a finished pet; `rec` sequences support frame-by-frame comparison with the web demo.
   - A headless `pi -p` run through the real extension and hub drives thinking, working, and the error wince.
4. **Physical evidence:** [HAND-REVIEW.md](HAND-REVIEW.md).
