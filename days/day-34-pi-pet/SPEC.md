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
pi session ─┐   JSON lines over          ┌──────────┐ ── USB serial (trusted) ──► ┌───────────┐
pi session ─┼─► ~/.pi-pet/hub.sock  ────►│ pet hub  │ ── Wi-Fi TCP (paired) ────► │ Stopwatch │
pi session ─┘   (one extension each)     └──────────┘ ◄──── GB_* events ───────── └───────────┘
```

- **Extension:** one per pi process, loaded from `~/.pi/agent/extensions/pi-pet/`. It must never throw into pi or block an agent turn. `PI_PET=0` disables it.
- **Hub:** a single local process that owns the serial port, dials paired boards over Wi-Fi, and merges every session. The first extension that cannot connect starts it detached. It exits after 5 minutes with no clients. It opens no listening network port.
- **Device:** renders the focused pet and drives mood from the reported state.

Text-to-speech is a later phase (see *Roadmap*). Transports must not change the state rules below.

## Transports: USB and Wi-Fi

- **The same line protocol runs over both.** A board may be connected by USB, by Wi-Fi, or both.
  - Per board, the hub uses one **active** link, preferring USB, and keeps the other on **standby**.
  - It writes roster lines only to the active link and ignores device events arriving on a standby link.
  - When the active link changes, the hub resyncs the new one (`unpet *` plus every pet).
- **USB is the trusted provisioning channel.** When a board is on USB, the hub asks `hello` and reads its `GB_HELLO`:

  ```text
  GB_HELLO id=<12 hex> paired=<0|1> key=<8 hex|-> wifi=<none|saved|connected> ip=<ipv4|-> hub=<ip:port> link=<wifi|-> err=<code>
  ```

  - `key` is the first 8 hex of SHA-256(token).
  - If the board is unpaired, or its key doesn't match the hub's record, the hub sends `pair <64-hex token> <host> <ip> <port>`.
  - The hub records the board's `ip` for dialing, in `~/.pi-pet/devices.json` (mode 600).
  - Provisioning commands (`wifi`, `pair`, `hubaddr`, `forget`) are accepted only from USB. Over Wi-Fi they answer `GB_ERROR usb_only`.
- **Wi-Fi credentials** live in the shared ESPtember Wi-Fi slot (Preferences `day22`: `ssid`, `pass`). The Day 22 portal or any earlier network lesson may have set them already.
  - `/pet wifi [ssid]` reads the password from the Mac's login keychain; macOS asks the user to allow it.
  - With no argument, it uses the first preferred network (`networksetup -listpreferredwirelessnetworks en0`).
  - It sends `wifi <base64 ssid> <base64 password>` over USB. The password is never logged or echoed, and the board confirms with `GB_WIFI saved ssid_len=<n>`.
- **Wi-Fi link:** the board joins Wi-Fi with modem sleep enabled, advertises `pi-pet-<last 4 hex of id>.local` and `_pi-pet._tcp`, and listens on TCP 47837.
  - The hub dials each paired board every 3 s while it has no Wi-Fi link, alternating the last known IP and the `.local` name.
  - Outgoing connections pass the macOS application firewall without prompting. A hub-side listener was tried first and was silently dropped by the firewall in stealth mode.
- **Handshake** (mutual HMAC-SHA256 with the pairing token as key; all hex):

  ```text
  board → HELLO <id> <nonceD:16>
  hub   → CHALLENGE <nonceH:32> <HMAC(token, "hub:" nonceD ":" nonceH)>   (or DENY unpaired)
  board → AUTH <HMAC(token, "dev:" nonceH ":" nonceD)>                   (board checks the hub's MAC first)
  hub   → OK                                                             (or DENY auth; timing-safe compare)
  ```

  - Afterwards, protocol lines flow both ways, and every `GB_*` line the board prints is mirrored to the link.
  - A newly authenticated connection replaces an old one, which covers a restarted hub.
  - Traffic is authenticated but not encrypted. It carries session names and redacted activity lines on the local network only.
- **USB never blocks the board.** The board sets its USB CDC write timeout to 0. Otherwise a closed host port (a released hub or a charger-only connection) stalled the main loop, and with it the Wi-Fi link, for about 25 s.
- **`/pet forget`** clears the board's pairing (`GB_FORGOT`) and the hub's records. Plugging in over USB pairs again automatically.

## Host → hub protocol

Newline-delimited JSON on the Unix socket `~/.pi-pet/hub.sock`, overridable with `PI_PET_SOCKET`:

| Message | Meaning |
| --- | --- |
| `{"t":"hello","id","name","cwd"}` | Register or rename a session. The name defaults to the cwd basename. |
| `{"t":"state","id","state","detail"}` | Report a state and a short activity line. |
| `{"t":"bye","id"}` | The session ended. |
| `{"t":"status"}` | Hub replies with the port, readiness, and the merged roster. |
| `{"t":"release"}` | Close the serial port for 30 s so a flasher can use it. Wi-Fi stays up. |
| `{"t":"wifi","ssid"?}` | Send the keychain Wi-Fi credentials to the board over USB; replies `{"t":"result","ok","message"}`. |
| `{"t":"forget"}` | Unpair every board; replies `{"t":"result",...}`. |
| `{"t":"pause","seconds"}` / `{"t":"resume"}` | Tests: leave the board entirely alone (no USB, no Wi-Fi) for a while. Required before `check_device.py`, because sessions respawn a killed hub. |
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
- USB only: `hello`, `pair`, `hubaddr`, `wifi`, and `forget` (see *Transports*). `hello` is also answered over Wi-Fi.
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
- Link lines `GB_HELLO`, `GB_PAIRED key=`, `GB_WIFI saved ssid_len=`, `GB_FORGOT`.

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

Clock actions `advance_5s`, `advance_30s`, and `advance_10m` stand in for elapsed time. `hub_silent_20s` stands for 20 s without hub traffic.

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
9. **Hub silence:** the hub sends `ping` every 5 s on the active link. After 20 s with no line from the hub (action `hub_silent_20s`), the board clears the roster as if `unpet *` arrived and emits `GB_EVENT hub_gone`. The hub resyncs everything when it reconnects.
10. **Demo mode is the screensaver** (no pets):
   - Left/right cycle the eight roster bots and wake a sleeping demo.
   - Short both cycles idle → happy → curious → excited → surprised → thinking → working → waiting → blocked → done → sad → idle; from sleeping it returns to idle. It is silent.
   - Without buttons or touch for a minute, it advances every 12 s through idle, happy, curious, thinking, working, excited, waiting, done, surprised (never blocked or sad), and changes head style every third step.
   - All sounds are muted while there are no sessions.
   - Hold both toggles sleeping.
11. **Touch** uses the shared map:
    - Tap the pet to hop.
    - Drag the pet to move it; release springs it home with a wobble.
    - Tap elsewhere to make it glance there.
    - Any touch wakes manual sleep.
12. **Tilt:** the pet reacts to *changes* in orientation, never to the resting pose.
    - Downhill on the screen is (IMU `ay`, −`ax`); this mapping was measured on the device (see *Character and motion*).
    - A baseline follows it with a 2.5 s time constant. The difference (deadband 0.05 g, gain 1.5) slides the pet up to 40 px horizontally and 30 px vertically, and steers its glances. The pet drifts back to center as the baseline settles, so it stays centered lying flat, standing upright on its lanyard, or leaning.
    - A shake of more than 2.2 g makes it bounce and spin.

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

  Flips between thinking and tool are silent. Manual sleep and the demo screensaver mute all sounds. Review them with `sound <name>`. Every played sound emits `GB_SOUND <name>`.
- **Brightness** (battery care): any button or touch restores full brightness for a minute.
  - Screensaver: 255 for its first minute, 120 until ten minutes, then 50.
  - Pets: dims to 120 when no pet has been working, finished, or waiting on you for 5 minutes.
  - Sleeping: 70.
  - `GB_HELLO` reports `battery=<percent> charging=<0|1>` for runtime measurements.
- **Sleeping:** closed-line expressions, drifting `z`, dim backlight.
- **Placement:** the head's home is x 234 (the round face's center). Vertically, it sits at y 204 when the name/activity band shows (pet mode) and at y 233, centered, in demo/screensaver mode, easing between the two in about 0.3 s. Measured on the device: body center (233.5, 232.0) in demo and (232.5, 204) with a pet.
- **IMU axes** (Stopwatch, lanyard down, standing upright: ax ≈ −0.83): IMU +x points toward the lanyard (screen down); with +z out of the screen, +y points screen-left.
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
- **Flashing:** `/pet release` in pi, or `{"t":"release"}`, frees the port for 30 s, and the pets continue over Wi-Fi. The device must be flashed app-only at `0x10000`, which preserves NVS (touch map, Wi-Fi, and pairing). A merged image at `0x0` is a fresh install that can erase them.
- **Wi-Fi loss:** a dropped or unauthenticated link falls back to USB when present. The hub keeps redialing, and the board keeps rejoining its network every 15 s.
- **Malformed device lines** are rejected with `GB_ERROR` and do not change state. A full roster answers `GB_ERROR pets_full`.

## Roadmap (not in this contract)

- **Battery care:** dim or sleep when every session is idle, and consider Bluetooth LE for lower power once the display's budget is known.
- **Text-to-speech:** a new `POST /v1/speech` route on `devices.chan.dev` backed by xAI.
  - The device authenticates with its AuthKit device session, as the other Devices routes do.
  - Short spoken lines such as "your turn" or "tests failed" play on the Stopwatch speaker.
  - This needs a `chan-services` change and its own contract.
- **Approvals:** pi's blocking `tool_call` answered by acknowledge (allow) or hold (deny).

## Acceptance layers

1. **Host contract:** `tests/run.sh`.
   - The shared runner checks three reducer shapes and rejects a mutation.
   - `tests/reference_model.py` audits every scenario against these rules and fuzzes invariants.
   - `tests/author_contract.py` generates `contract.json` from compact scenarios, filling complete expectations from the model. Edit scenarios there, not in the JSON.
   - `tests/check_host.mjs` checks redaction and hub merging without a board. It also checks the Wi-Fi link against a fake board on localhost: the hub proves the token, a wrong key is refused, a good key links and resyncs, roster traffic flows, and no provisioning goes over Wi-Fi.
2. **Compile evidence:** the pinned Arduino ESP32 3.3.10, M5Unified 0.2.19, and M5GFX 0.2.26 toolchain compiles the candidate for ESP32-S3 with OPI PSRAM.
3. **Injected-device evidence:**
   - `tests/check_device.py` replays every scenario that has no `given` state and no long clock on the real board.
   - Framebuffer captures cover every state, the demo, a working pet, and a finished pet; `rec` sequences support frame-by-frame comparison with the web demo.
   - A headless `pi -p` run through the real extension and hub drives thinking, working, and the error wince.
   - Wireless failover: release USB while the board is paired, then drive a session. Roster updates, sounds, and `pets` replies must arrive over the Wi-Fi link, and USB must resume as active when it returns. Stop the hub entirely, not just release it, before `check_device.py`; otherwise the hub drives the board over Wi-Fi during the replay.
4. **Physical evidence:** [HAND-REVIEW.md](HAND-REVIEW.md).
