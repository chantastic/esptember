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
- **Day 02: stopwatch** — print elapsed time on screen. This is where the
  FreeRTOS pacing conversation lives (`vTaskDelay` vs `esp_timer` vs LVGL
  timers), deliberately kept out of day 01's naive hello world.
- Planned early days: hello world text → stopwatch → single image →
  carousel of images → gif → movie. Prefer whatever toolchain produces the smallest
  binary for the simple days; heavier days may switch toolchains — always
  note the toolchain in the day's frontmatter.
