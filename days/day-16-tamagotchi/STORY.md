---
title: Tamagotchi
---

The 1996 Tamagotchi is one of the best-documented toys ever made — the P1's timers, evolution rules, and care-mistake logic have all been reverse-engineered, and TamaLIB will run the actual ROM on a microcontroller.

That was the tempting road: real emulation, pixel-perfect nostalgia.
It's also Bandai's ROM and Bandai's sprites in a public repo, so the honest version of this lesson is a clean-room build: their *rules*, our code, our creature.

## Offline aging is the whole toy

Strip away the art and the Tamagotchi is one mechanic: it needs you even when you're not there.
Which means the simulation can't live in `loop()` time — it has to live in calendar time.

The design answer is a single `simulate(minutes)` function.
The live loop feeds it one minute per real minute.
Boot feeds it the entire absence: read the RTC, subtract the saved last-seen epoch, replay.

There's no separate "catch-up logic" to write or to get wrong — being powered off is just a large number of minutes.
The function that ages the pet overnight is character-for-character the function that ages it while you watch.

Fractional progress persists too: decay accumulates in minute counters that save with everything else, so twenty short sessions can't gift the pet immortality by resetting the clock on every boot.

## Two buttons, four verbs

The original had three buttons: select, confirm, cancel.
The StopWatch has two.
The menu absorbs the difference — A cycles a wrapping selection, B confirms, and cancel stops existing because no action needs one.
Feeding is idempotent, cleaning is idempotent, and play's failure mode is the creature declining, which is the original's charm anyway: the guessing game losing half the time *is* the personality.

## Sixteen rows of hex

The art question answered itself at 16×16.
One `uint16_t` per row, five bitmaps, fat pixels with a one-pixel gut between them — the LCD-grid look the original had, achieved by drawing rectangles.
An egg is symmetry.
A baby is the egg with eyes.
Death is a flat line under a face — sixteen numbers that read instantly from across a room.

No converter, no assets, no pipeline.
Sometimes sprite art is just typing.

## What we learned

- Offline aging is one function with a big argument. Replay minutes; never write "catch-up" code.
- Persist the fractions. Decay accumulators must save too, or short sessions reset every clock.
- Menus absorb missing buttons. Cycle-and-confirm turns two pushers into any number of verbs.
- Rules aren't copyrightable, expression is. The clean-room line: their timers, our everything else.
