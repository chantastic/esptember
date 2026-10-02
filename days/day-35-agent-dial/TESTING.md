# Agent Dial testing model

Run the portable suite:

```sh
days/day-35-agent-dial/tests/run.sh
```

1. **Contract.** `scripts/prompt-contract/validate_day.sh` generates three reducer shapes from `tests/contract.json`, checks each with the shared runner (defaults, bounds, determinism, nonmutation, every scenario step, 10,000 generated actions, Stopwatch metadata, the C25K profile, and round-screen bounds for the listed controls), then requires a mutated candidate to fail.
2. **Reference model.** Scenarios live in `tests/author_contract.py`, which fills every expectation from `tests/reference_model.py` and aborts on a disagreeing claim. The model is an executable reading of SPEC.md's *Behavior*. It re-audits the contract and fuzzes 40,000 actions for invariants:
   - focus exists exactly when a session does, and points at one;
   - the voice overlay is up exactly while voice runs, and voice and its target agree;
   - no sound while the microphone runs;
   - a question screen belongs to the focused asking session;
   - a question row exists exactly while its session is asking;
   - the badge counts the rows.
3. **Frame vectors.** `tests/gen_frame_vectors.py` writes `tests/vectors/dial_frame.txt` from the format description. `tests/check_frames.py` replays it through a second, streaming decoder fed in random-sized pieces. Upstream's `A5 48` frames must be ignored.
4. **Host bridge.** `tests/check_host.mjs` runs when `.build/host-candidate/agent-dial/hub.mjs` exists. It checks the hub's codec against the vectors, `redact()` against `tests/redaction_cases.json`, and a hub on a temporary socket with `AGENT_DIAL_PORT=none`:
   - merge and harness-prefixed ids;
   - field caps;
   - ttl expiry;
   - capability-gated commands;
   - question close on client exit;
   - the Claude Code hook's `PermissionRequest` output.

This suite needs neither the board nor a harness.
It does not prove rendering, touch alignment, the pushers, audio, Wi-Fi, or any harness's event timing.

## Device layers

The hub owns the port. Before `check_device.py`, send `agent-dial pause 600` (or `{"t":"pause","seconds":600}` on the socket). Resume afterwards. Day 34's pi-pet hub also scans vendor `303a`; pause it too. The device scripts use only the Python standard library.

- **Compile and flash:** the pinned toolchain; flash **app-only at `0x10000`** to keep `espt-touch/record`, Wi-Fi, and pairing.
- **Replay:** `python3 tests/check_device.py` (pyserial) sends `diag.act` for every step of every scenario without a `given` state and compares `diag.state` after each.
- **Framebuffer:** `python3 tests/capture_screens.py OUT_DIR` drives every screen with hub-shaped messages and saves each `diag.shot` as a PNG.
- **Device codec:** compile `dial_frame.h` on the host against `tests/vectors/dial_frame.txt` (the candidate's `.build/codec-test/test_frame.cpp`).
- **Live:** a Claude Code `PermissionRequest` answered on the dial, and a pi turn observed through the hub's monitor stream.

Finish with [HAND-REVIEW.md](HAND-REVIEW.md).
