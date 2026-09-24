---
title: Sound Effects Board
---
## The prompt-first pass

The original build below records how this idea first worked.
The durable lesson now targets the M5Stack Stopwatch and lives in the prompt, behavior contract, generalized tests, and hand-review checklist.
Generated firmware is evidence for those sources, not the source itself.


The plan said "soundboard" and the first design question was where the sounds come from.

WAV files need an asset pipeline, flash budget, and a converter script.
A laser needs a sine wave and a falling frequency.
Six effects turned out to be about a dozen lines of math each — sweep, square, noise, envelope — and the asset pipeline for this day is `sinf()`.

## The shape of an effect

Every generator is the same skeleton: loop over samples, compute a normalized time `t`, shape a waveform with an envelope.
The differences are small and audible.
The coin is two square-wave notes because the reference hardware for that sound was square waves.
The kick drum is a sine wave whose pitch falls 90 Hz in 200 milliseconds — pitch drop *is* the kick.
The explosion runs noise through a one-pole lowpass because unfiltered noise is hiss, and explosions rumble.

```c
        low += 0.08f * (noise() - low);
```

One line of filter.
That's the whole difference between static and thunder.

## Audio never blocks the UI

The first architecture temptation was to play the sound in the button callback.
The callback runs on the LVGL task — synthesizing and writing half a second of PCM there freezes every animation on screen.

So one task owns the codec, and pads only queue an id.
The queue holds exactly one entry and sends don't wait: mash the pads and the current effect finishes, the latest request plays next, everything between is dropped.
That's not a compromise — it's the correct soundboard feel, chosen in one line of queue configuration.

## Pagination

Three pads per page is a touch-target decision: 92-pixel pads, full width, no near-misses.
The pager is deliberately dumb — two containers, one hidden flag, previous/next buttons that wrap.
Day 08's harness pattern carried over unchanged and drove the whole checkout: tap ids verified against the firmware's own play counter, page flips confirmed by captured screenshot.

## What we learned

- An asset is sometimes a function. Six sound effects cost zero bytes of flash assets and one afternoon of math.
- The audio callback is a trap. Synthesis belongs on its own task; UI events should only ever enqueue.
- A one-slot queue is a design statement: latest wins, nothing stacks.
- One-pole filters are twenty sounds wide. Noise, lowpass, envelope — most of a sound library lives in that triangle.
