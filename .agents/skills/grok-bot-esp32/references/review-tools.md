# Frame-by-frame review tools

Motion fidelity needs matched frame sequences from the reference and the device, not single stills. The tools here were used on September 24, 2026; the scripts are bundled in `scripts/`.

## Reference: the live web demo

The article's **Avatar motion system** runs the real procedural engine. It cycles Idle → Working → Waiting (`bored`) → Blocked (`alerting`) → Thinking → Done (`celebrate`), roughly 4.2–4.6 s each.

```sh
mkdir -p /tmp/gb-capture && cd /tmp/gb-capture && npm i puppeteer-core@23
cp <skill>/scripts/capture-web-demo.mjs . && node capture-web-demo.mjs frames 34 420
python3 <skill>/scripts/web-contact-sheets.py frames sheets 4 48
```

- **What the capture does:**
  - It uses the installed Google Chrome, headless, and enlarges the demo SVG to 420 px so eye shapes are legible.
  - It records a CDP screencast at about 55–60 fps, logging each frame's `data-state`.
  - The contact sheets give one PNG per state, every 4th frame (about 15 fps), stamped with milliseconds since state entry.
- **Guide videos** (`x.ai/bot/guides/designing-grok-bot-with-grok-bot`): curl and direct navigation to `media.x.ai` return **403**, and cross-origin fetch is blocked. Seeking the in-page `<video>` and screenshotting it does work, but the bot is tiny inside UI demos. For eye motion, prefer the live demo.
- **Saved sheets:** `assets/review/web-*.png` are sheets from one captured cycle. They are reference evidence of motion, not assets to ship.

What the frames established that source reading alone did not:
- blinks flatten pills into thin dashes;
- expression changes blink through;
- spins trail parallel multicolor ribbon lanes;
- Thinking's highlight travels through size, lift, and brightness together;
- Blocked's stem drops in and shakes.

## Device: the firmware's recorder

The Pi Pet candidate accepts serial diagnostics.

| Command | Purpose |
| --- | --- |
| `mood <state>` | Set the demo state: idle, thinking, working, waiting, blocked, done, surprised, sleeping, happy, curious, excited, sad. |
| `style <n>` | Head style 0–7. |
| `expr <n>` | Force an expression (0–24), held for 60 s. |
| `look <x> <y>` | Hold gaze, in the range −1…1. |
| `blink`, `spin`, `bounce`, `burst`, `reset` | Trigger the corresponding motion, or reset. |
| `shot` | Full framebuffer dump. |
| `rec <frames> <x> <y> <size> <step>`, then `recdump` | Record cropped, downsampled frames into PSRAM at the loop's own rate, then dump them. |

```sh
python3 <skill>/scripts/device-recorder.py rec out.png 60 "mood thinking" 0.2
python3 <skill>/scripts/device-recorder.py shot out.png "expr 20;look 0 0" 1
```

- **Default crop:** `--box 84,54,300,2` (the head sits at y 204 with pets and y 233 in demo; adjust `y` for demo shots), a 300 px square around the head downsampled 2×, gives about 45 KB per frame. The recorder captures at the real frame rate because it doesn't block the loop, unlike `shot`: a full dump takes about 0.5 s and ruins any concurrent fps sample.
- **Port ownership:** the pi-pet hub owns the USB port and, when the board is paired, also drives it over Wi-Fi. Send the hub `{"t":"pause","seconds":300}` on `~/.pi-pet/hub.sock` before recording, and `{"t":"resume"}` after. `/pet release` frees only USB, and killing the hub doesn't stick, because running sessions respawn it. Opening the port can reset the board; the script waits for it.
- **Saved sequences:** `assets/review/device-*.png` hold device sequences from the revision-3 candidate.
  - Recorded with contour eyes: the Working spin, Thinking→Blocked, Done, a blink, and all 25 settled contours.
  - Recorded after the compositing fix but before contour eyes (capsule eyes): Idle→Thinking and Blocked→Idle.

## Comparing

- **Line up by time since state entry.** Compare these channels separately:
  - eye shape and pose;
  - blink timing and shape;
  - gaze;
  - body rotation, offset, and squash;
  - spin visibility;
  - ribbon depth;
  - glyph geometry.
- **Artifacts are defects.** Blocky patches, hollow eyes, seams, and dark streaks came from compositing, classification, or morph-space bugs, not from reference behavior. Every one found so far was visible in a recorder sequence before it was noticed live.
- **Keep evidence layers separate.** Browser study, web capture, device recorder, and physical review are distinct evidence layers. Record each where the lesson keeps results (`RESULTS.md`); don't promote one to another.
