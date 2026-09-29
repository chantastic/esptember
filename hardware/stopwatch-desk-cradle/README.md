# M5Stack Stopwatch desk stands

There are two printable stands for the M5Stack Stopwatch (C152). Both hold it
lanyard-down, with both buttons, the power button, USB-C and Grove all
exposed, and both route the cable out behind. The device's geometry lives in
one shared file, [../lib/m5-stopwatch.scad](../lib/m5-stopwatch.scad). Each
`.scad` file has its settings at the top, and each `.stl` file is a
ready-to-print render of the defaults.

| | Puck dock | Upright cradle |
| --- | --- | --- |
| Source | [stopwatch-puck-dock.scad](stopwatch-puck-dock.scad) | [stopwatch-desk-cradle.scad](stopwatch-desk-cradle.scad) |
| Print file | [stopwatch-puck-dock.stl](stopwatch-puck-dock.stl) | [stopwatch-desk-cradle.stl](stopwatch-desk-cradle.stl) |
| Face angle | 30° from horizontal | 72° from horizontal |
| Size | 58 × 54 mm, 34 mm tall, 44 mm round foot | 70 × 50 mm, 58 mm tall |
| Holds the device by | A seat plate and a 7 mm lower-half lip with port windows | Cheeks at 3 and 9 o'clock, plus a back rest |
| Cable | A tunnel under the device to a rear port | A tunnel under the back rest |

In the previews, the translucent shapes are the device and the space each
stand must leave clear: finger room at the buttons and power button, plug
room at USB-C and Grove, and an air gap behind the speaker.

## Where things are on the Stopwatch

These positions are for the device held lanyard-down, facing you.

| Feature | Position | Source |
| --- | --- | --- |
| BtnA (KEYA, G2) | 10:30, a 9 mm nub | Size drawing, photos |
| BtnB (KEYB, G1) | 1:30, a 9 mm nub | Size drawing, photos |
| Power button | 7:30, 5.7–11.6 mm from the back | Size drawing, the holster below |
| USB-C and lanyard hole | 6:00, 6.4–11.6 mm from the back | Size drawing |
| HY2.0-4P (Grove) | 4:30, 4.3–10.7 mm from the back | Size drawing |
| Speaker | Back, about 17 mm above centre | Size drawing, photos |
| Magnets | Back, four on a square, 25.0 mm apart | The holster, then proven by the seat test; the drawing says 4 × Ø5 on 25.46 |
| Rear ø4 features | Back, at (±20, 0) | Size drawing; the holster clears them with Ø4.2 |
| Body | Ø51.95 at the face, about Ø51.35 at the back, 15.5 mm thick; R1 face edge, R2 back edge | Size drawing edge views; the holster only fits back-first |

The sources are M5Stack's
[C152 size drawing](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1242/C152-StopWatch-model-size.pdf),
the [product photos](https://docs.m5stack.com/en/core/StopWatch), and
measurements of a working third-party holster, *CounterClock M5 Case v2*.
The drawing is printed rotated 180° from lanyard-down: its buttons appear at
4:30 and 7:30. It also uses first-angle projection. Read it wrongly and the
buttons end up at 3 and 9 o'clock, which an earlier revision of these stands
did.

## Puck dock

![Puck dock](puck-dock-preview.png)
![Puck dock with the device and its keep-outs](puck-dock-side.png)
![Puck dock from behind, with the cable port](puck-dock-rear.png)

The shape borrows from a sleek harness for a case-less round display. It's a
thin seat plate on a skirt that sweeps in to a small round foot, with a
0.4 mm shoulder step where they meet. The seat follows the holster's open
layout:

- **The pocket follows the body.** The Stopwatch tapers: it's about 0.6 mm
  smaller across at the back than at the face, and its back edge has a 2 mm
  radius. The pocket copies that shape with 0.2 mm clearance, so the device
  settles into the centre instead of rattling.
- **The lip** wraps the lower half from 2:30 to 9:30 and rises 7 mm up the
  device's side. It has windows for Grove (4:30), USB-C (6) and power
  (7:30), cut from the shared keep-outs. The whole top half is open. Gravity
  and the magnets hold the device against the seat.
- **Magnet holes:** four Ø6.2 × 2.4 mm blind holes, lined up with the
  device's magnets. A 6 × 2 mm magnet pressed to the bottom sits 0.48 mm
  (two draft layers) below the seat: deliberately a little deep rather than
  risk it standing proud. For other magnets, set `magnet_hole_t` to the
  magnet's thickness plus about 0.5 mm. The seat plate is 2.84 mm, which
  lands the flat seat test on the draft layer grid and leaves a two-layer
  floor under each magnet. Magnets press-fit into them without glue (proven on the
  seat test). Check each magnet's polarity against the device before
  pressing it in, because they're hard to get out.
- **Buried magnets (optional, untested):** `magnet_skin` hides the magnets
  under a thin skin, so the seat is unbroken and nothing metal touches the
  device's back. The press-fit pocket stays, and a looser Ø6.6 channel behind
  it runs out through the underside. The lower two magnets load from under
  the foot, the upper two from small holes in the rear skirt. Push each one
  in with a rod until it seats against the skin. Use 0.6–0.8 mm on this
  tilted seat: a two-layer skin prints as a thin staircase and may have
  pinholes.
- **Speaker notch:** a 10 mm notch through the top of the seat, behind the
  rear speaker.
- **Cable:** a 14 mm slot at 6 o'clock takes the USB-C plug and the
  lanyard. A 5.5 mm tunnel with a peaked roof runs from the bottom of that
  slot, under the device, to a port at the back of the skirt. A right-angle
  (90°) plug turned downward suits this path best. A straight plug sticks
  out toward you.
- **Printing:** print it on its foot without supports. The tunnel roof is
  45°, and the skirt reaches about 50° over a few square millimetres beside
  the front shoulder.

## Upright cradle

![Upright cradle with the device and its keep-outs](preview.png)

- **Cheeks** cup the rim from 12° above to 28° below horizontal, which lies
  between the buttons (35° above and higher) and Grove and power (35° below
  and lower). A 1.5 mm front lip on the cheeks keeps the device from sliding
  forward.
- **The back rest** reaches 16 mm above centre, so it covers all four magnet
  holes. It has the same speaker notch.
- **The bottom edge** is open from 4 to 8 o'clock. The device sits 14 mm
  above the base, and a 12 × 9 mm tunnel under the back rest carries the
  cable behind.
- **Printing:** print it on its base without supports. The only flat
  ceiling is the tunnel roof, a 12 mm bridge.

## Rendering and printing

Run these from the repository root:

```bash
openscad --backend=manifold --export-format binstl -o hardware/stopwatch-desk-cradle/stopwatch-puck-dock.stl hardware/stopwatch-desk-cradle/stopwatch-puck-dock.scad
```

```bash
openscad --backend=manifold --export-format binstl -o hardware/stopwatch-desk-cradle/stopwatch-desk-cradle.stl hardware/stopwatch-desk-cradle/stopwatch-desk-cradle.scad
```

To try buried magnets on the seat test (parked for now), add
`-D magnet_skin=0.48` to the command below. That's two 0.24 mm layers, and
it's only true layers here because the seat test prints flat. The magnets push in
from the bottom.

For a 10-minute fit check of the puck dock, print just the seat and lip, a
10.04 mm ring (0.2 + 41 × 0.24, so it lands on the draft layer grid):

```bash
openscad --backend=manifold --export-format binstl -D seat_only=true -o hardware/stopwatch-desk-cradle/seat-test.stl hardware/stopwatch-desk-cradle/stopwatch-puck-dock.scad
```

- Open the STL in Bambu Studio and print it as modelled.
- Use PLA or PETG, 0.2 mm layers, 3 walls, and 15–20% infill.
- The fit is `fit = 0.2` mm per side, over the tapered body. The holster's
  straight Ø52.0 seat only goes on back-first, because the face end is the
  wide one. If the seat test binds, raise `fit` by 0.1. If it rattles, lower
  it by 0.05.

## Acceptance

Each layer is separate. Passing a CAD check says nothing about a physical
check.

**CAD** (see the `openscad-parts` skill)
- The file renders with the Manifold backend and no warnings.
- `mesh_check.py` reports one shell, 0 non-manifold edges and 0
  near-duplicate vertices.
- Intersecting the stand with `m5_body()`, `m5_keepouts()` and the button
  nubs renders empty.

*Recorded · September 29, 2026,* with OpenSCAD 2026.09.29 (snapshot):
- **Puck dock:** 15,106 triangles, genus 1 (the cable tunnel), 0
  non-manifold edges, 0 near-duplicate vertices. The keep-out intersection
  was empty.
- **Upright cradle:** 7,194 triangles, genus 1 (the cable tunnel), 0
  non-manifold edges, 0 near-duplicate vertices. The keep-out intersection
  was empty.
- **Seat test print:** genus 0 (the 2.4 mm magnet pockets are blind, as in
  the dock, with a 0.44 mm two-layer floor), with 0 non-manifold edges. The first, proven
  seat test had 3.2 mm pockets, which went through its 2.6 mm plate.

The first seat test printed at `fit = 0.4` with a straight Ø52.75 pocket, and
it felt slightly large. The 4 mm lip only reached the narrow, rounded back of
the body. That led to the tapered pocket, the 0.2 mm fit and the 7 mm
windowed lip.

This is evidence about the models only. Nothing has been printed or fitted
yet.

**Physical fit** (printed part plus a real Stopwatch)

*Recorded · September 29, 2026:* the puck dock's seat test (`seat_only`,
`fit = 0.2`, tapered pocket, 7 mm windowed lip) was a perfect fit on the
user's Stopwatch. Its magnet holes (Ø6.2 at 25.0 mm pitch) line up, and
6 × 2 mm magnets press-fit into them without glue. Pressed flush with the
seat, they left three layer lines of hole wall below them, which puts
their thickness at about 1.92 mm. The pockets are now 2.4 mm deep, so a
magnet pressed to the bottom sits two draft layers (0.48 mm) below the
seat. It always goes in fully and can never lift the device. That
covers the seat diameter, the taper, the lip and the magnet pattern. The
speaker notch, the port windows, the cable path and the full dock are still
untested. It was printed on a Bambu Lab A1 mini with a 0.4 mm nozzle at
0.24 mm draft layers. The filament wasn't recorded. Re-run the seat test
before trusting `fit` on another printer, nozzle or layer height.

- The device drops in and lifts out with one hand. It doesn't rattle when
  the desk is tapped.
- BtnA (left, 10:30) and BtnB (right, 1:30) click fully while docked, alone
  and as a two-button chord. Power presses, and USB-C charges with the cable
  in the tunnel.
- The speaker is no quieter docked than held in the hand.
- Tapping anywhere on the screen doesn't tip or slide the stand.

**Software note**
- The IMU axes this repo measured (IMU +x toward the lanyard) assume the
  device stands vertically. Docked, the device sits 60° off upright in the
  puck dock and 18° off upright in the cradle. Level, Pi Pet tilt and
  similar apps will show that offset until they are re-zeroed.
