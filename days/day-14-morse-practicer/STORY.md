---
title: Morse Code Practicer
---

Three days of button timing — grammar, then a stopwatch, then C25K before either — and the natural question is what all that discipline is *for*.

Morse is the answer with a birth certificate.
It's a timing protocol from 1844: one signal, one dimension, everything encoded in durations and silences.
A straight key is the minimum viable keyboard.

## The whole standard is five ratios

ITU-R M.1677-1 defines international Morse in a page: dah is three dits, gaps of one, three, and seven units separate symbols, letters, and words.
Speed is one number — words per minute, measured against the word "PARIS," which happens to be exactly 50 units.
So `unit = 1200 / WPM` milliseconds, and the entire protocol scales from one division.

The decoder needs two decisions.
Press length: shorter than 2 units is a dit — the midpoint between 1 and 3 gives equal tolerance both ways.
Silence length: 3 units closes a letter, 7 closes a word.
That's it.
The rest is a 36-entry lookup table that hasn't changed since radio operators wore wool.

## Silence is data

The interesting engineering is that most of the decoding happens when nothing is happening.
The loop watches the gap since the last release grow — past 3 units it commits the letter, past 7 it commits the space.
There's no "done" button.
The absence of input *is* the input, which makes this the purest possible exercise of day 12's rule: timestamps, not counters, and never block the loop, because the loop is literally reading silence.

The sidetone follows the same honesty.
It starts on press and stops on release — what you hear is exactly what the decoder measures.
If your dah sounds short, it was short.
The tone is the debugger.

## What we learned

- A protocol can be five ratios and a table. Morse survives because its spec fits in a pocket.
- Silence is data. Letter and word boundaries are decoded from what *didn't* happen, on a clock.
- The sidetone is the debugger. Feedback that mirrors measurement teaches faster than any error message.
- Every timing lesson was this lesson. Grammar, clocks, non-blocking loops — pointed at 1844, they decode telegraphy.
