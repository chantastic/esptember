# Embedded rendering on the Stopwatch — measured techniques

Findings from porting the Grok Bot avatar engine to the M5Stack Stopwatch (ESP32-S3, 240 MHz, 8 MB OPI PSRAM, 468 × 466 AMOLED). The pipeline is Arduino ESP32 3.3.10, M5Unified 0.2.19, and M5GFX 0.2.26, with no LVGL. The figures are injected-device measurements from September 24, 2026, reported by the firmware's `GB_FPS` line (`render_ms`, `fill`, `tiles`, `shaded`). They are not physical review. Re-measure before relying on them; a later candidate may differ.

## Frame budget

- One full-screen RGB565 `M5Canvas` lives in PSRAM (436 KB). Render into it, then push only dirty rectangles: set a clip rect, then `pushSprite`. A roughly 280 × 280 push costs about 8–10 ms. Push the status band as a second small rectangle instead of widening the union.
- Full redraws while a label is visible, during sleep, or during confetti/ribbons halve the frame rate. Size the dirty box to the content (head reach 124 units, 142 with ribbons) rather than forcing a full redraw.
- The sprite buffer is **byte-swapped** RGB565. APIs such as `fillRect` and `setTextColor` take native 565.

Measured per state after the optimizations below, including the contour eyes:

| State | fps |
| --- | --- |
| idle | 28–31 |
| thinking | ≈26 |
| blocked | ≈27 |
| done, with ribbons | ≈25 |
| working | ≈24 |

## Distance-field rendering

- **One field per pixel:** evaluate a signed distance per pixel in head-local coordinates: translate, rotate, then scale. Coverage is `clamp(.5 − d·px_per_unit)`.
- **Tile classification:** the fields are 1-Lipschitz in screen pixels, so one sample at a tile center classifies the whole tile.
  - Use 8 × 8 tiles with reach `4·√2 + 1`. Tiles that straddle an edge split into 4 × 4 quarters (reach `2·√2 + 1`).
  - Fully inside or outside tiles get flat fills; only edge leaves are shaded.
  - Shading fell from about 11,000 to about 6,600 pixels per frame, and the frame rate rose from 9 to about 31 fps.
- **Keep scaling on the geometry:** apply non-uniform squash to the geometry, never to the distance. Stretching coordinates by 1/lid and multiplying the distance by the lid keeps the zero set but blurs the antialiasing by 1/lid; it looked like faint slivers. Either shrink the primitive's parameters, or invert the scale exactly and correct with the gradient (see *Eyes*).
- **Combining fields:** a linear blend of two SDFs stays 1-Lipschitz, so shape-to-shape morphs (head A → head B) remain classifiable.

## Memory placement and math

- **Keep textures out of flash reads:** sampling int8 textures from flash (`.rodata`) on the hot path cost about 2× the whole frame. Copy the active head textures (2 × 16 KB) and the active eye pair (2 × up to 7.6 KB) into internal RAM when they change.
- **Replace `hypotf` and `sqrtf`:** both are slow here. A one-Newton-step inverse-square-root `fsqrt` (about 0.2 % error) is ample for coverage. Hoist reciprocals such as texture step, eye foreshortening, and core scale out of per-pixel code. Per-pixel cost fell from about 2.7 to about 1.8 µs.
- **Build flags:** `#pragma GCC optimize("O2")` at the top of the sketch helps. The Arduino default is `-Os`.
- **Arduino prototype hoisting:** the Arduino preprocessor hoists function prototypes above in-sketch `struct` definitions. Put any type used in a function signature in a header (`types.h`).
- **Stack:** the loop task has about 8 KB of stack, with about 4.6 KB free in this candidate. Large per-frame arrays belong in static storage or in lambdas' enclosing scope with care.

## Eyes: true contours at embedded cost

The reference draws a **settled** expression's true 48-point contour. While morphing, it interpolates capsule fits: `eX` → `eJ` (fit) → `eq` (capsule polygon), near characters 70,300–71,700 of the saved bundle. A capsule-only renderer is therefore correct mid-morph but loses curvature at rest.

| Approach | Result |
| --- | --- |
| Capsules only | Fast, but at rest the fit deviates by up to about 5.6 units (expression 20, right eye). |
| Exact 48-edge polygon SDF per pixel | About 17–30 µs per evaluation and 3,000–4,000 evaluations per frame: **7–19 fps**. A bounding-box lower bound and per-leaf candidate edges still left about 18 fps, because thin eyes keep both sides as candidates. |
| **Per-contour SDF textures** (chosen) | 50 textures on a 1-unit grid, int8 at quarter units, centred on each contour centroid with 7 units of margin: 182,684 bytes of flash, active pair in RAM. **About 30 fps.** |

The lid squash is applied to the points along the reference's lid axis: the principal axis, blended toward vertical for round eyes by the smoothstep of anisotropy (0.12…0.30). On the device:

1. Map a pixel to eye-local contour units.
2. Undo the one-axis scale exactly: `q' = q + (g/(1−g))·(a·q)·a`, where `g = 1 − lid`.
3. Sample the texture.
4. Divide by `sqrt(1 + ((1/(1−g))² − 1)·(a·n)²)`, where `n` is the texture gradient by central difference. Only do this near the edge (|d| < 6); farther away, use the conservative lower bound `d·(1 − g)`.

A closing pill then becomes the reference's thin dash with crisp edges. Use capsules only while `blend < .999`.

Precompute each contour's lid axis and centroid offline with the reference rule. Scale the eye by the face-fit eye scale and cap it so the two eyes never merge. Confine the eye using its squashed extents: test the four extremes against the head field and push the center inward.

## Glyphs, compositing, and motion

- **Glyph parts composite "over":** the retained head core, satellites, and stem each have an opacity; combine them as `1 − Π(1 − coverageᵢ·alphaᵢ)`.
  - Proximity-weighted opacity blending produced dark rectangles where a fading stem overlapped the opaque head, and seams between merging dots.
  - Flat-fill a tile only when every part is wholly in or out of it.
- **Eyes follow the core:** eyes live inside the retained head's transform. Evaluate them in core coordinates, `((u, v − coreY) / coreScale)`, so they contract with the head; otherwise they leave artifacts during entry.
- **Ribbons:** antialiased `drawWideLine` for orbit ribbons dropped the frame rate to about 17 fps. Two `fillTriangle` calls per segment (12 segments per ribbon, up to 9 ribbons) restored about 25 fps. Draw back halves before the body and front halves after it. Fade by width at full color; blending toward the background paints dark streaks across the body.
- **Springs:** use the reference form `v += (−2ζω·v − ω²(x − target))·dt` in fixed 1/120 s substeps, independent of display frame rate.

## Display and platform

- **Background:** the user chose pure black. AMOLED pixels are off there, which minimizes power and burn-in. Some OLEDs smear when pixels turn on from full black; if that is visible, use near-black. Eye holes show the background, so they are black too.
- **Panel:** correct the panel to 468 × 466 with x offset 6 immediately after `M5.begin()`. Load `espt-touch/record` and map raw touches through the shared warp.
- **Flashing:** flash app-only at `0x10000` to preserve NVS. Compare the partition table first. A merged image at `0x0` is a fresh install that can erase calibration.

## Wireless link (Wi-Fi and USB)

- **Let the Mac dial out.** A hub-side TCP listener was silently dropped by the macOS application firewall in stealth mode: the board connected, but no data came back and no prompt appeared for an unattended `node`. Have the board listen, advertising `pi-pet-<id4>.local` and `_pi-pet._tcp`, and let the hub dial it. Outgoing connections need no approval.
- **Never let USB CDC block.** With the host port closed (a released hub or a charger-only connection), HWCDC writes wait out a timeout per call and stalled the main loop, and with it the Wi-Fi link, for about 25 s. Call `Serial.setTxTimeoutMs(0)` right after `Serial.begin()`.
- **Keep the socket away from the render path.** Run Wi-Fi joins, accepts, and the handshake in a FreeRTOS task on core 0, so connects never stall the animation. The main loop owns the socket only while the link is up, with a mutex around reads, writes, and replacement. Buffer mirrored `GB_*` output per line; per-byte socket writes are slow.
- **Latency and cost:** with modem sleep, hub-to-board latency was about 0.1–0.2 s. The Wi-Fi stack added about 600 KB of flash. USB is the trusted provisioning channel: pairing tokens and Wi-Fi credentials never cross the network.

