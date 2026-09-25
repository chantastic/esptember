# Thinking and Blocked geometry — September 24, 2026

The first browser study substituted whole-face opacity fades for the original glyph transitions. The user rejected both. This was a study simplification, not evidence of an ESP32 rendering error.

Source: the saved [xAI animation bundle](upstream/article-animation.js), captured from [Designing Grok Bot](https://x.ai/news/designing-grok-bot). The equations below describe the relevant source behavior. Names and approximate character offsets refer only to that saved bundle: `ts` near 75,724; `ae`/`at` near 108,147; `ac` near 115,400; shared transforms near 123,000–124,900; lifecycle near 126,500; springs near 135,800. Inspect these as source text, not a runnable module.

## Shared retained head

Use `C=114.2705`, morph progress `p=clamp(E.x,0,1)`, and a critically damped spring with frequency 14. Glyph entry starts immediately; the study's previous 1.2-second Thinking delay was not this reference behavior.

- Interpolate the head's 96-point radial ring toward a circle of radius C. The interpolation weight is cubic ease-in-out of `clamp(p/.62)`; the shape finishes rounding before its overall contraction finishes.
- Keep the head itself and transform it into the glyph's main dot. Body offsets and rotation decay with `1-p`. Do not fade the full head to zero while drawing an unrelated symbol over it.
- Eyes rotate cylindrically through a half-turn on glyph entry, using another frequency-14 spring. Include foreshortening and rear-face suppression; draw eyes only while `p<.5`. Exit adds the second half-turn in the same direction.
- Retain the outgoing glyph during exit. A direct Thinking↔Blocked change keeps glyph presence active, blends styles using a frequency-11 spring, and does not add another half-turn.
- The original uses a randomly signed spin. The deterministic study always uses the positive direction. Circle geometry, timing, and glyph behavior are based on source; the study's analytic alternate heads and eye confinement remain simplified.

## Thinking

The head becomes the center dot, radius 22 source units. Outer dot centers settle at `x=C±62`. The core's base scale is `(1-p)+(22/C)*pop*p`; its color retains opacity `1-(1-tone)*p`.

For time in seconds since Thinking activation, let `phase=positiveModulo(time/1.4+.119,1)`. For dot index `j=0,1,2`, let `d` be the shortest cyclic distance from phase to `j/3`, and `s=exp(-d*d/.045)`:

```text
lift = 9*s*p
pop  = .84+.22*s
tone = .5+.5*s
```

The core's vertical offset includes `-lift*p`. Satellites use `-lift` and have a staggered progress `u=clamp((p-.12*side)/(1-.12*side))`. Their growth is cubic ease-out, spread is ease-out-back, radius is `22*growth*pop*1.02`, and opacity is `growth*tone`. The wave changes lift, size, and brightness together. Three identical sine bobs do not reproduce it.

## Blocked

The retained head becomes the **lower dot**, not the stem. Let `B=exp(-5.5*(time%2.2))`:

```text
dot scale    = (1-p)+(13/C)*(1+.04*B*p)*p
dot center y = C+58*p*p
dot opacity  = 1
```

The separate tapered stem uses source path `tH`:

```text
M99.2705 81.2705 A15 15 0 0 1 129.2705 81.2705
L122.7705 153.7705 A8.5 8.5 0 0 1 105.7705 153.7705 Z
```

With weight `e=p` for ordinary entry, use `r=cubicEaseOut(clamp(1.1*e))`, scale `clamp(1.2*e)`, opacity `clamp(1.5*e-.2)`, vertical translation `-26-70*(1-r)`, and rotation `2.2*sin(42*time)*B` degrees. Apply translation, rotation about `(C,40.3)`, then scale about `(C,C)`, in that order. The stem fades as one part of its birth; the retained head stays opaque. Blocked remains a glyph until state changes; automatic glyph/avatar alternation belongs only to upstream progress/spawning.

## Study implementation and checks

The revised study preserves spring positions/velocities and current eye contours when a new state starts, and records that starting pose for repeatable seeking. It uses separate pulse clocks for the two glyphs so switching, including reversing a partly completed switch, does not reset an outgoing visible pulse. This continuity improvement differs from the original's shared clock reset.

Browser checks inspected entry around 67/150/483 ms, settled glyphs, a full Thinking cycle, Blocked→Idle expansion, Thinking→Blocked without an initial pose jump, and frame stepping. A separate source review checked the stem's transform order and shared-head formulas. This is browser/source evidence, not device or pixel-identical parity evidence.

Reduced motion settles the glyph without automatic playback or traveling pulses. Keep the status text available so state meaning does not depend on motion.

## Device implementation (Pi Pet revision 3)

Implemented in the device candidate after the handoff. Record changes here rather than editing the reference sections above.

- **Distance fields:** everything is one signed-distance field per pixel in head-local units, with the parts composited "over" each other.
  - **Core:** the retained head, sampled at `(u, v − coreY)/coreScale` and blended toward the circle `|q| − C` by the 62 % rounding weight. When fully rounded, it skips the texture.
  - **Satellites:** analytic circles.
  - **Stem:** an uneven capsule (cap r 15 at −33, foot r 8.5 at +39.5) under the inverse of drop → rotation about y = 40.3 − C → scale. This matches the study's canvas order.
  - **Eyes:** evaluated in core space, so they contract with the head.
- **Transforms:** body offsets, rotation, squash, and bounce are multiplied by `1 − g`. Style uses a frequency-11 spring and snaps to the target when entering from no glyph. The eye half-turn uses a frequency-14 spring about the vertical axis, direction randomized per entry. It resets after the exit half-turn completes a full turn.
- **Adaptations:**
  - Thinking entry is debounced by 350 ms so sub-second tool bursts don't flicker the face.
  - Separate pulse clocks start at each state's entry.
- **Evidence:** device `rec` sequences in `assets/review/device-idle-to-thinking.png`, `device-thinking-to-blocked.png`, and `device-blocked-to-idle.png`.
  - The first recordings exposed eyes that didn't contract, and dark rectangles from proximity-weighted opacity. Both were fixed before these captures.
  - The Idle→Thinking and Blocked→Idle sheets predate contour eyes.
  - This is injected-device evidence, not physical review.
