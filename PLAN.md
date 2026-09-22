# ESPtember — 30-day plan

Working plan for the remaining days. Days 1–6 and 12 are shipped; everything
else is planned. Each day still gets its own SPEC before implementation —
this file records the lineup, the reasoning, and the constraints we've agreed.

Boards: `waveshare-amoled-18-v2` ("1.8"), `waveshare-amoled-175-b` ("1.75-B"),
`m5stack-stopwatch` ("Stopwatch"). Board balance: 1.8 ×14, Stopwatch ×10,
1.75-B ×3, multi-board ×3. (Days 9 and 15 retargeted to the Stopwatch
— the 1.75-B is missing in action; it keeps 20, 22, 23.)

## Lineup

| # | Lesson | Board | Status |
|---|--------|-------|--------|
| 1–6 | Hello World → Movie | 1.8 | shipped |
| 7 | LVGL Basics | 1.8 | planned |
| 8 | Sound Effects Board | 1.8 | planned |
| 9 | Level | Stopwatch | planned |
| 10 | Buttons, Buzzer, Haptics | Stopwatch | planned |
| 11 | Basic-Ass Stopwatch | Stopwatch | planned |
| 12 | C25K | Stopwatch | shipped |
| 13 | Morse Code Practicer | Stopwatch | planned |
| 14 | Sound Level (dB) | 1.8 | planned |
| 15 | Music Visualizer | Stopwatch | planned |
| 16 | Tamagotchi | Stopwatch | planned |
| 17 | Pokedex | 1.8 | planned |
| 18 | Metronome | Stopwatch | planned |
| 19 | Tempo Checker | Stopwatch | planned |
| 20 | Yo — ESP-NOW pager | all three | planned |
| 21 | Captive Portal Provisioning | 1.8 | planned |
| 22 | Weather Station | 1.75-B | planned |
| 23 | Walkie-Talkie: local (ESP-NOW) | Stopwatch + 1.8 | planned |
| 24 | Walkie-Talkie: internet | Stopwatch + 1.8 | planned |
| 25 | Flight Radar | 1.75-B | planned |
| 26 | Package Tracker | 1.8 | planned |
| 27 | Real-Time Captions (STT) | 1.8 | planned |
| 28 | Note Taker (voice memos) | Stopwatch | planned |
| 29 | Trivia Capstone | all three | planned |
| 30 | WorkOS AuthKit — Device Grant | 1.8 | planned |

Arc logic: display/assets (1–6) → UI + per-board I/O taught once each (7–11) →
timing capstones (12–13) → audio-in (14–15) → persistence/assets at scale
(16–17) → local radio (18, 21) → provisioning gateway (19) → internet arc with
a hidden ramp of fetch → poll+render → authenticate → write (20, 22–27) →
audio+API composition (28) → capstones (29–30).

## Day details

### 7. LVGL Basics
Widgets, screens, touch input. The enabling lesson for everything after —
lists, keyboards, pagination all trace back here. Days 1–6 used LVGL for
rendering; this day teaches it as an interactive UI toolkit.

### 8. Sound Effects Board
Touch pad grid + first audio out through the ES8311 codec. **Paginated: 3 pads
per page** with swipe/arrows — sneaky LVGL reinforcement. Sound stingers built
here get reused by trivia (29) and the Morse sidetone (13).

### 9. Level
IMU (QMI8658) → live bubble. Model on the **Apple Watch Ultra level**:
2-D bubble when flat, edge-strip inclinometer when near-vertical, snap-to-green
with a haptic/visual tick at 0°. First sensor lesson, first round-display
lesson.

### 10. Buttons, Buzzer, Haptics
Introduces the Stopwatch board: no touch — physical input with non-visual
feedback. Sequence: raw GPIO reads with visible bounce trace → debounce →
press vocabulary (short, long, hold-repeat, chords) → buzzer tones + vibration
via `M5.Power.setVibration()` → tiny no-look menu payload. Day-12's
`buttons.h` (80 ms chord window, 98 portable assertions) is this lesson's
machinery, extracted and taught. Settle the open NOTES question here: measure
the real max loop gap vs. the 80 ms chord window.

### 11. Basic-Ass Stopwatch
Real pusher ergonomics, matching mechanical/Casio convention:
- Top/crown pusher = start/stop, always; never resets.
- Second pusher = lap while running, reset while stopped; reset is a **hold**.
- Lap freezes the displayed split while the timer runs; lap list on the round
  face.
Guide cites used resources: a mechanical stopwatch operation reference, Casio
digital stopwatch-mode docs (lap/reset overload), M5Unified button naming.
Hook: the kit is shaped like a stopwatch — make it behave like one.

### 13. Morse Code Practicer
Pure application of 10–11's timing skills. ITU-R M.1677-1 timing (cite it):
dit = 1 unit, dah = 3, intra-char gap 1, letter gap 3, word gap 7; unit ms =
1200/WPM (PARIS convention). Straight-key on one pusher (duration decides
dit/dah); stretch: iambic paddles on both. Live display of the raw `·−·−`
stream *and* decoded characters as gaps elapse; reference chart screen; buzzer
sidetone. Practice loop: target character, score keying against ITU
tolerances, show timing error ("your dit was 1.4 units"). Decoder is a pure
timing state machine — portable-test it like C25K's buttons.

### 14. Sound Level (dB)
Mic in, RMS → dB meter. **Uncalibrated MEMS mic ⇒ relative dB only** — the
guide must say so (or calibrate against a reference SPL meter). Mirrors day
8's audio-out.

### 15. Music Visualizer
Mic + FFT → radial spectrum on the round face. Builds on 14's capture code.

### 16. Tamagotchi
Clean-room reimplementation of the 1996 **Tamagotchi P1** mechanics (the ROM
is Bandai's; TamaLIB/MCUGotchi and the P1 disassembly docs are references,
not dependencies):
- Life cycle: egg → baby → child → teen → adult → departure; adult form
  determined by care-mistake count.
- Hunger + Happiness as 4-heart meters; feed (meal vs. snack), play (L/R
  guessing game), clean poop, medicine, lights-off for sleep, discipline,
  attention beep.
- **Ages in real time while powered off** — the RTC + NVS payload: on boot,
  replay elapsed decay/sleep/evolution.
Art: original creatures (Bandai sprites are copyrighted) on a **32×16 1-bit
grid** rendered with fat pixels inside a drawn egg-shell bezel; ~20 tiny
frames. Icon row arcs along the round bezel. Board: Stopwatch — the original
is a 3-button device (A select / B confirm / C cancel), pocketable, and the
attention beep needs the buzzer/vibration. V1 scope: egg→child→one-branch
adult, hunger/happiness/poop/sleep/death, offline aging. Full evolution tree
is stretch/STORY material.

### 17. Pokedex
PokeAPI for data + sprites via a **build-time fetch script** — assets are not
committed; licenses are a knowingly deferred concern. Technical meat:
- Asset pipeline at scale: batch palette-reduce → RGB565 → C arrays + index
  (reusable script; days 3–5 did one image, this does hundreds).
- Storage math taught explicitly (96×96 RGB565 ×151 ≈ 2.7 MB of 16 MB flash).
- LVGL virtual list (row recycling), detail page with stats bars + type
  badges, alpha/number jump. Stretch: play cries through the ES8311.
Offline by design — the pipeline is the lesson. A live-fetching dex can be an
internet-arc revisit.

### 18. Yo — ESP-NOW pager
The "how boards connect" lesson (ESP-NOW is built into ESP32 — peer-to-peer
Wi-Fi, no router). Framed after the app "Yo". Payload kept packet-shaped so
the protocol is the star:
- Periodic hello broadcast → live roster of nearby boards (broadcast +
  MAC-as-identity).
- Pick a peer, press a button → their board vibrates/beeps + shows your emoji
  (unicast + peer registration).
- Show send-callback status on screen: sent ≠ received; add a retry.
All three boards, heterogeneously: Stopwatch answers with haptics, touch
boards with taps.

### 19. Captive Portal Provisioning
The gateway lesson — everything internet-facing depends on it. No creds →
SoftAP → phone joins → DNS hijack pops the portal → pick SSID from scan,
enter password + API keys → NVS → reboot as station. Hold-button-at-boot
resets provisioning. Build a minimal portal from scratch; cite WiFiManager as
the prior art. Also the provisioning path for later API keys (day 24).

### 20. Weather Station
**Open-Meteo — genuinely no key.** The portal's immediate payoff: day 19 ends
with a connected board and nothing to show; day 20 makes it do something in
~50 lines. Round watch-face layout: current conditions, hourly arc, forecast.
Composes later with an ENV IV sensor (inside vs. outside).

### 21–22. Walkie-Talkie (local, then internet)
Deliberately adjacent — same UI, swapped transport.
- **21 (ESP-NOW):** PTT on a Stopwatch pusher, channel up/down on the others;
  mic → ESP-NOW → speaker to the 1.8. **22 logical channels** as a channel
  byte in the packet (RF has only 11–13 real channels — don't map 1:1).
  Half-duplex by design (PTT) — no echo cancellation. Audio kept cheap:
  8–16 kHz mono, ADPCM or raw PCM; skip Opus.
- **22 (internet):** transport swap to a **Cloudflare Durable Object relay**
  (~50-line Worker; repo already deploys with wrangler). Channels = rooms.
  Alternatives considered: public MQTT (higher latency), Mumble/murmur
  (heaviest; Opus on S3 is tight).

### 23. Flight Radar
**adsb.lol / adsb.fi / airplanes.live** — free, no-key community ADS-B APIs
("aircraft within X nm of lat/lon"); OpenSky works anonymously with tight
limits. Radar-sweep UI on the round face: you at center, blips with
callsign/altitude, tap for details. Builds on 22's fetch/poll, adds geo math +
custom rendering. STORY: the ADS-B volunteer ecosystem (1090 MHz, $20 SDRs).

### 24. Package Tracker
First **keyed** API — free tier of an aggregator (17track / AfterShip /
TrackingMore; carriers all require registration, scraping is bot-protected
and brittle). Key entered through day 19's portal — framed as "your first
keyed API", modeling how real integrations work. REST + polling + the
virtual-list UI from 17.

### 25. Music Player
The de-scoped iPod (no BT Classic on the S3 ⇒ no A2DP): onboard ES8311 +
speaker, library UI, album art through the asset pipeline. 3.5 mm line-out
via PCM5102A is a stretch upgrade (see hardware).

### 26. Real-Time Captions
Streaming mic → STT service (Deepgram / Whisper endpoint). **Resurrect the
prior captions project (somewhere local or on GitHub) before building.** The
enabling lesson for 27–28.

### 27. Note Taker (voice memos)
STT-first, not keyboard-first: **push-to-talk voice memos on the Stopwatch**
— PTT pusher, streams through 26's pipeline, saves the transcribed utterance
to **Memos** (self-hosted, simple REST) — first *write* to a service.
Supabase is the hosted alternative.

### 28. Shazam
Same capture path; fingerprinting is off-device via **ACRCloud or AudD**.
Mostly an audio-capture + API lesson on top of 26.

### 29. Trivia Capstone
Deliberately late — it composes prior days instead of introducing five things
at once: ESP-NOW (18), sound stingers (8), buttons/haptics (10), portal (19),
REST (24). Host broadcasts the question (1.8 = host/scoreboard), buzz-in on
the Stopwatch; **latency fairness: timestamp at press, not at receive**.
Questions live from Open Trivia DB (free, no key). Prior art to cite:
**M5Stack TriviaPOD** (TP-01-A, $39.90) — licensed Trivia Crack product on
the same 1.75″ 466×466 AMOLED family, almost certainly closed-source; steal
concepts (offline packs + Wi-Fi refresh, two-device link), write our own game.
Hook: "M5 sells this as a product; we build it on the same silicon."

### 30. WorkOS AuthKit — Device Grant
OAuth 2.0 Device Authorization Grant: board shows user code + QR → approve on
phone → device polls for tokens. Least-documented ESP32 territory in the
series; retroactively upgrades earlier days (authenticated Memos, per-user
anything).

## Stretch / sequel list

- **Agent pet** — a desk creature embodying your coding agents (Codex,
  Claude Code, pi/Herdr): asleep when idle, working animation while an
  agent runs, attention beep + vibration when one blocks on input,
  celebration on completion; crown = acknowledge. Composes day 16 (pet
  state machine + art), day 10 (haptics), day 19 (portal, for the
  wireless v2). Host half: pi extension lifecycle events / Claude Code
  hooks / a Herdr publisher; transport v1 = USB serial lines
  (`AGENT running codex`), v2 = WebSocket via the day-22 relay.
- **Music Player** (demoted from the main line when metronome/tempo
  were promoted) — the de-scoped iPod: onboard ES8311, library UI,
  album art via the asset pipeline.
- **Shazam** (demoted likewise) — capture path + fingerprint API
  (ACRCloud/AudD) on top of the captions pipeline.

- **Conference badge** — Mastodon/Bluesky public endpoints (no API keys),
  wearable on the cased 1.75-B. Best if there's an event to wear it at.
- **Compass** — blocked: no magnetometer on any board. Unblocks with the
  M5 **IMU Pro Mini Unit** (BMM150) clicked into the Stopwatch's Grove port.
- **3.5 mm line-out** — PCM5102A/UDA1334A I²S DAC breakout on the 1.8's GPIO
  header (second I²S peripheral; ES8311 untouched). **Never tap the speaker
  header**: BTL Class-D output, neither wire is ground.
- **GSR "lie detector"** — sensor on hand; analog ⇒ needs an ADC pin (1.8's
  header, or a remapped Grove pin). Skin-conductance graph + truth-meter
  theatrics.
- **Sensor days** — ENV IV (SHT40+BMP280, ~$5), SCD41 true CO₂ (~$25–40),
  PM2.5 (PMSA003, UART), mmWave presence, MAX30102 heart rate, GPS (pairs
  with C25K pace), MLX90640 thermal camera, IR remote.
- **Internet speed tester** — cut: 2.4 GHz-only caps real throughput at
  ~20–40 Mbps; misleading as a connection tester.
- **iPod → Bluetooth speaker** — cut: no BT Classic/A2DP on ESP32-S3.

## Constraints & known deficiencies

- **No magnetometer, no BT Classic** on any board (see stretch list).
- **Grove port ≠ I²S**: HY2.0-4P exposes two signal pins; I²S needs three.
  3.5 mm audio belongs on the 1.8's raw headers, not the Stopwatch.
  (Verify the Stopwatch port is Port A / I²C and its GPIO mapping before
  buying Units.)
- **Secrets in a public repo**: day 19 is the pattern; nothing credentialed
  ships before it.
- **dB accuracy**: uncalibrated MEMS mic — relative readings unless calibrated.
- **Hotspot detection** (if the Wi-Fi scanner returns from the cut list):
  heuristic only — locally-administered MAC bit, SSID patterns,
  Interworking/Passpoint IEs; never ground truth.
- **Verification for network days**: outcomes depend on external services —
  record dated transcripts/soak logs, per the repo's verification discipline.
- **Multi-device days (18, 21–22, 29)** need 2–3 working boards on hand.
- **PokeAPI/Nintendo, Tamagotchi/Bandai**: knowingly deferred; assets fetched
  at build time where possible, original art where practical.
