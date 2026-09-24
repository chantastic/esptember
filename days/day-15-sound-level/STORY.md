---
title: Sound Level (dB)
---
## The prompt-first pass

The original build below records how this idea first worked.
The durable lesson now targets the M5Stack Stopwatch and lives in the prompt, behavior contract, generalized tests, and hand-review checklist.
Generated firmware is evidence for those sources, not the source itself.


The plan called this lesson "sound level (dB)" and the first honest act was deciding which dB.

Sound pressure level — the number on a real SPL meter, the one where 60 dB is conversation and 90 dB is a lawnmower — requires knowing how many pascals of air pressure produce a given sample value in *this* mic, through *this* gain stage.
That's a calibration constant nobody publishes for a $2 MEMS part on a dev board.

dBFS requires knowing only the number 32768.

So the meter reads dBFS, says "relative, uncalibrated" on its face, and every claim it makes is true.
The gap between the two units is one constant, measurable with a reference meter and a steady tone, and the README says exactly that.

## RMS or nothing

Three candidate statistics for "how loud is this window":

Peak — jumps to full scale on a single click, tells you about transients, not loudness.
Mean — a sine wave's mean is zero; congratulations, silence.
RMS — square, average, root: the energy measure, the one the ear roughly follows, the one every meter uses.

The window is 50 ms — 800 samples — because that's fast enough to catch speech dynamics and slow enough that the number is readable.
Twenty windows a second, and the UI timer runs at the same 50 ms so every reading gets drawn exactly once.

## The peak that waits

The peak-hold marker is the one behavior borrowed straight from hardware meters: hold the maximum three seconds, then follow the signal down.
Without it, a clap is gone before the eye arrives.
With it, the meter has memory — the transient waits for you.

The first boot printed a quiet room at -54 dBFS with the peak riding at -47, which is exactly what a quiet room with a computer fan in it should look like.
The number moved when spoken to.
Instruments that respond to the world on the first try are rarer than they should be.

## What we learned

- Name your reference. dB means nothing until you say dB *relative to what* — and dBFS is the only honest unit an uncalibrated mic can claim.
- RMS is the loudness statistic. Peak lies about clicks, mean cancels to zero; square-average-root or go home.
- Meters need memory. Peak-hold exists because eyes are slower than transients.
- Calibration is one constant. The distance from honest-relative to useful-absolute is a reference meter and an afternoon.
