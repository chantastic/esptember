---
title: Level
---

This lesson was planned for a different board — the second round Waveshare display.
The board couldn't be found on build day, and the StopWatch turned out to be the better host anyway: it has the round face the design wants, plus the two things the original target lacks — a vibration motor for the snap moment, and a BMI270 already wired.

The best hardware for a lesson is sometimes the one on the desk.

## The support force is not gravity

The first bug a tilt sensor invites is a sign error.
An accelerometer at rest doesn't measure gravity — it measures the table pushing back.
Face-up, that's +1g on Z; the vector points *up*.
Get this backwards and every tilt runs mirrored, and you'll "fix" it by negating an axis somewhere downstream, and the mirror will come back with the next refactor.

The project had already written this warning down once — the conference-badge notes on the same IMU — and reusing a documented convention beat rediscovering it.
Two `atan2`s later, tilt is two floats in degrees.

## Bubbles lie in the right direction

A bubble level's bubble moves *away* from the low side — the air floats.
The natural implementation (bubble follows tilt vector) feels wrong instantly, in the hand, before you can articulate why.
Negate both axes and it feels like a tool.
Instrument metaphors carry physics with them; honor the physics or the metaphor turns hostile.

## The green moment

The snap-to-level is the whole product.
Two details make it feel engineered instead of coded:

Hysteresis — enter level inside 1.0°, leave outside 1.5°.
Without the gap, the boundary flickers green-orange-green at exactly the moment the user is being most careful, which reads as the tool mocking them.

The haptic tick — one 60 ms tap when the state latches.
You stop watching the screen and start feeling for the answer, which is how a real level works: you know it's true when it *stops arguing*.

This is also the board's first continuously animated day, and the sprite earned its 434 KB of PSRAM: compose off-screen, push whole frames, and the bubble glides at 30 fps where direct drawing would tear.

## What we learned

- Read the sensor's convention before the sensor. Support force, not gravity — the sign error you prevent is the refactor you don't chase.
- Metaphors carry physics. A bubble that follows tilt is code; a bubble that flees it is a level.
- State edges need hysteresis. Any threshold a user deliberately approaches will chatter without a dead band.
- Full-screen sprites buy smoothness with memory. 434 KB of PSRAM is the price of a bubble that glides.
