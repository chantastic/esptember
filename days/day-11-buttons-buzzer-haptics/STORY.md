---
title: Buttons, Buzzer, Haptics
---

Nine days of screens.
Today the screen is optional.

The StopWatch kit is shaped like a stopwatch, and stopwatches are operated by feel — thumb on the crown, no eye contact.
That's the design constraint this lesson borrows: every gesture must be recognizable without looking.

## Five gestures, two buttons

Two buttons give you two clicks.
Time gives you two holds.
Simultaneity gives you a chord.
Five distinct inputs from two pieces of metal, and every one of them is a timing problem.

M5Unified handles the debouncing and the hold thresholds.
What it can't decide is ownership: when both buttons go down, who gets the event?
Day 13's C25K solved this with a grammar — a gesture owns its buttons until both are released — and this lesson extracts that rule into twenty lines.
The chord fires the moment both buttons are down, then swallows everything until full release.
No stray clicks on the way out.
The same discipline covers holds: a release after a hold is not a click.

Those two leaks — chord-then-click and hold-then-click — are the entire difference between a button grammar and a bug tracker.

## The feedback map

The cue table is the part worth arguing about.

Clicks: short, high, distinct pitches for A and B.
Holds: longer, lower — an octave down from their clicks.
Chord: highest pitch, longest buzz, maximum vibration, unmistakably *the big one*.

Pitch encodes which button; duration encodes click versus hold; intensity encodes the chord.
Three dimensions, five gestures, no collisions.
That's not sound design so much as information design that happens to be audible.

The vibration motor turns off on a scheduled deadline, not a `delay()` — the loop never blocks, because a blocked loop drops the next button edge, and a dropped edge in a timing grammar corrupts the gesture after it.

## What we learned

- Two buttons are five inputs. Clicks, holds, and a chord — timing multiplies hardware.
- Gestures need ownership rules. A chord that leaks clicks isn't a chord; it's three bugs.
- Feedback is a code. Pitch for identity, duration for type, intensity for priority — design the mapping, not just the sounds.
- Never `delay()` in a timing loop. Every blocking call is a dropped edge waiting for a victim.
