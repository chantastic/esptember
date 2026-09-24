# Three-build comparison

The same Day 07 contract was implemented three ways in ignored, disposable candidate directories.
No candidate is the lesson source; the prompt, specification, and tests are.

## Results

| Candidate | Affine fitting method | Contract result | Core source | Optimized object |
| --- | --- | ---: | ---: | ---: |
| A | Normal equations + pivoted Gaussian elimination | 1,052/1,052 assertions | 370 lines / 15,643 bytes | 16,616 bytes |
| B | Modified Gram-Schmidt QR decomposition | 1,052/1,052 assertions | 237 lines / 13,473 bytes | 17,776 bytes |
| C | Centered covariance regression | 1,052/1,052 assertions | 240 lines / 13,269 bytes | 15,712 bytes |

The measurements used Apple Clang with C++17, `-Os`, and strict warnings on September 23, 2026.
Object sizes compare the portable core only and do not predict a complete Arduino firmware image.

## What differed

Candidate A follows the familiar normal-equation shape used by many small embedded calibration routines.
It is direct, but squaring the input matrix amplifies poor conditioning.

Candidate B avoids normal equations and has the strongest general numerical story.
Its dynamic work arrays and QR machinery produced the largest object.

Candidate C centers the two raw-coordinate predictors and solves their 2 × 2 covariance system.
It is specialized to this affine problem, produced the smallest object, and keeps the intercept calculation easy to inspect.

All three produced the same observable behavior for exact transforms, noise, swapped and inverted axes, skew, singular data, the deterministic 51-target schedule, quick and imperfect tap reduction, separately collected clockwise and counterclockwise coverage, equal direction weighting, varied-radius traces, polar interpolation, asymmetric local scale, smoothing, both record versions, corruption, migration, and 100 generated transforms.

The hardware exercise corrected a deeper prompt mistake: Around the World is calibration data, not a test of how accurately the person can trace a thin circle. The revised contract collects a complete clockwise pass and a complete counterclockwise pass without radial grading, keeps their differences visible, and gives them equal weight in the outer correction ring.

The physical error also changed sign around the face. That pattern does not behave like one pixel offset. The version-2 contract therefore retains the affine seed and adds a smooth residual field at the center and three radii. Asteroid Shooter supplies known interior targets; every miss contributes a residual instead of failing the exercise.

A mutation check raised the allowed maximum error from 22 to 30 pixels in a disposable copy.
The 1,052-assertion suite failed two assertions and exited with status 1, confirming that the shared pass was not a vacuous compile check.

## Hardware iteration

The first device candidate exhausted task stack when it entered the perimeter phase because it placed an 8 KB trace array on the stack. Moving trace state out of automatic storage fixed the screen transition; the measured high-water mark then remained at 4,432 stack units through every phase.

The next candidate treated the ring as a pass/fail accuracy test. Repeated physical traces covered all sectors with good 6–10 px mean error but failed on direction, end position, or a single 23–36 px point. That behavior discarded the measurements calibration was meant to learn from.

The final version-1 candidate treated the gesture as collection. It accepted either direction and lifts, accumulated four or more samples in every sector, recorded the error pattern, refined the transform, saved it, and loaded the validated record after reset. The physical run collected 464 samples; the refined fit residual was 8.05 px mean and 16.20 px maximum.

That run did not exercise the later version-2 flow. The two directional passes, Asteroid Shooter, local warp application, and 440-byte persistence record still require a generated firmware and a new physical run.

An interim version-2 run followed. Clockwise collection used 352 samples and reported 7.54 px mean and 24.36 px maximum radial deviation. Counterclockwise collection used 389 samples and reported 9.75 px mean and 27.20 px maximum. The sector-by-sector direction difference averaged 8.67 px and reached 27.38 px.

The original 35-target shooter recorded 12.49 px mean and 25.03 px maximum miss distance. It produced a valid 440-byte generation-1 map and reached PASS. Its title and progress footer occupied the lower display during the exercise, so the next prompt revision added a discarded tap-to-start screen, removed every non-target mark during collection, and added a 170-pixel target ring.

The revised 51-target run completed with 15.15 px mean and 29.57 px maximum affine miss distance. The maximum came from the bottom-center `(234,403)` target, whose residual was approximately `(0.3,-29.6)` pixels. The candidate combined the new outer target ring with the two-direction perimeter ring, saved a 440-byte generation-2 map, and reported that version and generation loaded after reconnect.

## What the comparison changed in the prompt

The first prompt described the intended system but left several compatibility decisions open.
Independent builds could reasonably disagree about them.

The contract now defines:

- the exact core API,
- a 440-byte version-2 persistence record and migration from the 40-byte version-1 record,
- CRC-32 parameters and byte order,
- the coefficient order,
- the median rule for an even sample count,
- inclusive sample and point-verification thresholds,
- the UP, DOWN, LEFT, RIGHT endpoint order,
- separate clockwise and counterclockwise Around the World passes with equal sector weighting,
- the 51-target Asteroid Shooter coverage and miss-as-data rule,
- center-plus-three-ring local warp interpolation and safety rules,
- finite-input and null-input behavior,
- the minimum fit size,
- and the affine determinant threshold.

Those are source-prompt fixes.
None belongs as a private patch to one generated implementation.

## What remains physical

The host suite cannot prove that the Stopwatch reports stable raw touches, that a fingertip lands on a visible target, that the display and touch rotations agree, or that Preferences survives a real reboot.

The firmware acceptance section in `SPEC.md` names those checks without converting them into false host claims.
A generated firmware must compile against the real libraries, expose its serial evidence, and complete the cardinal drags, both Around the World passes, and Asteroid Shooter on the attached device.

For the portable core, Candidate C is the clearest fit for this exact two-axis regression.
The important result is that all three can be discarded and rebuilt from the same contract.
