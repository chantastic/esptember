# ESPtember — notes & lesson ideas

- **Browser flashing with ESP Web Tools** — devote a later day to adding
  one-click flashing from the site via Web Serial + a `manifest.json` per
  day pointing at the hosted merged binaries. The site already hosts
  `public/firmware/<day>.bin`, so this is mostly a manifest + a
  `<esp-web-install-button>` per day page.
- **Day 02: stopwatch** — print elapsed time on screen. This is where the
  FreeRTOS pacing conversation lives (`vTaskDelay` vs `esp_timer` vs LVGL
  timers), deliberately kept out of day 01's naive hello world.
- Planned early days: hello world text → stopwatch → single image →
  carousel of images → gif → movie. Prefer whatever toolchain produces the smallest
  binary for the simple days; heavier days may switch toolchains — always
  note the toolchain in the day's frontmatter.
