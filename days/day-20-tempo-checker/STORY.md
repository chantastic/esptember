---
title: Tempo Checker
---

Day 19 built the clock that speaks.
Day 20 builds the ear that listens back — and the two verify each other, which is the real reason they shipped as a pair.

## Beats are edges, not sounds

The first instinct is spectral: FFT the music, find the beat in the frequencies.
But a beat isn't a frequency — it's an *event*, a sudden rise in energy against what came before.
The detector is therefore two numbers: a fast one (this frame's RMS) and a slow one (a 2% running average), and an onset is the fast crossing 1.8× the slow.

The refractory window does the quiet heavy lifting.
A snare hit is not one energy spike; it's a burst that would read as three onsets 40 ms apart and wreck every gap measurement downstream.
Declaring 220 ms of deafness after each onset costs nothing musically — 240 BPM is still detectable — and buys clean intervals.

## The octave problem is unsolvable, so say so

Fold every gap into 250–1500 ms and the estimator becomes robust to missed beats and double-counts — and permanently blind to the difference between 60 and 120 BPM.
Every beat tracker in existence fights this; the honest ones admit the ambiguity instead of guessing.
This one folds, reports, and lets the confidence bar say *how sure* while the human supplies the octave.

Confidence itself is the day's best interface idea, borrowed from day 15's honesty streak: the fraction of gaps agreeing with the median.
Music scores high.
Conversation, keyboard clatter, a passing truck — the onsets fire, but the gaps agree on nothing, and the bar stays low.
The instrument knows when it doesn't know.

## The pair validates itself

The verification plan needs no reference gear: set day 19 clicking at 120 on one board, point day 20 at it from the other.
If the checker reads the metronome's number, both instruments are right; if not, at least one is lying, and the disagreement localizes the bug.
Instruments in pairs audit each other — the same trick as the walkie-talkie's two transports, applied to time.

## What we learned

- Beats are edges. A fast average crossing a slow one finds rhythm better than any spectrum.
- Refractory windows clean the data at the source. 220 ms of deafness beats any amount of downstream filtering.
- Fold, then admit it. The octave ambiguity is structural; honest instruments report it instead of guessing.
- Confidence is an output. The fraction-in-agreement number turns "a BPM" into "a BPM you may trust."
