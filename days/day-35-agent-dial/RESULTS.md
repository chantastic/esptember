# Agent Dial results

## Revision 2: LVGL candidate on the attached board

September 26, 2026 (Pacific).

- **Board:** a factory Stopwatch (MAC 28:84:85:44:60:98, ESP32-S3 rev 0.2, 8 MB PSRAM, 16 MB flash).
  - Its factory partition table puts the app at `0x20000` with a 16 KB NVS, so an app-only write at `0x10000` could not boot.
  - The full 16 MB flash was backed up first (SHA-256 `ac7643d6fc96dca447576d3c762f90deb3a5dc3a3d21f09dd083840c43e5b7ed`, kept under the ignored `.build/factory-backup/`).
  - The first flash was a merged image at `0x0`, a fresh install that erased NVS. Every later flash was app-only at `0x10000`, and the pairing token survived each one.
- **Review finding (the user, on the device):** the first candidate drew with M5GFX primitives and bitmap fonts, and "the graphics are a little whack."
  - Upstream renders with LVGL 9.5 and Geist.
  - The prompt now requires LVGL 9.5.0 with upstream's Geist faces, regenerated over a narrower range, and upstream's icons.
  - SPEC's provenance table records the range and the simplified Settings rows.
- **Defects found by the device replay, now rules in the prompt:**
  - The loop sampled `now` before input handlers stamped `millis()`, so `now - stamp` underflowed.
    - The dial slept at once, a recording stopped the moment it began, and routing timed out immediately.
    - The hub-silence check would have dropped a live hub.
    - Every stamped timer now uses a signed difference against `millis()`.
  - `AD_FPS` reported 576 bytes free on the default 8 KB loop stack. The loop now runs on 16 KB.
  - Captures lagged one state behind a pending rebuild. `diag.shot` now rebuilds first.
- **Host defect found live:** a backlog of `hello` greetings at port open made the hub pair five times in 2 ms. The last token matched, but the hub now waits 5 s after sending `pair`.
- **Tests:**
  - `tests/run.sh`: three shapes at 307,479 assertions each; mutation rejected; 40,183 model checks; 190 frame checks; 133 host-bridge checks.
  - The device codec (`dial_frame.h`), compiled on the Mac, passes 46 vector checks.
  - `tests/check_device.py`: 31 scenarios, 4,575 checks, 0 failures.
- **Compile evidence:** 1,928,519 bytes (61 %) with LVGL 9.5.0, ArduinoJson 7.4.2, M5Unified 0.2.19, and M5GFX 0.2.26. The libraries were installed into a lesson-local `ARDUINO_DIRECTORIES_USER`, because this session could not read `~/Documents/Arduino`.
- **Live, through the real hub on USB:**
  - **CLI:** `agent-dial event` showed a working tile, with `--password hunter2` redacted. `agent-dial ask` opened a question with a beep, and the dial's answer (injected over USB) returned `Ship` with exit 0.
  - **Claude Code 2.1.283**, headless:
    - With the hooks from `agent-dial install-claude`, the session appeared and left with `bye`.
    - With `--permission-mode default`, its `PermissionRequest` opened Allow/Deny on the dial within 4 s. The dial's Allow let `date > when.txt` run.
    - The hook input carried no `tool_use_id`, now recorded in SPEC's failure behavior.
  - **pi**, headless with `-e index.ts`: the session appeared, the dial beeped on its summary, and it left with `bye`.
- **Not yet observed:**
  - physical touch and pusher review, and the shared calibration flow run to completion;
  - the microphone and a real `AGENT_DIAL_STT` command;
  - the speaker's tone;
  - direct Wi-Fi (this board has no stored network);
  - battery life;
  - `turn.stop` and `turn.send` reaching a live pi turn.

## Revision 1: lesson source

September 26, 2026 (Pacific).

- **Source:** README prompt, SPEC.md with a provenance table against upstream `4689799`, contract (30 scenarios, 175 steps), reference model, frame vectors, hand review.
- **Host contract:** `tests/run.sh` passed. Three reducer shapes: 307,171 assertions each. Mutation rejected. Reference model: 40,175 checks. Frame vectors: 190 checks.
- **Not yet run:** host bridge, compile, injected-device, and physical layers.
