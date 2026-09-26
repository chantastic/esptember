# Pi Pet testing model

Run the portable suite:

```sh
days/day-34-pi-pet/tests/run.sh
```

It runs four checks:

1. **Contract.** It generates three reducer shapes from `tests/contract.json` and checks each with the shared runner. That covers defaults, bounds, determinism, nonmutation, every scenario step, 10,000 generated actions, Stopwatch metadata, the C25K control profile, and round-screen bounds for the pet body, name, activity line, and page dots. It then drops one transition and requires that mutation to fail.
2. **Reference model.** Scenarios are written in `tests/author_contract.py`, which fills every expectation from the model and aborts on any claim that disagrees; `contract.json` is its output. `tests/reference_model.py` is an executable reading of SPEC.md. It requires every scenario expectation to be complete (no silently omitted field changes) and to agree with the written rules. It then fuzzes 40,000 random actions for these invariants:
   - focus exists exactly when pets exist;
   - focus points at a pet;
   - moods stay valid;
   - empty slots keep no flags.
3. **Host bridge.** `tests/check_host.mjs` runs when a host candidate exists under `.build/host-candidate/pi-pet`. It checks the exported `redact()` against `tests/redaction_cases.json`, which covers both leaks and over-redaction. It then starts the real hub on a temporary socket with `PI_PET_PORT=none` and checks:
   - session merge;
   - id derivation;
   - field sanitizing;
   - unknown-state fallback;
   - distinct styles;
   - removal on socket close and on `bye`;
   - the eight-pet cap;
   - the Wi-Fi link against a fake board on localhost: the hub proves the token, a wrong key is refused, a good key links and resyncs, roster traffic flows, and nothing is provisioned over Wi-Fi.

This suite needs neither the board nor pi.
It does not prove rendering, frame rate, touch alignment, the physical controls, IMU behavior, or pi's event timing.

## Device layers

The pi-pet hub owns the port. Before `check_device.py`, send it `{"t":"pause","seconds":400}` so it leaves both USB and Wi-Fi alone; running sessions respawn a killed hub. Resume it with `{"t":"resume"}` afterwards. A released hub fails over to Wi-Fi and would drive the board during the replay; `/pet release` is enough for flashing.

- **Compile and flash:** compile with the pinned toolchain and flash **app-only at `0x10000`**. This preserves NVS and `espt-touch/record`.
- **Replay:** `python3 tests/check_device.py` (needs pyserial) replays each scenario without a `given` state or a 30 s / 10 min clock. It checks the device's `pets` report and its emitted `GB_FOCUS`/`GB_ACK`/`GB_SOUND` lines after every step. Waits stand in for `advance_5s`.
- **Framebuffer:** `shot` returns the raw 468 × 466 RGB565 canvas, byte-swapped as M5GFX stores it.
- **Live pi:** a headless `pi --no-session -p "..."` run through the installed extension should produce `thinking → working → thinking`, plus `surprised` for a failing bash command, in the hub's monitor stream.
- **Wireless failover:** with the board paired and the hub running, send `{"t":"release"}` and drive a fake session. Roster updates, `GB_SOUND`, and `pets` replies must arrive tagged `via wifi` on the hub's monitor stream within about 0.2 s, and USB must return as the active link after the release window.
- **Motion review:** `rec <frames> <x> <y> <size> <step>` then `recdump` returns cropped frames at the loop rate. Compare them with screencast frames of the post's live demo.

Finish with [HAND-REVIEW.md](HAND-REVIEW.md) on the attached Stopwatch.
