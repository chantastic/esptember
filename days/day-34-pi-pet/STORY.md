---
title: Pi Pet
---
## It started as a visualization

The first request was small: make a Grok Bot–style visualization for the connected board.

A blob with two pill eyes, some breathing, a hop.
It ran at 9 frames per second, and after two rounds of tile tricks, 36.

Then the verdict: the eyes aren't as playful as Grok Bot's.

## A pet for every agent

The better idea arrived one message later.
What if the pet belonged to the coding agent?

One pi extension per session reports what the agent is doing.
A small hub owns the USB port and merges every session.
The Stopwatch pages between them like channels, and a pet that finishes waits until you check in.

A Tamagotchi for agents.

## The engine was already in the page

The imitation kept missing, so we stopped imitating.

The article's avatar demo is procedural SVG, and its engine ships in the page bundle:
- eight head shapes;
- 25 pairs of 48-point eye contours;
- per-state spring targets;
- blink keyframes and a decaying bounce chain;
- eye spins, and morphs into dots and `!`.

Porting it made the pet recognizable.
It didn't make it right.

## Frame by frame

Reading the code told us what the engine *could* do.
Recording the live demo at 60 frames per second told us what it *does*.

Blinks flatten a pill into a thin dash, not a dot.
Expression changes blink through instead of sliding.
Spins drag parallel ribbon lanes around the head.

A small recorder in the firmware captured the device at its own frame rate, and comparing the two sheets side by side became the whole workflow.
Every artifact we fixed (hollow eyes, dark rectangles, seams between merging dots, a ribbon fleck left behind) showed up in a recording before anyone noticed it live.

## The handoff

A second agent had studied the same demo in a browser, and its notes arrived as a handoff.
Its Done animation had been wrong; ours was the one to keep.
But its account of Thinking and Blocked was sharper than our port.

The head doesn't dissolve into the glyph.
It *becomes* the glyph: it contracts into the center dot, or drops into the bottom of the `!` while the stem falls in from above.

## The curvature question

"Are you not able to handle the curvature in the eyes?"

We could, just not the obvious way.
Exact distance to a 48-point outline cost 30 microseconds per sample and dropped the pet to 7 frames per second.
So each contour became a precomputed distance texture instead: 183 KB of flash, the active pair in RAM, the eyelid inverted exactly.
The pet was back to 30 frames per second, with bean-shaped eyes.

The reference turned out to use capsules only mid-morph.
The shortcut had been right half the time, which is exactly how it hid.

## What we learned

- Port before you polish. Imitating a character's motion by eye stalls; its engine was a download away.
- Frames beat source. The code said what was possible; sixty frames a second showed what actually happens.
- The contract finds real bugs. Writing the rules as an executable model caught firmware defects that the device had rendered without complaint.
- Budgets shape fidelity. The right contour renderer wasn't the exact one; it was the one that kept thirty frames a second.
