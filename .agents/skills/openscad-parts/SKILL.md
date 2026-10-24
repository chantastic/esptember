---
name: openscad-parts
description: Design, verify, and reverse-engineer 3D-printable parts for ESPtember hardware with OpenSCAD — stands, docks, cases, mounts — including mesh checks, clearance tests against device keep-outs, overhang review, rendering review images, and extracting dimensions from reference STL/3MF models.
---

# OpenSCAD printable parts

Use this skill to create or change anything under `hardware/`, or to measure
a reference model someone shares. For M5Stack Stopwatch geometry, also read
[m5-stopwatch-mechanical](../m5-stopwatch-mechanical/SKILL.md).

## Toolchain

- Install with `brew install --cask openscad@snapshot`. The stable
  `openscad` cask was disabled on 2026-09-01 because it fails Gatekeeper.
  The snapshot links `/opt/homebrew/bin/openscad`.
- Always render with `--backend=manifold`. It takes well under a second on
  these parts, and CGAL is much slower.
- Export committed STLs with `--export-format binstl`. Binary STL is about a
  quarter the size of the default ASCII: the dock drops from 5.1 MB to
  0.77 MB. Slicers and `mesh_check.py` read both.
- Give render commands relative to the repository root. The user's terminal
  sits there, and a bare `openscad -o part.stl part.scad` fails from the
  root.
- The helper scripts in `scripts/` use only the Python standard library:
  - `mesh_check.py part.stl`: shells, genus, non-manifold edges,
    near-duplicate vertices, size, and overhangs by angle and region.
  - `mesh_convert.py in.3mf out.stl`: lists each 3MF object and writes an
    STL that OpenSCAD can import.
  - `section_holes.py model.stl Z…`: slices the mesh at each height, lists
    every loop, and fits circles to holes.
  - `render_views.sh in.scad|in.stl out_dir`: iso, front, side, rear and
    under PNGs, rendered in preview so ghosts show.
  - `drawing_measure.py page.png X0 Y0 X1 Y1 --mm-per-px S`: outline width
    per row in a raster drawing. Use it for tapers, steps and spans that a
    drawing shows but doesn't dimension.
- PIL and numpy aren't installed. The scripts get pixels by converting to
  BMP with `sips`, and read meshes as plain STL.

## Design loop

This is the order that produced a first-time perfect fit on the M5Stack
Stopwatch dock:

1. **Gather the evidence first.** That means the manufacturer's dimensioned
   drawing and photos, any reference part the user has, and the user's own
   observations of it, such as "it only goes on back-first". An
   illustration or an earlier design is not evidence.
2. **Put the geometry in the shared library.** Record each value's source
   and confidence, and model the real body: tapers, fillets, nubs. Add
   keep-outs for everything that must stay reachable.
3. **Design the part** against the library, subtracting the keep-outs.
4. **Run the CAD checks below.** Show renders.
5. **Print the interface first.** Ship a test-print mode that cuts out just
   the part that touches the device, laid flat: 10 minutes instead of 2
   hours.
6. **Change one variable at a time.** Usually that's `fit`, in 0.05–0.1 mm
   steps. If the fit feels wrong, compare the part with the reference before
   touching `fit`. Here the cause was the shape, not the clearance.
7. **Record the physical result** in the part's README and the device
   skill's evidence log. Give the date, the settings, what the print proved,
   and what it didn't. Only then print the full part.

## Thin features: think in layers and lines

- The user's printer is a Bambu Lab A1 mini with a 0.4 mm nozzle. They
  print tests on the **Draft** profile (0.24 mm layers) and everything else
  on **Optimal** (0.16 mm layers). Both start with a 0.2 mm first layer. An
  extrusion line is about 0.4 mm wide, and the bed is 180 × 180 mm.
- **Design for both grids.** Draft boundaries fall at `0.2 + 0.24·n` and
  Optimal at `0.2 + 0.16·n`. They coincide every 0.48 mm: 0.2, 0.68, 1.16,
  1.64, 2.12, 2.60 mm. Put depths that matter, such as a magnet pocket's
  floor, on a shared boundary. A thin skin sized in Optimal layers still
  prints on Draft, because the slicer samples each layer at its middle. The
  clip's 0.32 mm magnet skin (2.12–2.44) is two Optimal layers, or one Draft
  layer sampled at 2.24. The user is happy with two layers wherever "about
  one layer" is asked for, and two is sturdier for a bridged skin.
- **Layer grid (Draft).** The first layer is 0.2 mm, so Draft layer
  boundaries fall at `0.2 + 0.24·n`: 0.2, 0.44, 0.68 … 2.12, 2.36, 2.60 mm. A flat feature that
  must be an exact number of layers needs its top and bottom on that grid.
  Otherwise the slicer smears it across an extra layer. A 0.4 mm skin from
  2.1 to 2.5 mm showed three layer lines; 2.12–2.60 mm would give exactly
  two. Total height rounds up to the next boundary: the 9.7 mm seat test
  slices into 41 layers (9.8 mm). The seat plate is now 2.84 mm and the seat
  test 10.04 mm, both on the grid.
- **Layer lines are a ruler.** On a known layer grid, the user can measure
  by counting visible layer lines. Three lines of wall below a magnet
  pressed flush into a 2.6 mm plate gave its thickness: 2.6 − (0.2 + 2 ×
  0.24) = 1.92 mm, a 6 × 2 magnet.
- A skin, a membrane or a floor is only "N layers" when it's parallel to the
  bed. On a surface tilted by θ, the slicer builds the same thickness as a
  staircase of layers with a run of `layer_h / tan θ`, so a 0.4 mm skin at
  30° can open pinholes. On a near-vertical surface, thickness is counted in
  extrusion lines (about 0.4 mm each). Size tilted skins at 0.6–0.8 mm, or
  print that feature flat.
- To embed parts such as magnets or nuts, pausing at a layer only works if
  the cavity's axis is vertical in the print. Otherwise, load them from
  behind through a slightly larger channel (+0.4 mm) that ends in the
  proven press-fit pocket, and check with renders where the channel exits.

## Reading drawings and reference parts

- Check the sheet's orientation against a photo before reading positions.
  M5Stack's C152 sheet is rotated 180°.
- Work out the projection. In first-angle (common on Chinese and European
  sheets), a side view's edge nearest the front view is the object's back.
  In third-angle (US) it's the front. Confirm it with a feature you can see
  in a photo or a reference part.
- Measure undimensioned features at full resolution, and calibrate the
  scale against a dimensioned feature in the same view.
- A reference part that fits only one way round reveals a taper or a step.
  Model it.

## Source conventions

- Keep one parametric `.scad` file per part. Put named, commented
  parameters at the top, grouped with `/* [Group] */` for the Customizer.
  Every length is in millimetres, and each comment says why the value is
  what it is.
- Keep device geometry in a shared library under `hardware/lib/` and load
  it with `use <../lib/…>`. `use` imports modules and functions but not
  variables, so the library exposes values as functions (`m5_r()`). Top-level
  `$fn` in a `use`d file doesn't apply either, so set it in the caller.
- Model in a device frame, then `pose()` into the world:
  `translate([0,0,cz]) rotate([-tilt,0,0])`. Write `world(p)` as the same
  maths, so lofts and placement can compute world points. Derive `cz` from
  the lowest point that must clear the desk.
- `slab_y(y0, y1) children()` extrudes a 2D shape drawn in the device's XZ
  plane along Y. It keeps sectors, seats and pockets readable.
- Build sweeping forms such as skirts, flares and waists as a `polyhedron`
  loft between rings with matching point counts. Ease the horizontal
  position with `pow(s, flare)` and keep height linear. Face winding: bottom
  `[0..N-1]`, top reversed, sides `[k*N+i, (k+1)*N+i, (k+1)*N+j, k*N+j]`.
- `hull() { part; linear_extrude(base_t) projection() part; }` drops a part
  straight to the bed. Use it for pillars and for slots that must reach the
  desk. The convex hull can cut into the device, so always subtract the
  device envelope afterwards.
- Ship a quick test-print mode, such as `seat_only`, that cuts out just the
  interface and lays it flat. A 10-minute print catches fit errors before a
  2-hour one.
- Include translucent `%` previews of the device and its keep-outs, so
  review renders show the relationship.

## Organic forms

The user prefers soft, organic parts: no hard edges, curves instead of
walls, and channels instead of holes. CSG unions of slabs and cylinders
always leave creases, so build the main body as a single lofted surface
instead:

- **One loft, one profile per angle.** Write `profile(a)` to return the
  whole cross-section at angle `a`, as world points from the foot's bottom
  edge round to the seat floor. Build it from N angles, and cap the two
  ends. Every profile must have the same number of points, so vary lengths
  and heights, never the point count. The stopwatch puck dock is the worked
  example.
- **Vary features along the angle with cosine blends**
  (`(1 - cos(180x)) / 2`) instead of cutting them. The dock's rim height
  runs 1.2 → 7.2 → 3.7 mm around the device, so it needs no windows. Blend
  over at least about 35° of arc. Shorter blends read as a fold running down
  the skirt.
- **Tangent continuity.** Join sections with a cubic Bézier whose handles
  follow the neighbouring surfaces: along the rim's lean at the top, and
  vertical at the foot. A shoulder crease comes from mismatched tangents.
- **Round over thin edges.** End a rim with a semicircle of radius
  `rim_t / 2` between its inner and outer faces. About 1 mm (2–3 lines) is
  printable, and it reads as the part rounding right into the device.
- **Cut grooves with a swept scoop, not by pushing the loft in.** Pushing
  loft points inward (a displacement "funnel") works for broad, gentle
  dips. For a narrow, steep groove it stair-steps: the grid can't follow a
  wall that changes position with height. Instead, sweep an elliptical
  tube along the ball's meridian as one polyhedron loft (`scoop()`). Bury
  it partway, so its cut depth is `d` and its semi-axis along the normal is
  `scoop_soft × d` (2.6 on the dock). Ease its width and depth along its
  length. The groove's edges then meet the surface on a soft, clean curve.
  Check each scoop cutter alone with `mesh_check.py`.
- **Don't turn sketch shading into geometry.** The user's hatching around
  the channel was shading, not roots, funnels or flares. They asked for
  "smooth on both front and back" and "we don't need all those extra
  grooves". The final dock has one front groove, a plain arch at the back,
  and a gently widening channel. Before adding a feature from a sketch,
  ask whether the marks are form or shading.
- **Lead a groove to where the cable really comes from.** A funnel that
  stops partway up reads as a face (the user saw Mike Wazowski). At the
  dock's front, the rim dips at 6 o'clock under the USB-C port (`h_dip`,
  eased over ±0.6 clock hours). One scoop then sweeps from that dip down
  the ball's front meridian, below the table, and into the channel.
- **A ball cut by the seat and the table.** The user wanted the body "more
  ball shaped, as if it were a ball with a hard cut to make it sit on the
  table". Put the ball's centre C on the device's axis, behind the seat
  (`ball_e`), with a radius that passes through the rim. A sphere through
  a circle has its centre on the circle's axis, so the rim meets the ball
  exactly. Generate each profile along the ball's meridian in the plane of
  the device's axis and that profile's radial direction, so the rim's
  round-over and the ball share a plane. `meridian_at_z()` solves for where
  the meridian meets the table. Let the rim's height variation depart from
  the ball over only the top `ridge` share of the profile, so cheeks read
  as ridges on the ball. Keep the ball's centre height under about
  0.45 × its radius, so the table cut stays under about 27° of overhang.
- **Read sketches for outline and form.** The user's bottom view showed
  the channel's outline, which is worth matching. Their hatching was
  shading (see above).
- **Loft cutters instead of chaining hulls.** A chain of `hull()` segments
  shares identical stations, which leaves near-duplicate vertices and
  non-manifold edges. Build a variable cross-section cutter, such as the
  hourglass channel, as one polyhedron loft of sections. Then run
  `mesh_check.py` on the cutter alone: inconsistently wound edges must be
  0, as well as the volume being positive.
- **A deliberate hard cut can contrast a soft form.** The user asked for
  the front "cut off with a knife or laser" at 75–80°, and meant it
  leaning forward: an undercut, which they wrote as −78. Ask which way an
  angle leans. The cut is one flat half-space,
  `translate(q) rotate([tilt, 0, 0])` of a large cube, with
  `tilt = angle < 0 ? 90 + angle : angle − 90`. A 12° undercut prints fine. Where the ball already curves inward below the rim, a plane
  that only grazes the rim cuts nothing. Anchor the plane where it meets
  the table (`front_cut_depth` into the foot), and check what it removes:
  on the dock it takes the lower lip from 4:30 to 7:30, which changes
  retention. Keep alternatives behind a style switch (`front_style`).
- **Fillet an opening in a tilted cut face the same way.** Compute the
  face's position at height z (`front_cut_y(z)`), and key the fillet
  stations to where the fillet's top meets it. On a 12° undercut, the short
  walls near the desk leave the face before the fillet starts, so they stay
  crisp. The arch, which is what shows, is rounded.
- **Fillet an exit on an overhanging surface at the fillet's top.** Where
  a channel leaves an overhanging ball, place the quarter-circle fillet
  stations so the fillet's highest point meets the surface. Grow the
  cutter only slightly past it (0.3 mm). A large growth beyond the surface
  cuts a hard crescent into the overhang.
- **Level floors on a tilted part.** When a pocket sits in a tilted face,
  the user wants its full depth "on the same layer". Loft the pocket from
  sections in level world planes along its centreline. Give each section
  a flat floor band at one world height, snapped to the layer grid
  (`0.2 + 0.24·n`), walls rising tangentially (`u^2.2`), and a rounded lip.
  Verify the result by summing the area of horizontal facets at that height:
  one height, a large area. A superellipse bottom looks flat but isn't, so
  it spreads over several layers.
- **One curve, not wall + lip.** A wall curve followed by a separate
  rounded lip reads as a "double lip". Make each side one S-curve,
  `smooth(u) = (1 − cos(180u)) / 2`, from tangent-to-floor to
  tangent-to-surface. Stop it at about u = 0.95, just below tangency, then
  turn up into air, so nothing lies on the surface and makes slivers. The
  gentler walls leave less clearance near the edges, so re-run the keep-out
  test. Here it flagged a rough early speaker box, which the drawing then
  replaced with measured values.
- **Not everything wants the same curve.** The one-curve treatment that
  worked for the finger pocket was tried on the rim too: a Bézier crest in
  place of the round-over bead, rising with the step at the bottom. The
  user called it a step backward and asked for the bead back. Keep the
  small round-over bead on the rim, and try a styling change on one feature
  at a time.
- **Bloom a tube into a plate with one Bézier per surface line.** For a
  tube leaving a plate at an angle (the tripod mount), take each line on
  the tube's surface. Where it meets the plate's plane is the corner Q.
  Run a quadratic Bézier from a point `bloom_up` up the tube, with Q as its
  control point, to a point `bloom_out` across the plate along that side's
  outward direction. Loft those curves into the tube's rings. Each curve is
  tangent to the tube at one end and to the plate at the other. The acute
  side's corner (45° of air) gets most of the fillet, and the obtuse side
  (135°) needs only a few millimetres, so blend `bloom_out` by angle. Keep
  the bloom's footprint clear of anything cut through the plate, and check
  with `difference(intersection(tube, slab above the back), plate)`, which
  must be empty. Extend the last ring 0.6 mm into the plate for the union.
- **Breakaway fins for tall, narrow parts. Unproven: the first one failed
  badly (2026-10-01). The cause wasn't recorded. Before relying on one,
  ask how it failed and print a test.** On a bed-slinger (A1 mini:
  the bed moves in Y), a tall part on a small footprint wobbles long before
  its overhangs need support. Model a fin into the export as a separate
  shell: a wall two lines thick (0.84 mm), in a plane along Y, standing on
  the bed with a long foot and stopping one layer (about 0.2 mm) short of
  the part, under its farthest-out low point. The slicer's lines just fuse
  across that gap, so the fin holds while printing and snaps off by hand.
  Build it from the part's own 2D section in that plane:
  `minkowski(section, triangle)` casts the part's shadow down, and out at
  the fin's lean. Intersect that with the fin's span, then subtract
  `offset(delta = gap)` of the section. Don't `hull()` the shadow to a foot
  point: the hull bridges notches above the part and leaves stray slivers.
  Check that the fin's 2D outline is one piece.
- **Wine-glass stands: one profile per line, a level waist.** For the desk
  stand, each line round the tilted top's rim is one profile: a lip, a
  cubic from the foot (tangent to the desk) to the waist (vertical), and a
  cubic bloom from the waist to the rim. The bloom ends parallel to the
  top's normal, then runs through a short rim band and a round-over onto
  the top. Keep the waist a level circle at one height, so the foot stays
  round while the bloom absorbs the tilt. When a tilted top would overhang
  too far if printed standing (a thin plate at 26° is 64°), let the bloom
  carry the whole top. Size its handles so the 6 o'clock side, where the
  bloom is shortest, stays within 45°.
- **Height constrains the form, so check proportions before styling.** The
  user asked for the device's front edge 1/2" off the desk. That leaves no
  room for a foot, a waist and a bloom, which all need height. The foot
  shrank to a thin coaster, and the user preferred a thick stem: a base
  chamfer straight into the bloom, like the Harness. Shorter cubic handles
  (pulls of 0.2–0.35) keep a short bloom within 45°. Sweep them with `-D`
  and read `mesh_check.py`'s 45° list. Work out a device's lowest point from
  the posed `m5_body_true()` (here 9.9 mm below the top's centre) instead of
  guessing.
- **Design round a cable's path, not a plug box.** When the user names a
  specific cable, model its path: plug centre, head, a stiff run, then a
  bend of known inner radius onto the desk. Derive the part's height from
  that path, as a function, so correcting one measured value moves
  everything. Cut the channel as a loft of the ribbon's section plus
  clearance, with a quarter-round flare at each mouth. Place the allowance
  for a stiff joint (the collar between flat and braided) by path length
  from the head, with some slack, and blend the channel into it. Model the
  cable too, and require `intersection(part, cable)` to be empty. A
  listing's photos give shapes but rarely numbers: mark every value read
  from them "est." and ask for measurements. Ask which way a right-angle
  plug's lead runs before routing it.
- **Check a new loft's orientation before booleans.** If `mesh_check.py`
  reports a negative volume, or a pocket turns up as its own shell, the
  winding or a frame rotation is reversed. `rotate([t, 0, 0])` takes +y to
  (0, cos t, sin t). Check which way a tilt goes with a single point before
  trusting a render.
- **Don't bolt a boxy feature onto an organic form.** A cable-holding nose
  on the desk stand, after a notch in a reference video, was built from
  hulled rectangular sections: two pillars and a window. The user rejected
  it on sight, and it was removed. If a feature returns, grow it from the
  form's own profile (the bloom's lines) instead of adding a separate solid,
  and show a render before building the clearance tests round it.
- **Simplify after a physical print.** Once the user has held a print, act
  on what they liked and drop the rest. On the dock that meant one uniform
  rim instead of cheeks, dips and cuts; the ball's natural front; and seat
  features that match the device (a finger pocket and a registration step)
  instead of generic cut-outs. The simpler loft (constant rim height, ball
  through the rim) removed most of the special-case code.
- **Nudge away slivers.** If `mesh_check.py` reports a few near-duplicate
  vertices where a cutter crosses the loft, move the cutter by about
  0.1 mm. The crossing lands off the loft's grid and the sliver disappears.
- **Soft cutters.** Where something must be cut, use a hull of spheres:
  large radii, with centres set back from the surface so the cut meets it
  at a shallow angle. Give tunnels and grooves a small teardrop peak, so
  the roof prints at 45°. Give lips that meet the bed a 45° flare
  (a hull of a disc at z = 0 and a sphere).
- **Check the loft three ways:** `mesh_check.py`'s signed volume is
  positive; `openscad --preview=throwntogether` shows no purple
  (inside-out) faces; and a clearance test is run against every keep-out.
  OpenSCAD's direct render (F6) of a complex Manifold result can drop
  faces and show background through a solid part. On this dock it showed
  phantom windows in the skirt and white patches inside the rim. Before
  calling anything a hole, slice the part (`projection(cut = true)`) and
  render the exported STL through `import()`. Make review and README images
  from the exported STL.

## Robust geometry

These are failures seen in this repository, with their fixes:

- **Coincident walls.** Two solids that share a cylinder, or a flat face,
  union into non-manifold edges or 10⁻⁴ mm near-duplicate vertices. That
  happens even with identical `$fn`, because each passes through its own
  transform. Build them as one solid and cut away what isn't wanted. For
  example, make the seat plate plus lip one slab, then remove the open half.
- **Mismatched facet counts** at the same radius, such as a 96-segment
  sector against a 160-segment circle, leave slivers. Intersect with the
  same `circle()` instead of drawing a new arc.
- **A frustum stacked on a cylinder** of equal radius leaves a seam with
  edges shared by four triangles. `hull()` the pair.
- **A loft meeting a plate** exactly at its rim can split into several
  shells. End the loft a little inside the plate's outline (a 0.4 mm
  `shoulder` step) and continue its top ring 1 mm into the plate, so the
  surfaces cross cleanly.
- OpenSCAD can print `Status: NoError` while the exported STL still fails
  `mesh_check.py`. Negative genus means several shells or bad edges.
  Always run the script.

## Verification: CAD evidence

Record these for each part in its README, with the date and OpenSCAD version:

1. The render with `--backend=manifold` has no warnings.
2. `mesh_check.py` shows one shell, 0 non-manifold edges and 0
   near-duplicate vertices. The genus equals the intended through-holes: a
   tunnel is 1, an open groove is 0.
3. **Clearance.** `intersection() { part(); pose() { device(); keepouts(); } }`
   renders "Current top level object is empty." Write it as a small wrapper
   file that `use`s the part and the library. Keep-outs are boxes for finger
   room, plug room and air gaps, not just the bare body. The clearest
   guarantee is to subtract the keep-outs from the part itself. Subtract
   them grown by 0.1 mm (`grow = 0.1`): subtracting the exact test volume
   leaves zero-thickness slivers that fail the test.
4. **Model the real body.** Cut pockets with the device's true shape,
   including any taper and edge fillets, rather than an idealised cylinder.
   Compare the pocket with any proven reference seat before choosing `fit`,
   and record which way round a reference part fits.
5. **Overhangs.** Check `mesh_check.py`'s region list. Anything past about
   50° needs a reason: a small area, a bridge under 15 mm, or a 45° peaked
   roof on a tunnel.
6. **Review renders.** Look at them before claiming a shape. Cameras that
   are too close hide problems, so use `--viewall --autocenter` when
   framing. Orange faces in preview are cut surfaces; speckling on a flat
   cut face is preview shading, not a defect.

CAD evidence proves nothing physical. Keep a separate physical-fit
checklist, and never claim fit, retention, button feel or stability until a
printed part has met the real device.

## Reverse-engineering a reference model

When someone shares a model, such as a case, harness or holster:

1. Convert 3MF with `mesh_convert.py` and get the size and bbox with
   `mesh_check.py`. The Shapr3D STL header says "Made with Shapr3D".
2. Render views and read the thumbnail (`Metadata/thumbnail.png` in a 3MF)
   to learn the layout.
3. Find the seat and floor heights from large flat faces, using a z
   histogram of horizontal facets.
4. Slice below the seat with `section_holes.py` to get the hole pattern and
   diameters.
5. Map the walls. Sample slices at several heights and test points just
   outside the seat radius at each clock position. Remember that rays
   starting inside a solid tail or boss count differently from rays through
   a thin wall. A recess (a sliding channel) isn't a window: check both
   surfaces.
6. Name each proven dimension's source in the part. Don't copy the
   reference model into the repository unless its licence allows it.

## Handing work back

- Say what was verified (CAD) and what wasn't (physical).
- Show renders from where the user sits and from the side.
- Commit only when asked. Commit the `.scad` source, the shared library,
  review PNGs, and binary STLs of the defaults and the test print. Leave out
  experimental variants and `.DS_Store`.
- Put render and test-print commands in their own `bash` blocks, with paths
  relative to the repository root.
