# ESPtember — notes & lesson ideas

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
  Writeups remain in `drafts/` until visual/hardware checks are complete.
  Reproducible geometric assets come from `scripts/make-media.py`.
- **Storage is a partition decision** — day 02's 16 MB flash setting still
  used the default 1 MiB app partition. Media lessons reserve 3 MiB for the
  app; day 06 additionally reserves 12 MiB for movie data and includes it in
  the merged firmware image.
- **LVGL heap vs PSRAM** — the installed LVGL is 9.5.0. Its built-in 64 KiB
  allocator cannot hold even the 102,400-byte ARGB8888 canvas of a 160×160
  GIF. Days 05–06 select the C library allocator; keep PSRAM enabled and
  check actual memory allocation rather than importing LVGL 8 formulas.
- **Movie follow-ups** — measure SD playback with a known card; add MJPEG
  parsing/decoding if longer flash clips are useful. Audio synchronization,
  large-file support, maximum card capacity, and higher frame rates remain
  separate experiments. Current raw player targets 184×224 at 10 fps,
  silent, with two PSRAM frame buffers; it does not decode MP4 or MJPEG.
