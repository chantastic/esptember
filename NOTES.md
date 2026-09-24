# ESPtember — notes & lesson ideas

- **Hardware direction · September 23, 2026** — Day 07 is Touch Calibration;
  LVGL and the existing lineup follow it. Target the M5Stack Stopwatch and
  work one lesson at a time.
- **Prompt-first direction · September 23** — stop treating a generated build
  as the lesson source. Keep a detailed human-readable prompt, behavior spec,
  generalized portable tests, and layered acceptance criteria. Generate
  implementations only when explicitly requested; disagreements between
  implementations first trigger a prompt/spec review.
- **Day 07 contract experiment · September 23** — three disposable affine-core
  implementations (normal equations, QR, centered covariance) each passed the
  same 436 assertions. The comparison added an exact API, 40-byte record,
  CRC byte order, median rule, inclusive thresholds, and invalid-input behavior
  to the prompt/spec. Centered covariance produced the smallest host object;
  no candidate was promoted to lesson source.
- **Day 08 calibration handoff** — the retained LVGL firmware predates Day 07's
  `espt-touch` persistence contract. Its next source prompt must require loading
  and applying a valid calibration after panel geometry setup and before LVGL
  creates its pointer input. Do not patch offsets into the LVGL callback.
- **Default controls · September 23** — C25K is the reference. Lanyard down:
  BtnA is left/previous/decrease; BtnB is right/next/increase. Press both and
  release to select; hold both 600 ms to go back. Preserve C25K's chord
  timing and click swallowing. Use localized conventions where they make
  sense (e.g. stopwatch), with explicit physical mapping.
- **Control audit, for each day's rework** — Day 13 already maps A to Prev
  and B to Next. Days 17 (Tamagotchi) and 21 (Yo) currently advance selection
  with A and act with B; adapt to the default grammar when reached. Days 11,
  12, 14, 24, 25, and 29 describe A as the "crown"; review those descriptions
  and physical assignments rather than assuming that means right. Day 12's
  stopwatch-specific actions remain an intentional exception. Its current
  code puts start/stop on left/A and lap/reset on right/B; review those sides
  against the intended stopwatch convention when reworking that day.

- **Days 09–33 prompt-contract migration · September 23** — all 25 lessons now
  use a detailed build prompt, lesson-specific `SPEC.md`, machine-readable
  contract, three disposable reducer shapes, generated action sequences,
  deliberate mutation rejection, reference frame, and a hand-review sheet.
  Historical implementations and build products are local evidence under
  ignored `.build/` directories; they are no longer the published source.
- **Evidence taxonomy is a project invariant** — call deterministic artwork a
  reference frame, portable state checks host-contract evidence, target-library
  builds compile evidence, scripted input on the real app injected-device
  evidence, and direct human operation physical evidence. Do not let one label
  imply another. Every Day 09–33 post now uses that vocabulary.
- **Generic contracts need domain companions** — the shared runner catches
  incomplete state machines, mutation, invalid bounds, layout overflow, and
  control-map drift. It cannot establish signal processing, timing math,
  packet formats, storage encodings, or calibration quality. Day 13's retained
  workout, progress, summary, and 98-assertion button suites are the model for
  adding focused tests beside the common contract.
- **Hand review is specification work** — each lesson now has a deterministic
  review path and a place to classify findings as lesson-specific,
  Stopwatch-wide, or process-wide. Change the narrowest durable owner, add an
  observable check, regenerate a disposable candidate, and record only the
  evidence layer that was rerun.
- **Repeated product decisions extracted during migration** — use identical
  Stopwatches for peer roles in Yo, local/internet walkie-talkie, and trivia;
  keep the calibrated touch record device-owned and updateable at runtime;
  use C25K's four gestures by default; let established activities such as a
  stopwatch, Morse key, or metronome document a localized control grammar.
- **Interactive audio latency · Day 09** — prepare final PCM before enabling
  input, retain it in PSRAM, trigger on touch-down, and let the newest pad
  restart the single voice immediately. Keep synthesis, decoding, file I/O,
  allocation, long logging, animation waits, release detection, and pending
  queues out of the trigger path. Measure request-to-speaker-start latency on
  the device instead of describing the interface as fast.

- **Pending hands-on checks (autonomous run, Sep 21)** — day 09 speaker
  ear check; day 11 cue discrimination in-pocket; day 12 pusher feel;
  day 14 keying feel per WPM; day 17 multi-day aging + evolution +
  death (an egg is incubating on the StopWatch now — it hatches after
  1h and should show offline aging after any power-off gap). Day 15
  needs a shout test beyond room tone. Nothing deployed to the site
  yet — all committed locally, deploy after the touch-drill below.
- **Day 17 art pass** — the five 16x16 creatures are programmer hex art;
  worth a real pixel pass. Also: sleep schedule and evolution branching
  by care mistakes are speced in PLAN.md but not in v1.

- **Day 08 Stopwatch port · September 23** — Arduino esp32 3.3.10,
  M5Unified 0.2.19, M5GFX 0.2.26, LVGL 9.3.0. Flashed and passed the
  device harness: count, brightness range, switch, state retention,
  framebuffer captures, and 20 more screen round trips with unchanged
  heap/PSRAM. C25K button controls now cover focus, edit, select, and back;
  the copied recognizer passes its 98 portable assertions. Button feel,
  physical touch alignment, and perceived brightness await the user's
  hands-on review. The driver reports 468 × 468 despite the
  advertised 466 × 466 panel; derive render dimensions from M5GFX.
- **Day 08 previous Waveshare investigation** — the physical touch
  calibration question remains historical, not a prerequisite for the
  Stopwatch version. Earlier source and findings remain in Git history;
  the local previous build is retained in the day's ignored
  `.build/legacy-waveshare/` directory.
- **Day 08 harness pattern is reusable** — virtual LVGL pointer plus
  `status`/`tap`/`drag`/`capture` USB commands. The Stopwatch version keeps
  the pixels sent to M5GFX in PSRAM and streams a snapshot in bounded
  chunks. It tests LVGL separately from the physical touch controller.

- **Day 13: C25K on the M5Stack StopWatch** — standalone Arduino firmware;
  all 27 workouts passed accelerated device checks, and a 125-second real-time
  check crossed warm-up/run/walk boundaries. Button feel, perceived sound and
  vibration cues, daylight legibility, and full-workout battery runtime still
  need an outdoor check. The recorded 68.2 ms maximum loop gap excludes frame
  captures; measure again before tightening the 80 ms chord window.
- **Progress versus elapsed time** — replaying an interval moves the ring back
  while retaining the effort in the log. Portable timing/button helpers make
  those rules testable without real-time waits or fake completed user sessions.
- **Day 13 clock follow-up** — hour/minute editing preserves the RTC date and
  uses the visible build date when unset. Add calendar/timezone editing if this
  firmware needs to travel beyond its current Pacific-time build.

- **Browser flashing with ESP Web Tools** — devote a later day to adding
  one-click flashing from the site via Web Serial + a `manifest.json` per
  day pointing at the hosted merged binaries. The site already hosts
  `public/firmware/<day>.bin`, so this is mostly a manifest + a
  `<esp-web-install-button>` per day page.
- **"Why did my screen die?" lesson** — the day-01 debugging saga is
  strong episode material: AXP2101 input-current starvation (USB drops,
  AMOLED GRAM freezes the last frame so it looks like a hang), and the
  V2 TCA9554 floating panel reset (waveshareteam issue #12). Method:
  serial heartbeat + on-screen counter to distinguish dead chip / dead
  panel / flush starvation.
- **V2 hardware note** — user's board (and current retail units) are V2:
  CO5300 panel + CST816-family touch, not the wiki's SH8601/FT3168.
  Every future day inherits pmu_init() + panel_reset_release() —
  consider extracting a tiny shared component once day 02 needs it.
- **Retest backlight_on** — we proved `bsp_display_backlight_on()` was
  required *before* the panel-reset fix existed. The floating reset may
  have been why init brightness didn't stick. Retest removal now that
  `panel_reset_release()` runs; if it's removable, day 01 drops to seven
  calls and the comment needs rewording.
- **Styling lesson** — day 01 is deliberately unstyled (default LVGL
  theme). A later day covers LVGL styling: colors, fonts
  (CONFIG_LV_FONT_MONTSERRAT_*), alignment, wrapping. ESPtember orange is
  #ff5b04.
- **Day 02 became the styling lesson** (customized text, escalating:
  color → size → custom font → effects). Stopwatch moves later.
- **Stopwatch day (was day 02)** — print elapsed time on screen. This is where the
  FreeRTOS pacing conversation lives (`vTaskDelay` vs `esp_timer` vs LVGL
  timers), deliberately kept out of day 01's naive hello world.
- Planned early days: hello world text → stopwatch → single image →
  carousel of images → gif → movie. Prefer whatever toolchain produces the smallest
  binary for the simple days; heavier days may switch toolchains — always
  note the toolchain in the day's frontmatter.
- **Days 03–06 media sequence implemented** — static graphic, swipeable
  carousel, GIF, raw RGB565 movie with SD-file preference and flash fallback.
  Days 03 and 04 are published through September 4.
  Days 05 and 06 remain in `drafts/`; outstanding hardware checks are recorded there.
  Saved internet media and attribution live in `assets/media/`;
  `scripts/make-media.py` converts them reproducibly with FFmpeg.
- **Storage is a partition decision** — day 02's 16 MB flash setting still
  used the default 1 MiB app partition. Media lessons reserve 3 MiB for the
  app; day 06 additionally reserves 12 MiB for movie data and includes it in
  the merged firmware image.
- **LVGL heap vs PSRAM** — the installed LVGL is 9.5.0. Its built-in 64 KiB
  allocator cannot hold even the 659,456-byte ARGB8888 canvas of the 368×448 Homer
  GIF. Days 05–06 select the C library allocator; keep PSRAM enabled and
  check actual memory allocation rather than importing LVGL 8 formulas.
- **Movie follow-ups** — measure SD playback with a known card; add MJPEG
  parsing/decoding if longer flash clips are useful. Audio synchronization,
  large-file support, maximum card capacity, and higher frame rates remain
  separate experiments. Current raw player targets 368×224 at 10 fps,
  silent, with two PSRAM frame buffers; it does not decode MP4 or MJPEG.

- **Homer GIF fills turning black** — user observed the first frame looked
  right, then bushes, wall, and white shirt lost their fills. The old GIF
  had transparency in 28/29 frames (disposal 1); LVGL 9.5 clears those
  pixels' alpha instead of preserving prior colors. Encode complete opaque
  frames with `reserve_transparent=0` and `-gifflags 0`, and validate with
  `scripts/check-gif-frames.py`. User also requested full-screen playback:
  scale to cover 368×448 without stretching, then crop with the picture
  shifted left 26px (7% of screen width). User confirmed the opaque-frame
  fix preserves all fills and the full-screen version works.
