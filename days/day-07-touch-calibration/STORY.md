---
title: Touch Calibration
---

The first Stopwatch LVGL pass exposed two hardware truths.

The display driver described two rows that the panel did not use, which left a black strip at the bottom of a bright screen.
Correcting the drawable area fixed the image.

Then the touch targets still did not feel trustworthy.

That belongs before LVGL.
A widget toolkit can route a touch, but it cannot decide where the finger physically landed without a correct hardware transform beneath it.

So Touch Calibration became Day 07 and LVGL moved to Day 08.

The lesson is a detailed implementation prompt rather than a permanent firmware project.
Its interaction borrows the familiar controller-stick rhythm: up, down, left, right, then around the world in both directions. The first screen uses one center dot and one destination dot because the sentence explains the drag; an orange line only made the focus region feel clipped.

Then Asteroid Shooter places known targets across the interior. A miss is the useful measurement: it shows how far the current map lands from the intended point.

The first shooter screen kept its title and progress at the bottom. That UI consumed the region the user already suspected was least accurate. The revised exercise pauses at a separate Tap to start screen, then removes every mark except the current target and adds an outer target ring.

That outer ring found the strongest miss at bottom center: almost 30 pixels before the local correction. The UI had been sitting on top of the most useful measurement.

The saved model starts with an affine transform and adds a smooth polar residual field. That field can describe an asymmetric expansion or contraction across the face instead of pretending the whole panel is shifted by one amount.

The useful boundary is simple: hardware calibration produces screen coordinates; LVGL consumes them.

## What we learned

- A prompt needs byte-level persistence details when later projects must share its output.
- Multiple implementations expose ambiguity faster than polishing one implementation.
- Portable tests can prove the transform contract without pretending they touched the glass.
- The physical acceptance list remains part of the source even when generated code is disposable.
- Memory checks must cover phase transitions. The first device candidate put the perimeter trace on the task stack and failed only when Around the World opened; moving it to persistent storage fixed the transition.
- A calibration trace is evidence, not an exam. Rejecting the user's circle for leaving the guide ring discarded the exact deviation data the lesson was meant to collect.
- Clockwise and counterclockwise traces can expose direction-dependent drag, so they stay separate until their sector residuals receive equal weight.
- A known target turns a miss into a correction vector. Different radii reveal local scale error that one affine transform cannot fit.
- Calibration UI can hide the area it needs to measure. Put instructions before the test, then give the targets the whole face.
