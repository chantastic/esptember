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

- The user's printer is a Bambu Lab A1 mini with a 0.4 mm nozzle. Draft
  test prints use 0.24 mm layers; real prints are normally finer, so ask
  the layer height whenever a feature depends on it. At draft settings, two
  layers are 0.48 mm, and an extrusion line is about 0.4 mm wide. The A1 mini's bed is 180 × 180 mm.
- **Layer grid.** The first layer is 0.2 mm, so layer boundaries fall at
  `0.2 + 0.24·n`: 0.2, 0.44, 0.68 … 2.12, 2.36, 2.60 mm. A flat feature that
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
