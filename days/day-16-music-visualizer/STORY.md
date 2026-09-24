---
title: Music Visualizer
---
## The prompt-first pass

The original build below records how this idea first worked.
The durable lesson now targets the M5Stack Stopwatch and lives in the prompt, behavior contract, generalized tests, and hand-review checklist.
Generated firmware is evidence for those sources, not the source itself.


The FFT was going to be imported.
Every ESP32 audio project pulls in a library for it, and every one of those libraries wraps the same thirty lines of Cooley-Tukey that have existed since 1965.

Thirty lines is not a dependency.
Written out, the bit-reversal and the butterflies stop being incantation and become inspectable — and when the display later showed garbage, there was no black box to distrust.

## The garbage was upstream anyway

First boot: every bar pinned high in a quiet room, bass bar always the winner.
The FFT was innocent.

A MEMS mic rides on a DC bias, and DC is frequency zero — but a window function *smears* it, leaking that huge constant into the first several bins.
The fix is one subtraction: remove the mean before windowing.
Average levels dropped by a third and the bass bar stopped lying.

The lesson generalizes: in a capture → transform → display pipeline, the transform is usually the most-tested part.
Debug the edges first.

## Ears are logarithmic twice

Two log scales hide in a visualizer that feels right.

Frequency: the bars split 125 Hz–8 kHz geometrically, so each bar covers the same number of musical semitones.
Linear spacing would spend twenty bars on frequencies above the top of a piano and two on everything below middle C.

Amplitude: bar length maps dB, not magnitude.
Raw magnitudes would make quiet sounds invisible and loud ones indistinguishable.

Get either wrong and the display technically works while looking broken.

## Attack and decay

Bars rise instantly and fall four levels per frame.
That asymmetry is older than digital audio — analog VU meters had ballistics chosen so needles caught transients but didn't seizure.
Instant-rise/slow-fall makes beats *visible*: the hit snaps up, the decay draws its tail.
Symmetric dynamics turn music into static.

## The 47-millisecond frame

First hands-on report: “VERY slow.”
Instrumenting the loop split the frame into capture (0 ms), FFT (0 ms), drawing (7 ms) — and 37–47 ms of pushing the sprite over the display bus.
The math everyone worries about was free; the pixels were the cost.
Dropping the sprite to 8-bit color and shrinking it to the ring's bounding box helped less than hoped — the bus, not the byte count, sets the floor — and the honest fix was making the decay snappier so 22 fps *feels* alive.
Measure before optimizing: the FFT was never the suspect, and “slow” lived somewhere no algorithm could fix.

## What we learned

- Thirty lines is not a dependency. Owning the FFT cost nothing and made the pipeline debuggable.
- Debug the edges, not the transform. The bug lives in capture (DC bias) or display (scaling), almost never in the math everyone tests.
- Ears are logarithmic twice — space frequency geometrically and amplitude in dB, or the display lies.
- Meter ballistics are interaction design. Instant rise, slow fall: the asymmetry is what makes sound look like music.
