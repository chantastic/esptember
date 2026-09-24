---
title: Metronome
---

A metronome is a stopwatch with opinions about when to speak.

The whole build is day 12's one rule — clocks are arithmetic — under a musical deadline.
A stopwatch that renders late shows a stale number and nobody dies.
A metronome that *clicks* late is worse than no metronome, because a musician trusts it with their hands.

## delay() is a tempo rubato

The naive loop — click, `delay(interval)`, repeat — doesn't keep tempo; it keeps *tempo plus overhead*.
Every iteration silently adds the cost of rendering, button polling, serial chatter.
At 120 BPM with two milliseconds of loop cost, you lose a full beat every four minutes, and you lose it gradually, which is the worst way.

The absolute schedule can't do that.
`nextBeatMs += interval` fixes each beat to a grid laid down when the transport started.
A slow frame makes one click late; it cannot make the *grid* late.
Errors stay errors instead of becoming policy.

## The median forgives one bad tap

Tap-tempo is the day's second timing instrument, pointed the other way — measuring the human instead of pacing them.
Humans tap sloppily.
Four taps give three gaps; taking the median means one rushed or dragged tap changes nothing at all, where an average would smear it into the tempo.

The detail that makes it feel *right*: setting a tempo re-anchors the beat grid to your final tap.
You tap-tap-tap-tap and the metronome comes in on your next beat, in your time — the machine joins you, not the reverse.

## Accent is hierarchy you can feel

The downbeat gets a higher pitch, a longer buzz, a harder vibration — day 11's three-axis feedback code applied to musical structure.
With the speaker covered, bars are still countable by feel alone, which is the difference between a metronome and a drum machine for practice in loud rooms.

The pendulum dot exists for the eyes-only case: it sweeps once per beat and alternates direction each beat, because the mechanical original did, and a century of musicians already knows how to read that motion.

## What we learned

- `delay()` keeps tempo-plus-overhead. Absolute schedules keep tempo; late beats stay incidents instead of becoming the new grid.
- Medians forgive humans. Three gaps, middle value — one bad tap costs nothing.
- Join the player's time. Re-anchoring the grid to the last tap is one line and the whole difference in feel.
- Accent is structure, not decoration. Pitch, duration, intensity — the downbeat should survive a pocket.
