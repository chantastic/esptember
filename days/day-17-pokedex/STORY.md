---
title: Pokedex
---

The interesting decision happened before any code: where do 151 copyrighted sprites live in a public repo?

Nowhere.
The repo ships a pipeline — fetch, composite, convert, generate — and the assets exist only on the machine that runs it.
PokeAPI serves the sprites keylessly; the script caches them, flattens transparency onto AMOLED black, packs RGB565, and emits generated C files that git never sees.
The lesson is the pipeline; the creatures are just its demo cargo.

## Budgets are design inputs

151 × 96 × 96 × 2 bytes is 2.7 MB, and the default ESP-IDF app partition is 1 MB.
That mismatch is the day's second lesson: partition tables are one CSV, and choosing where 16 MB of flash goes is firmware design, not configuration trivia.
Six megabytes to the factory app and the whole national dex of 1996 fits with room for three more generations.

## The crash that ate the USB port

First boot: black screen — and esptool couldn't even *reach* the chip.
That second symptom was the scary one, and it was pure noise: a panic on every boot, rebooting so fast the USB-JTAG peripheral never finished enumerating.
The lesson about instruments came first: a crash loop can take your debugger down with it, and the fix is boring — get the boot log the moment the port breathes.

The panic itself decoded to LVGL's theme engine dying inside `lv_list_add_button`, deep in row N of 151.
Null pointer.
LVGL was allocating from its builtin 64 KB pool — one missing `CONFIG_LV_USE_CLIB_MALLOC` line, lost when this day's config was derived from day 08's by careless truncation — and a 151-row list drinks 64 KB before it's a third built.

Day 07 had already learned this exact setting for snapshots.
The knowledge existed; the *transfer* failed.
That's the strongest argument yet for the shared board component in NOTES.md: pasted bring-up code forgets things that a dependency remembers.

## Re-dress, don't rebuild

The detail screen is constructed once.
Browsing retargets one `lv_image_dsc_t` at a different flash address and rewrites five labels — no allocation, no teardown, no churn.
The sprite never copies anywhere: LVGL draws it straight from memory-mapped flash, which is what all that partition budgeting bought.

## What we learned

- Ship the pipeline, not the assets. A fetch script is smaller than one sprite and carries none of its baggage.
- Partition tables are design surface. 2.7 MB of art fits the moment you *ask* the flash for it.
- A crash loop can kill your debugger. When the console and the bootloader both vanish, suspect reboot speed before dead hardware.
- Config is code and it rots the same way. The 64 KB ambush was a known setting that didn't survive copy-paste — dependencies remember what pastes forget.
