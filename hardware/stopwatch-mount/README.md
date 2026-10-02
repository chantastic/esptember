# Stopwatch tripod mount

A thin, flat plate that sits behind the Stopwatch, inside the edge of its
back, on a 3" stem of 3/4" (19.05 mm) tube. The stem leaves the plate's
back at 32° and ends in a hole for a 1/4-20 heat-set insert. The device is
held by its four back magnets alone. Screw the mount onto a tripod, light
stand, ball head or magic arm: with the stem vertical, the Stopwatch's face
stands 58° from horizontal, looking toward you and a little up, lanyard
down. The stem blooms into the plate with a fillet that's tangent to both.

![Side view, as it stands and prints](preview.png)

| Device side | Back (plate frame) |
| --- | --- |
| ![The device side](seat.png) | ![The back and the stem](rear.png) |

## Files

- `stopwatch-tripod-mount.scad` is the parametric source.
- `stopwatch-tripod-mount.stl` is exported standing on the stem's end,
  ready to slice.

Render, from the repo root:

```bash
openscad --backend=manifold --export-format binstl -o hardware/stopwatch-mount/stopwatch-tripod-mount.stl hardware/stopwatch-mount/stopwatch-tripod-mount.scad
```

Add `-D print=false` to export it in the plate frame instead (the back at
z = 0, the device side facing +z). Add `-D 'plate="seat"'` for the earlier
plate with the puck dock's seat and rim
([`../stopwatch-desk-cradle/stopwatch-puck-dock.scad`](../stopwatch-desk-cradle/stopwatch-puck-dock.scad)'s
`seat_test(pocket = false)`). Its bloom was tuned with `root_shift` 0 and
`bloom_out` 8 / 3.

## Parameters

| Parameter | Value | Notes |
| --- | --- | --- |
| `plate` | "flat" | or "seat" |
| `plate_d` | 48 | inside the back's Ø51.35 edge, where its R2 round-over starts, by about 1.7 mm |
| `plate_t` | 2.12 | just over the 6 × 2 mm magnets (about 1.92 measured), so they sit flush; 9 layers at 0.2 + 0.24 |
| `plate_round` | 1 | round-over on the back edge; the device side is flat to its edge |
| `magnet_pitch`, `magnet_hole_d` | 25, 6.2 | the proven press fit, now as through-holes: glue the magnets in too |
| `magnet_lead` | 0.3 | lead-in on the device side, where the magnets go in |
| `tube_d` | 19.05 | 3/4" |
| `tube_len` | 76.2 | 3", along the axis from the plate's back to the stem's end |
| `mount_angle` | 58 | between the stem and the face normal, leaning toward 6 o'clock; the stem is 32° to the plate |
| `root_shift` | -1.5 | moves the root toward 12 o'clock, centring the stem's footprint on the plate |
| `bloom_up`, `bloom_out`, `bloom_out_top` | 14, 5, 2 | how far the fillet runs up the stem and across the plate (6 o'clock side, 12 o'clock side) |
| `insert_hole_d` | 7.2 | matches the insert's smooth guide (d1 7.15) |
| `insert_depth` | 9.5 | the insert's 8.0 length plus room for displaced plastic, ending in a 90° cone so the roof prints |
| `insert_lead` | 0.4 | lead-in at the mouth, out to the knurl's Ø8.0 |

### The plate and magnets

At 32°, the stem meets the plate in a long oval. With its bloom, the
footprint on the back is 26.2 × 43 mm, reaching 21.5 mm from the centre, so
it stays 2.5 mm inside the plate's edge. `root_shift` centres it, and the
bloom is shorter than on the seat plate (5 mm and 2 mm instead of 8 and 3)
so it fits. The magnet holes are cut after the stem is added. The footprint
passes within 1.3 mm of the magnets' centres, so it partly backs the holes
behind them, giving those magnets a ledge. All four holes still go through.

The device side is one flat face. Nothing wraps round the device, so it
sits on the plate by its back's main face, and the speaker and logo recesses
stay open to the air at the edge.

### The insert

The hole fits the inserts bought for this mount: [ZWMSSLL 1/4"-20x8x8 brass
heat-set inserts](https://www.amazon.com/dp/B0DFWVD621). The listing's
package drawing gives the knurl's outside diameter as D 8.0 and the length
as L 8.0. Its size table gives d1 7.15 for the smooth guide at the leading
end, with a ±0.1 mm tolerance on metric dimensions. The listing gives no
recommended hole, so the hole matches the guide: the guide drops in and
holds the insert square, and only the knurl melts into the wall. FDM holes
print slightly undersize, so if the guide won't start, open
`insert_hole_d` to 7.3.

That leaves (19.05 − 8.0) / 2 = 5.5 mm of wall around the knurl. The hole
runs 1.5 mm deeper than the insert, so the displaced plastic has somewhere
to go and the insert can sit flush with the tube's end.

## Printing

Print it as exported, standing on the stem's end, with the slicer's supports
off and a brim. The insert's axis is then vertical and its hole opens at the
bed. The plate's back stands 32° from vertical and the bloom curves into it.
About 100 mm² is steeper than 50°, mostly 50–60°, on the 1 mm round-over at
the plate's low edge. A few mm² of its underside approach flat. It's a
small, rounded edge, so it should print acceptably. The part is 96 mm tall on
a 19 mm circle, but the thin plate keeps the weight up top low. Use at least
4 walls around the hole.

Press the insert in from the stem's end with the iron, keeping the iron
square to the stem.

### The fin (removed)

The 1.5" version shipped with a breakaway fin. It was a 0.84 mm wall from
the bed up to the rim's underside, stopping 0.2 mm short, to brace the part
against the A1 mini's Y motion. It failed badly in the first print, and it
was removed rather than tuned.

## CAD evidence (2026-10-01)

- `mesh_check.py`: one shell, genus 4 (the four magnet through-holes),
  13,284 triangles. No non-manifold edges, inconsistent windings or
  near-duplicate vertices.
- Standing up: 48.0 × 26.9 × 96.4 mm. The plate's back crosses the axis
  76.2 mm above the stem's end, and the plate's lowest edge is 55.0 mm up.
  In the plate frame the plate is 2.1 mm thick.
- The stem above the plate's back, minus the plate, renders empty: the stem
  stays inside the plate.
- Measured from the exported mesh (the 1.5" version, with the same hole),
  the hole is Ø8.0 at the bed, narrowing over 0.4 mm to Ø7.2, which runs to
  9.5 mm deep.
- With `pocket` at its default (true), the dock's
  `stopwatch-puck-dock.stl` and `seat-test.stl` re-export byte-identical.

## Physical evidence

- 2026-10-01: the 1.5" seat-plate version with the fin. The fin failed badly
  in printing (see above). The design then moved to this flat plate.
