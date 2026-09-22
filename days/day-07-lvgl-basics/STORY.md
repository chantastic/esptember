---
title: LVGL Basics
---

Six days of putting things on a screen.
Today the screen pushes back.

The build itself was the easy part: a flex column of widgets, callbacks on three of them, a second screen behind a button.
The first flash looked right.

Then touch didn't behave, and the day became about proving what works when you can't trust your finger.

## The suspect list

The report was "touch is way off."
That sentence contains at least four different bugs: a dead controller, a starved event loop, a coordinate transform, a misdrawn layout.
Each one lies in a different place in the stack.

So we split the stack.
A raw I²C poll read the CST816S directly — registers 0x01 through 0x06, finger count and coordinates — while LVGL reported what its input device saw.

```c
        uint8_t reg = 0x01, buf[6] = {0};
```

They matched, sample for sample.
The controller works, the BSP driver works, LVGL sees every touch.
Whatever is off lives in the mapping between panel image and touch coordinates — a calibration question, not a plumbing one, and one that needs fingers on corners to settle.

## The virtual finger

Day 12 taught this project that firmware can testify on its own behalf: give it a serial protocol, let a script interrogate it.
Day 07 extends that to touch.

LVGL doesn't care where input comes from.
A second pointer device whose read callback replays a scripted gesture is indistinguishable from a finger — same hit-testing, same events, same animations.
The harness takes `tap 184 108` over USB and the Count button presses.
It takes `capture` and returns the actual rendered frame, base64 row by row, reassembled into a PNG on the host.

The widget logic verified end to end: presses counted, brightness tracked the drag, the switch toggled, About slid in and back out with state intact.
Screenshots to prove it, captured from the device itself.

## What the harness caught

The capture command died at row 242 of 448 — every time, same row.
The dump loop hogged the CPU long enough that the idle task starved and the task watchdog fired, printing its complaint into the middle of the pixel stream.
One yield every 32 rows fixed it.
Deterministic failures are gifts: anything that breaks the same way twice is already half-diagnosed.

Then the script's first taps missed.
A hard fling toward the About button over-scrolled, snapped back elastically, and left the switch sitting where the script expected the button — so the "tap About" toggled Orange mode instead.
Hit-testing doesn't grade on intent.
A gentler drag, a capture to see where things actually settled, and coordinates read from the screenshot instead of the imagination.

## The open item

Physical touch calibration — whether a finger on the panel lands where the panel draws — still needs eyes and fingers on hardware.
The harness ships a `touchlog on` command that streams raw controller coordinates for exactly that session.
The lesson's claims stand on the injected path; the physical path has a pending appointment.

## What we learned

- A widget is create-then-subscribe. The whole library is that one pattern with different shapes.
- Scrolling is free. If the layout overflows, the container scrolls; you never build it.
- Screens don't die when they leave. A swap changes what's drawn, not what exists.
- A virtual finger is a real finger. Input injection through a second indev turns UI testing into a serial protocol.

Six days of output, one day of input, and the real lesson is neither: when you can't trust your senses, build the instrument.
