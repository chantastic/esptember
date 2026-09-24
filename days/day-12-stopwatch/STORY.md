---
title: Basic-Ass Stopwatch
---
## The prompt-first pass

The original build below records how this idea first worked.
The durable lesson now targets the M5Stack Stopwatch and lives in the prompt, behavior contract, generalized tests, and hand-review checklist.
Generated firmware is evidence for those sources, not the source itself.


This day exists because day 13 shipped first.

C25K is a stopwatch wearing a training plan, and building it out of order meant its foundations — button grammar, render cadence, feedback cues — never got their own lesson.
Day 11 extracted the buttons.
Day 12 extracts the clock.

## Ergonomics are a spec

Pick up any mechanical stopwatch: the crown starts and stops, the side pusher splits and resets, and the reset physically cannot fire while the timer runs.
Casio kept the same contract in plastic.
That's not tradition for its own sake — it's a hundred years of users encoding one insight: *the destructive action must never share a gesture with a frequent one.*

So the crown never resets.
Lap (frequent, running) and reset (destructive, stopped) share a pusher but can never collide, because run state separates them — and reset demands a hold on top of it.
The interlock costs four lines of `if`.

## The display never owns the time

The rookie stopwatch keeps a counter and increments it every frame.
Then the frame rate hiccups, and the clock drifts.

Here the clock is arithmetic, not accumulation: a banked total plus the delta since the last start.

```c
  return running ? accumulated + (millis() - startedAt) : accumulated;
```

Rendering at 10 Hz, or 1 Hz, or not at all changes nothing about elapsed time.
Laps are reads of that same arithmetic — the split freezes on screen while the timer runs through it, which is what "lap" has meant since the pocket-watch era.

## What we learned

- Ergonomics are inherited, not invented. The crown/pusher contract predates the microcontroller by a century; honor it.
- Destructive actions get friction. Reset hides behind stopped-state *and* a hold — two locks on the only unrecoverable button.
- Clocks are arithmetic, not counters. Compute elapsed from timestamps and no render hiccup can ever steal time.
- Ship the foundation late, mine it anyway. Day 13's machinery taught this lesson before it was written.
