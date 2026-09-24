# ESPtember hardware and controls

- For Day 07 and later M5Stack Stopwatch work, read and apply
  [.agents/skills/build-stopwatch-lessons/SKILL.md](.agents/skills/build-stopwatch-lessons/SKILL.md)
  before changing a lesson prompt, contract, tests, or disposable device build.
- For prompt-first lesson conversions, also read and apply
  [.agents/skills/prompt-first-embedded-lessons/SKILL.md](.agents/skills/prompt-first-embedded-lessons/SKILL.md).
- Day 07 is Touch Calibration; LVGL and the existing lineup follow it. From
  Day 07 onward, target the M5Stack Stopwatch and work one lesson at a time.
- The durable lesson source is a detailed human-readable build prompt, behavior
  specification, generalized contract tests, and explicit acceptance criteria.
  Do not implement firmware, build, flash, or modify generated application code
  unless the user explicitly asks for that step. Existing firmware may remain
  as historical evidence.
- Put deterministic behavior behind a portable contract and test it without the
  board. Keep compile, instrumented-device, and physical checks as separate
  acceptance layers; never claim physical behavior from host tests.
- Treat touch calibration as device-owned runtime state, never as coefficients
  compiled into an application. From Day 08 onward, load and apply the shared
  `espt-touch/record` after display setup and offer an explicit path to collect,
  observe, apply, and save a replacement map without reflashing. Version 2 is
  an affine seed plus a local residual field, so later apps must map raw touch
  through the shared module instead of relying only on M5GFX converted points. App-only and
  OTA updates must preserve that namespace. Label merged images written at
  `0x0` as fresh installs because they can erase NVS calibration.
- With the lanyard loop at the bottom, **M5.BtnA is left** and **M5.BtnB is
  right**. Use those physical names in instructions. Do not infer sides from
  A/B order, button colors, or earlier examples calling a button the crown.
- Default to the four gestures in [C25K's spec](days/day-13-c25k/SPEC.md#3-input-grammar):
  left/A = previous or decrease; right/B = next or increase; both briefly
  pressed and released = select/enter; both held for 600 ms = back/escape.
  A chord joins within 80 ms and overlaps for at least 80 ms. Consume its
  individual clicks, and never fire Enter after a hold. No single-button
  holds in the default grammar.
- Use an intentional project-specific control scheme when the activity has
  a better established convention, such as a stopwatch. Document the
  exception and verify its physical left/right mapping on the device.
- Audit legacy control descriptions and source prompts as each day is reworked;
  existing examples may have conceptually reversed the buttons.
