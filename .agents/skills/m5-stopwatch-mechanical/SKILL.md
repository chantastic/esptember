---
name: m5-stopwatch-mechanical
description: Physical geometry of the M5Stack Stopwatch (C152) for anything that holds, mounts, docks or encloses it — the tapered body and its proven 0.2 mm pocket fit, button, port, speaker and magnet positions, how to read M5Stack's size drawing correctly, reference holster dimensions, an evidence log of fit prints, and design rules for stands. Use with openscad-parts.
---

# M5Stack Stopwatch mechanical reference

The machine-readable source is
[hardware/lib/m5-stopwatch.scad](../../../hardware/lib/m5-stopwatch.scad).
Keep it and this file in step: when a measurement improves, change the
library first, then this table, then re-verify every part that `use`s it.
Worked examples are in
[hardware/stopwatch-desk-cradle/](../../../hardware/stopwatch-desk-cradle/README.md).

## Orientation

Hold the device lanyard-down, facing you, as AGENTS.md requires. In the
library's device frame, +X is your right, +Z is up the face (12 o'clock), and
the face is at −Y. Clock positions below use that view.

## Geometry

| Feature | Value | Confidence and source |
| --- | --- | --- |
| Body | A shallow frustum: Ø51.95 at the face, Ø51.4 just before the back fillet, 15.5 mm thick, 39 g. R1 face edge, R2 back edge. `m5_body()` models it. | [D] edge views (1190 → 1178 px, then the fillet). [C] only goes on back-first. **[F] Proven:** a pocket cut with `m5_body(0.2)` is a perfect fit. |
| BtnA (KEYA, G2, `M5.BtnA`) | 10:30, physical left. A 9 mm nub standing 1.3 mm proud, centred across the thickness. | [D] drawing view, [P] |
| BtnB (KEYB, G1, `M5.BtnB`) | 1:30, physical right. Same nub. | [D], [P] |
| Power (red) | 7:30, about 5.7–11.6 mm from the back. Barely proud. | [D]; [C] recesses it from 5.5 mm |
| USB-C | 6:00, about 6.4–11.6 mm from the back. The lanyard hole sits beside it. | [D] |
| HY2.0-4P (Grove: GND, 5V, G10, G11) | 4:30, about 4.3–10.7 mm from the back | [D] |
| Speaker (1 W) | Back, grille centred 15.7 mm above centre; the dots span about x ±3.3, z 12.6–18.7. `m5_speaker()` = 7.5 × 7 mm. | [D] rear view at full resolution (the earlier 10 × 10 guess was too big) |
| Magnets | Back, four at (±12.5, ±12.5), with Ø6.2 holes. [D] draws 4 × Ø5 on 25.46; use 25.0. | [C]. **[F] Proven** by the seat test. |
| Speaker recess | Back, top: tongue-shaped, 2.4 mm deep, open at the top edge. Its tip is 9.5 mm above centre and it widens to ±10.7 mm at 22 mm; the grille is centred about 15.7 mm up. Modelled by `m5_speaker_recess_2d()`. | [D] rear view (outline) and the top edge view (the 2.4 mm slot), read with `drawing_measure.py` |
| Logo recess | Back, bottom: the M5 segment, 1.48 mm deep beyond 19.8 mm below centre, with a 45° ramp up to the main plate at 18.1 mm. Modelled by `m5_logo_recess()`. | [D] side edge views (a 34 px step) |
| Rear ø4 features | Back, at (±20, 0). Ø2.2 screws also sit on that axis. | [D]; [C] clears them with Ø4.2 |
| Rear headers | 2.54 mm, 7-pin and 6-pin, flush along the left and right of the back | [P] |
| Microphone | Bottom edge, near USB-C | [P] |

The sources:
- **[D]** M5Stack's
  [C152 size drawing](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1242/C152-StopWatch-model-size.pdf).
  A 7016 px PNG render is at `…/1242/C152-StopWatch-model-size_page_01.png`,
  at about 22.9 px per mm.
- **[P]** The product photos, `…/1242/C152_stopwatch_mainpictures_01..13.webp`.
  Number 01 is labelled.
- **[F]** Fit prints made from the library. See the evidence log below.
- **[C]** *CounterClock M5 Case v2*, a working third-party holster the user
  shared. Its seat is Ø52.0 (a press fit), 4 mm thick, and 15 mm deep. Its
  wall runs from 2:30 through 6 to 9:30, with the top half open. It has an
  opening at 6 o'clock, a sliding recess for the power nub, and a speaker
  notch in the plate. It isn't in this repository.

Every position marked "about" was read off a drawing or photo, to about
±1 mm. Physical checks on a real unit beat all of them. Record them in the
library with the date.

## Evidence log

Add a line whenever a print meets the real device. Say what it proved, and
what it didn't.

- **2026-09-29 · puck dock seat test.** `seat_only`, `fit = 0.2` over
  `m5_body()`, 7 mm lip with keep-out windows: a perfect fit. The Ø6.2
  magnet holes at 25.0 mm pitch line up. 6 × 2 mm magnets press-fit into
  them without glue. Pressed flush with the seat in the test's 2.6 mm
  through-holes, they left three layer lines (0.2 + 2 × 0.24 = 0.68 mm)
  below them, so they're about 1.92 mm thick. Pockets are now 2.4 mm deep. This proves the body diameter, the
  taper, the back fillet and the magnet pattern as modelled. The speaker
  notch, port windows and cable path are not yet proven. It was printed on
  a Bambu Lab A1 mini with a 0.4 mm nozzle at 0.24 mm draft layers. The
  filament wasn't recorded.
- **2026-09-30 · full puck dock (knife-cut ball version)**, printed by the
  user. Their verdict: the 1.2 mm top rim was best, so use it all the way
  round. The front cut wasn't needed. The seat should match the back: a
  deeper finger pocket at the speaker recess to pull the device out, a step
  matching the logo recess, and no screw holes. That's the current design,
  which isn't printed yet.
- **2026-09-29 · first seat test** (superseded). A straight Ø52.75 pocket
  (`fit = 0.4`) with a 4 mm lip felt slightly large. The lip only reached the
  narrow, rounded back of the body.
- **Reference holster [C].** Its straight Ø52.0 seat goes on back-first
  only. That was the clue to the taper: the face end is the wide one.

## Mistakes to avoid

These all happened while designing the first stands:

- Taking button positions from the repo's board illustration. That put them
  at 9 and 3 o'clock instead of 10:30 and 1:30. Start from the official
  drawing and the photos, and cross-check each position against a second
  source.
- Reading the drawing upright. It's rotated 180° and uses first-angle
  projection (see below).
- Assuming a cylinder. The seat felt loose until the pocket followed the
  taper and back fillet.
- A lip too short to grip. At 4 mm, the lip sat almost entirely on the
  back's R2 fillet.
- Ignoring how the user describes a reference part. "It only fits
  back-first" is a measurement, so ask how reference parts behave.

## Reading M5Stack's drawing

- The drawing is **rotated 180°** from lanyard-down. Its rear view puts the
  speaker at the bottom, and its front view shows the button nubs at 4:30
  and 7:30. Rotate everything by 6 hours.
- It shows the taper but doesn't dimension it. Measure the edge views at
  full resolution with `openscad-parts/scripts/drawing_measure.py`, at 22.9 px
  per mm. The Ø51.95 front-view circle is 1190 px.
- It uses **first-angle projection**. The view drawn below the front view
  looks down on the drawing's top edge: Grove, USB-C, lanyard and power.
  The view above it shows the two buttons. In first-angle, the edge of a view
  nearest the front view is the device's *back*, so axial positions are
  measured from the back. The holster's power recess (from 5.5 mm) confirms
  this.
- This repository's older board illustration,
  `public/images/boards/m5stack-stopwatch.svg`, draws the buttons near 9 and
  3 o'clock. Don't take geometry from it.

## Design rules for stands, docks and cases

- **Keep clear:** finger room at both buttons and at power, plug room at
  USB-C (an overmold is about 12 × 7 mm) and at Grove, and an air gap behind
  the speaker. `m5_keepouts()` models all of these. Subtract it, or prove an
  empty intersection.
- **Where to grip:** grip at 2:30–4:00 and 8:00–9:30, the arcs between
  the buttons and the bottom-edge features. There the rim can rise to about
  7 mm. Across the bottom arc, keep it under 4.3 mm so it passes below
  power, USB-C and Grove (see "No windows needed" below). A rim that's low
  all the way round sits on the back fillet and doesn't grip.
- **Seat on the back.** It isn't flat: it has a 2.4 mm speaker recess at the
  top and a 1.48 mm logo recess at the bottom. Test clearance against
  `m5_body_true()`, which includes them. A pocket that follows the speaker
  recess but sinks about 6 mm is a natural finger pull. A step that fills
  the logo recess registers the device, and it pivots on that step when
  pulled from the top. The ø4 features at (±20, 0) don't need clearance
  holes; the user asked for them to be removed. Put magnet holes at the
  four positions. Ø6.2 holes are a proven
  press fit for the user's 6 × 2 mm magnets, with no glue. Make blind
  pockets the magnet's thickness plus about two layers (2.4 mm for 6 × 2).
  The user prefers a magnet slightly deep to one that doesn't go in fully:
  a proud magnet lifts the device off the seat, while 0.5 mm of recess
  costs only a little pull. Check each magnet's polarity against
  the device before pressing it in, because pulling one back out damages the
  hole.
- **Buried magnets** (`magnet_skin`, parked and untested; only the upright
  cradle still has the option): keep the Ø6.2 press-fit
  pocket, leave a skin between it and the seat, and load the magnets from
  behind through a Ø6.6 channel along the magnet axis. Print-in magnets,
  with a pause at a layer, don't work on a tilted seat: the skin starts
  before the cavity floor finishes. On the tilted puck dock, the channels
  exit under the foot (lower pair) and through the rear skirt (upper
  pair).
- **Fit.** Use `m5_body(0.2)` as the pocket cutter. It's proven on the
  user's unit. Never use a straight cylinder at the face diameter: the body
  tapers and the back is the narrow end, so a straight seat is either loose
  at the back or tight at the face. The taper may come from the screen being
  pressed in rather than the design, so treat it as a property of real units,
  not of the drawing. On a new printer or filament, re-run the seat test
  before a full print, and adjust `fit` in 0.05 mm steps.
- **Grip height.** The back 2 mm is the R2 fillet, so a lip shorter than
  about 4 mm barely touches the body. Grip with 6–8 mm, but only where
  there's nothing to clear. Prefer varying the rim height to cutting
  windows. If a design does need windows, cut them from
  `m5_keepouts(grow = 0.1)`.
- **Cable.** USB-C points straight down out of the bottom edge. The user's
  cables won't thread through a tunnel, so give them an open, rounded
  channel under the part that the cable lies into from below. Keep
  `m5_usb_plug()` (a 28 mm straight plug) clear of the part and of the
  desk: on a tilted dock, that sets how high the front must sit (11.5 mm on
  the 30° puck dock).
- **No windows needed below 4.3 mm.** A rim that stays under 4.3 mm from
  the back across the bottom arc (4:00–8:00) clears Grove, USB-C and power
  without cutting anything. Grip higher (about 7 mm) at 3 and 9 o'clock, and
  just cover the R2 fillet (about 1.2 mm) across the top, so the part ends
  at the device's silhouette.
- **IMU.** The measured axes (+x toward the lanyard) assume the device
  stands upright. A stand that tilts it offsets every tilt-reading app by
  the tilt angle. Say so in the part's README.
- **Touch calibration** lives on the device, so a stand never changes it.
  If edge touches feel wrong while docked, rerun the Day 07 calibration;
  don't adjust the design for it.
