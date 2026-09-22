---
title: A GIF
---

Four days of stills, and the fifth asks for motion.
The GIF is the folk format for that — old, everywhere, and carrying its own timing — and LVGL ships a widget that plays one with zero application code.
The day should have been trivial.

It was instead a lesson in what a GIF actually is.

## Transparency that means "remember"

The first flash played one perfect frame, and then the bushes went black.
Then the wall.
Then Homer's shirt.
The edges of the animation kept moving correctly around holes where the picture used to be.

A GIF optimizer marks pixels that don't change between frames as *transparent*, with a disposal mode that says "keep what was underneath."
It's a beautiful trick from 1989 for saving bytes — 28 of our 29 frames used it.
And the installed LVGL decoder honors the transparency but not the memory: it clears those pixels' alpha instead of preserving the previous frame, and the "previous image" becomes the black screen behind the widget.

The fix belonged to the *pipeline*, not the player: re-encode with every frame complete and opaque, no transparent palette entry, FFmpeg's optimizations off.
And because a rule that lives in a script comment dies in a month, `check-gif-frames.py` now rejects any asset with transparent or partial frames before a build can ship it.
The bug became a validator, which is the only place bugs stay fixed.

## Two budgets, one file

The encoded GIF is 583 KB of flash.
The *decoded canvas* is a separate, bigger budget: 659,456 bytes at LVGL's default ARGB8888 — and these frames are opaque, on an RGB565 panel, so half of that is alpha nobody will ever read.
One `lv_gif_set_color_format` call halves the canvas, and observed loop callbacks tightened from ~4.9 s to ~3.6 s with it.

Both canvases dwarf LVGL's private 64 KiB heap, and the 8 MB of PSRAM sitting next to it doesn't help until the allocator is rerouted — the `CONFIG_LV_USE_CLIB_MALLOC` lesson that days 07 and 17 would later relearn the hard way.
Day 05 found it first.

## The logs kept counting

The most instructive part of the failure: while the colors were wrong, the serial logs were *happy* — loop callbacks arriving on schedule, heap stable, everything green.
A running animation still needs someone to look at it.
That sentence became this series' whole verification culture: the harnesses, the captures, the "serial submissions are not panel frames" honesty — day 05 is where the project learned that logs measure the code's opinion of itself.

## What we learned

- GIF transparency can mean "remember," and your decoder may not. Re-encode complete opaque frames; validate the rule in a script.
- The file and the canvas are separate budgets. 583 KB encoded, 330-660 KB decoded — pick the honest pixel format and halve the second one.
- LVGL's heap is not your PSRAM. Reroute the allocator or watch big widgets fail — the month's most-relearned lesson debuted here.
- Logs measure self-opinion. The loop counter ran cheerfully through every black frame; only eyes caught it.
