# Pi Pet results

## Revision 7: centering, tilt axes, and bulk dumps

September 25, 2026 (Pacific).

- **Defect (reported by the user):** the pet sat to the right. The IMU reported ax = −0.83 with the board standing upright on its lanyard, so its x axis runs toward the lanyard (screen down). The old code treated `ax` as horizontal tilt and slid the pet about 34 px right.
  - Tilt now uses the measured mapping (downhill = (`ay`, −`ax`)) and responds only to *changes* in orientation, through a 2.5 s baseline, so any resting pose stays centered.
  - Framebuffer measurements: body center (233.5, 232.0) in demo and (232.5, 204.0) with a pet, against the face center at x 233–234.
- **Vertical centering:** in demo and screensaver mode (no name band), the head now centers vertically at y 233. With pets it stays at y 204 above the band.
- **Defect found:** the zero USB write timeout from revision 5 made the 436 KB `shot` and `rec` dumps drop bytes, so readers hung. The dumps now raise the timeout to 2 s while a reader is attached, then restore 0.
- **Authoring:** `tests/author_contract.py` is now durable source. It regenerated a byte-identical `contract.json`.
- **Tests:** `tests/check_device.py` passed 16 scenarios with 687 checks (hub paused).

## Revision 6: screensaver and battery care

September 25, 2026 (Pacific).

- **Heartbeat and hub silence:** the hub pings every 5 s. After 20 s of silence the board cleared its stale roster (`GB_EVENT hub_gone`, observed at 20.0 s) and fell back to the demo bot. When the hub resumed, your live sessions returned over USB.
- **Screensaver:** with no sessions, the bot drifted silently through moods every 12 s. The first step waited out the one-minute interaction pause; injected button presses count as interaction. Demo and screensaver sounds are muted, including the former both-buttons audition.
- **Brightness:** dimming policy implemented (see SPEC). `GB_HELLO` reported `battery=100 charging=1` over USB.
- **Defect found:** the replay caught one. A session arriving on an empty board was muted because the "no sessions" check ran before it took focus. Sounds are now decided first and played after focus settles.
- **Tests:** the contract now has 89 steps (243,258 assertions per shape), with 40,089 model checks and 23 host-bridge checks. `tests/check_device.py` passed 16 scenarios with 687 checks, with the hub paused through its new `pause` message.
- **Compile evidence:** 1,609,919 bytes.
- **Not yet observed:** battery runtime with dimming, and the screensaver's look over hours.

## Revision 5: Wi-Fi and USB

September 24, 2026 (Pacific).

- **Provisioning:** the board already held Wi-Fi credentials in the shared `day22` slot from earlier network lessons, and joined the network on boot (192.168.0.214). The keychain path (`/pet wifi`) was therefore not needed and not exercised; it remains unverified. When the hub opened USB, it paired the board automatically, with no user step.
- **Direction:** a hub-side TCP listener was tried first. The board reached the port but got no reply, because the macOS application firewall (stealth mode) silently dropped incoming connections to `node`. The final design has the board listen and the hub dial out, which needs no firewall prompt.
- **Link:** the mutual HMAC handshake linked in about 3 s after hub start, and in 2 s after a board reboot.
- **Failover:** with USB released (as if unplugged), the hub switched to Wi-Fi and resynced in about 0.1 s. A session's pop, start, chime, wince, and uh-oh played from roster lines sent over Wi-Fi, and `pets` replies returned in about 0.1 s. USB resumed as the active link when it reopened.
- **Defect found:** with the host port closed, USB CDC writes blocked the main loop, and the first failover test showed a 25 s stall. The board now sets a zero USB write timeout.
- **Tests:** `tests/check_host.mjs` gained six Wi-Fi checks (23 total). `tests/check_device.py` still passes 16 scenarios with 687 checks (hub stopped).
- **Compile evidence:** 1,601,647 bytes, with the Wi-Fi stack.
- **Not yet observed:** physically unplugged battery operation, battery life, and `/pet wifi` with a keychain prompt.

## Revision 4: state rings and transition sounds

September 24, 2026 (Pacific).

- **Page dots:** each dot keeps the session's ink and adds a state ring (amber working, white thinking, red error, pulsing blue unseen). A device framebuffer with five pets showed each ring as specified.
- **Sounds:** five synthesized sounds (pop, start, chime, wince, uh-oh) mark new sessions, run starts, completion, errors, and persisting errors. Manual sleep mutes them, and demo Enter auditions them.
- **Contract:** the contract now models `sound`. `tests/run.sh` passed 243,136 assertions per reducer shape over 86 steps (mutation rejected), 40,086 reference-model checks, and 17 host-bridge checks.
- **Injected-device evidence:** `tests/check_device.py` passed 16 scenarios with 687 checks, including every `GB_SOUND`. The speaker initialized (`GB_SPEAKER ok=1`).
  - The replay first failed on a real defect, now fixed: `tick()` runs with a timestamp taken before serial input. An error stamped later underflowed the unsigned wince and uh-oh arithmetic, playing uh-oh at once, and a newly reported pet looked ten minutes old for one frame.
- **Live sessions:** all three herdr pi sessions registered after reloading (`.pi`, `chan-services`, `esptember`). They had simply predated the extension.
- **Compile evidence:** 990,083 bytes.
- **Not yet observed physically:** sound level and character, and ring legibility.

## Revision 3: retained-head glyphs, contour eyes, black background

September 24, 2026 (Pacific).
This revision followed the `grok-bot-esp32` handoff and the user's direction to "do the best you can."

- **Glyph transitions:** Thinking and Blocked now follow the reference formulas in `.agents/skills/grok-bot-esp32/references/glyph-morphs.md`, replacing the earlier blend of the head's outline into the glyph's:
  - the head rounds and contracts into the center dot, or drops into the lower dot;
  - satellites stagger and spread with overshoot;
  - the tapered stem drops, grows, and shakes;
  - the eyes half-turn in and out;
  - Thinking ↔ Blocked blends directly;
  - parts composite over one another.

  Device `rec` sequences of Idle→Thinking, Thinking→Blocked, and Blocked→Idle were reviewed frame by frame. The first sequences exposed two defects, both fixed and re-recorded: eyes not contracting with the head, and dark rectangles from proximity-weighted opacity.
- **Contour eyes:** the reference draws a settled eye's true 48-point contour; only mid-morph shapes are capsules.
  - The device now samples exact signed-distance textures of all 50 contours (182,684 bytes of flash), with the active pair copied into internal RAM.
  - An exact polygon evaluator was tried first and measured at 7–19 fps, so it was replaced.
  - A 25-expression device sheet matches the reference contour sheet, and a recorded blink shows the contour squashing into a thin dash.
- **Background:** pure black.
- **Performance (injected, including state-change label redraws):**

  | State | fps |
  | --- | --- |
  | idle | 23–31 |
  | thinking | ≈26 |
  | blocked | ≈27 |
  | done | ≈25 |
  | working | ≈24 |

  Done's choreography is unchanged.
- **Compile evidence:** 969,611 bytes of the 3,145,728-byte slot and 78,024 bytes of static RAM.
- **Tests:** `tests/check_device.py` passed 13 scenarios with 459 checks.
- **Not yet observed physically:** black-level smear on the AMOLED, and the new transitions' feel in hand.

## Revision 2: Grok Bot avatar engine port

September 24, 2026 (Pacific).
The character and motion were rebuilt from the avatar engine behind x.ai's *Designing Grok Bot*. The first revision had only imitated its look.

- **Reference evidence (not published):**
  - The engine's head definitions, 25 eye expressions, spring constants, per-state motion tables, blink keyframes, bounce chain, spin, and glyph morphs were read from the public page's component.
  - Chrome screencast captured the post's live *Avatar motion system* at 59 fps, 2,011 frames, covering idle, working, waiting (bored), blocked (alerting), thinking, and done (celebrate).
  - Those frames showed four things the first port missed:
    - blinks flatten the eye into a dash;
    - expression changes blink through;
    - spins trail parallel ribbon lanes;
    - the thinking dots carry a travelling highlight.
- **Host contract:** `tests/run.sh` passed:
  - 232,190 assertions per reducer shape over 16 scenarios (61 steps), with the mutation rejected;
  - 40,061 reference-model checks;
  - 17 host-bridge checks.

  States now use the post's names: working, waiting, blocked, done, surprised, sleeping, and more. There are eight roster styles.
- **Compile evidence:** 759,343 bytes of the 3,145,728-byte slot and 62,592 bytes of static RAM, including two 16 KiB head textures kept in internal RAM.
- **Injected-device evidence:**
  - `tests/check_device.py` passed 13 scenarios with 459 checks, with three skipped as host-only.
  - Measured frame rates:

    | State | fps |
    | --- | --- |
    | idle | 35–40 |
    | working | ≈34 |
    | thinking | ≈30 |
    | blocked | ≈34 |
    | done, with ribbons | ≈28 |

  - On-device `rec` sequences of working, thinking, blocked, and done were compared frame by frame with the web captures. Their images live in `.build/` and are not published.
- **Not yet observed physically:** ribbon rendering uses unantialiased quads for speed; their look on the AMOLED, and the overall motion feel, still need hand review.

## Revision 1

First revision: USB transport and the pet roster.

## Host contract

`tests/run.sh` generated three reducer shapes from `tests/contract.json`.
Each passed 232,051 assertions over 15 scenarios (57 steps) and 10,000 generated actions, and a reducer missing its first transition failed as required.
The reference model agreed with every scenario expectation and passed 40,057 checks, including fuzzed invariants.
The host bridge passed 17 checks: redaction cases, both leaks and over-redaction, plus hub merge, sanitizing, style assignment, crash removal, and the eight-pet cap on a temporary socket.

Writing the reference model exposed two contract decisions, which are now in SPEC.md:
- automatic focus does not mark a pet seen;
- a page press wakes a sleeping demo bot.

The host tests also found that the hub sanitized names only on the way to the device; it now stores the sanitized form.

## Compile evidence

The pinned Arduino ESP32 3.3.10, M5Unified 0.2.19, and M5GFX 0.2.26 toolchain compiled the disposable candidate for ESP32-S3 with OPI PSRAM.
It uses 609,771 bytes of its 3,145,728-byte application slot and 28,544 bytes of static RAM.

## Injected-device evidence

Every load was an app-partition-only write at `0x10000`.
The partition table was compared with the build first, and it matched.
After the final load, the device reported:
- board 30 (Stopwatch), 468 × 466, touch enabled, 8 MiB PSRAM;
- shared touch map **version 2, generation 2**, unchanged.

- **Scenario replay:** `tests/check_device.py` passed 12 scenarios with 427 checks on the real board. Three scenarios that need a `given` state or 30 s / 10 min clocks remain host-only. The first device runs caught two firmware bugs that the host contract had specified correctly: removing the last pet kept that pet's mood and style instead of restoring the demo look.
- **Frame rate:** 29–30 fps in demo mode (about 20 ms render, 31 ms per frame including push). Pet mode with the status band measured 25–29 fps. Before tile classification, the same scene rendered at 9 fps.
- **Framebuffers:** captures of the eight moods, the demo, a busy pet with three page dots, and a finished pet show the status band inside the round face. They are in `.build/evidence/`, which is not published.
- **Live pi:** headless `pi --no-session -p` runs through the installed extension started the hub on demand. The pet went `thinking → busy → thinking` for each bash call. A failing `ls` produced `worried` for 2.5 s before returning to `busy`, where a 36 ms `thinking` blip is visible in the monitor stream. The pet left when the session ended.

## Known gaps

- The candidate does **not** yet include the shared runtime touch-calibration flow that the platform contract requires. It loads and applies the stored map but offers no recalibration path.
- The IMU axis mapping for tilt and downhill slide was not confirmed physically.
- Redaction over-redacts harmless words such as `auth login`, which fails safe.
- No physical review has been recorded; see [HAND-REVIEW.md](HAND-REVIEW.md).
