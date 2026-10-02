// M5Stack Stopwatch tripod mount — parametric OpenSCAD source.
//
// A thin, flat plate that sits behind the Stopwatch's back, inside its edge,
// held by the four back magnets alone, on a 3/4" tube that leaves the
// plate's back at 32° and ends in a hole for a 1/4-20 heat-set insert. Screw
// it onto a tripod, light stand or ball head: with the tube vertical, the
// face stands 58° from horizontal, looking up and toward you, lanyard down.
// The tube blooms smoothly into the plate.
//
// plate = "seat" swaps in the puck dock's seat and rim instead, from
// ../stopwatch-desk-cradle/stopwatch-puck-dock.scad (its seat_test()).
// Units are millimetres.
//
// Render:  openscad --backend=manifold --export-format binstl -o stopwatch-tripod-mount.stl stopwatch-tripod-mount.scad
// Print:   standing on the tube's end, with a brim. The plate's back then
//          stands 32° from vertical, which prints without supports.

use <../stopwatch-desk-cradle/stopwatch-puck-dock.scad>
use <../lib/m5-stopwatch.scad>

/* [Plate] */
plate = "flat";      // "flat": a thin disc held by the magnets; "seat": the puck dock's seat and rim
plate_d = 48;        // inside the back's Ø51.35 edge, where its R2 round-over starts, by about 1.7 mm
plate_t = 2.12;      // just over the 6 × 2 mm magnets, so they sit flush in through-holes (9 layers at 0.2 + 0.24)
plate_round = 1;     // round-over on the back edge; the device side stays flat and crisp
magnet_pitch = 25;   // proven by the dock's seat test
magnet_hole_d = 6.2; // press fit, proven; glue them in as well, since the holes go through
magnet_lead = 0.3;   // lead-in on the device side, where the magnets go in

/* [Tube] */
tube_d = 19.05;      // 3/4"
tube_len = 76.2;     // 3", from the plate's back to the tube's end, along the axis
mount_angle = 58;    // between the tube and the plate's face normal, leaning toward 6 o'clock:
                     // the tube is 90 - 58 = 32° to the plate, and the face 58° from horizontal
root_shift = -1.5;   // moves the tube's root toward 6 o'clock (negative: toward 12), centring its footprint on the plate
bloom_up = 14;       // the bloom starts this far up the tube from where the tube meets the plate...
bloom_out = 5;       // ...and reaches this far across the plate's back on the 6 o'clock side,
bloom_out_top = 2;   // ...and this far on the 12 o'clock side, where the corner is already open (148°)
end_chamfer = 1;     // softened edge at the tube's end

/* [1/4-20 heat-set insert] ZWMSSLL 1/4"-20x8x8 (Amazon B0DFWVD621) */
// The listing's drawing: knurl D 8.0, length L 8.0, and a smooth guide at
// the leading end, d1 7.15 (±0.1). The hole matches the guide, so the insert
// stands square on its own, and the knurl melts into the wall.
insert_hole_d = 7.2;  // d1 plus 0.05; FDM holes print a little small, so this grips the guide
insert_depth = 9.5;   // L 8.0 plus room for the plastic the knurl displaces
insert_lead = 0.4;    // lead-in at the mouth, out to the knurl's 8.0

print = true;        // stand it on the tube's end, as it prints and as it mounts; false: the plate frame

$fn = 96;
r0 = tube_d / 2;
NA = 128;            // points round the tube
NB = 24;             // steps through the bloom

// Plate frame: the back is z = 0, the seat faces +z, 12 o'clock is +y.
// The tube's axis runs from the plate's centre, down and toward 6 o'clock.
d = [0, -sin(mount_angle), -cos(mount_angle)];
e1 = [1, 0, 0];
e2 = cross(d, e1);                    // perpendicular to the axis, pointing up the acute side
function n_at(a) = cos(a) * e1 + sin(a) * e2;
// Where the tube's surface line at angle a meets the plate's back (z = 0).
function s_meet(a) = -r0 * n_at(a)[2] / d[2];
o = [0, -root_shift, 0];
function on_tube(a, s) = o + s * d + r0 * n_at(a);
function out_at(a) = let(v = [n_at(a)[0], n_at(a)[1], 0]) v / norm(v);
// The bloom: a quadratic Bezier per surface line, tangent to the tube at its
// start and to the plate's back at its end, with its corner where they meet.
function bloom_pt(a, u) = let(
  p0 = on_tube(a, s_meet(a) + bloom_up),
  p1 = on_tube(a, s_meet(a)),
  p2 = p1 + (bloom_out_top + (bloom_out - bloom_out_top) * (1 + sin(a)) / 2) * out_at(a))
  (1 - u) * (1 - u) * p0 + 2 * u * (1 - u) * p1 + u * u * p2;

// Rings from the tube's end up to the plate, then into it.
function tube_ring(i) =
  i == 0 ? [for (k = [0:NA - 1]) on_tube(360 * k / NA, tube_len) - end_chamfer * n_at(360 * k / NA)] :
  i == 1 ? [for (k = [0:NA - 1]) on_tube(360 * k / NA, tube_len - end_chamfer)] :
  i <= NB + 2 ? [for (k = [0:NA - 1]) bloom_pt(360 * k / NA, (i - 2) / NB)] :
  [for (k = [0:NA - 1]) bloom_pt(360 * k / NA, 1) + [0, 0, 0.6]];
NR = NB + 4;

module tube_solid() {
  pts = [for (i = [0:NR - 1]) each tube_ring(i)];
  side = [for (i = [0:NR - 2], k = [0:NA - 1])
    let(a = i * NA + k, b = i * NA + (k + 1) % NA) [a, b, b + NA, a + NA]];
  polyhedron(pts, concat(side,
    [[for (k = [NA - 1:-1:0]) k]],
    [[for (k = [0:NA - 1]) (NR - 1) * NA + k]]));
}

module insert_hole() {
  tip = insert_hole_d / 2;    // a 90° cone at the bottom, so its roof prints
  translate(o + (tube_len - insert_depth) * d) rotate([180 - mount_angle, 0, 0]) {
    translate([0, 0, -tip]) cylinder(h = tip + 0.01, r1 = 0, r2 = insert_hole_d / 2);
    cylinder(h = insert_depth + 0.01, d = insert_hole_d);
    translate([0, 0, insert_depth - insert_lead])
      cylinder(h = insert_lead + 0.01, d1 = insert_hole_d, d2 = insert_hole_d + 2 * insert_lead);
  }
}

// The flat plate, in the plate frame: its back at z = 0, the device side at
// z = plate_t, 12 o'clock at +y.
module flat_plate() rotate_extrude($fn = 160) hull() {
  square([0.01, plate_t]);
  translate([plate_d / 2 - plate_round, plate_round]) circle(plate_round, $fn = 32);
  translate([plate_d / 2 - 0.01, plate_round]) square([0.01, plate_t - plate_round]);
}

// The magnet holes go right through the flat plate. They're cut after the
// tube is added, so where the bloom covers one from behind, the hole stops
// at the bloom, still a full plate_t deep.
module magnet_holes() for (m = m5_magnets(magnet_pitch)) translate([m[0], m[1], 0]) {
  translate([0, 0, -0.01]) cylinder(h = plate_t + 0.02, d = magnet_hole_d);
  translate([0, 0, plate_t - magnet_lead]) cylinder(h = magnet_lead + 0.01, d1 = magnet_hole_d, d2 = magnet_hole_d + 2 * magnet_lead);
}

module mount() difference() {
  union() {
    if (plate == "flat") flat_plate();
    else seat_test(pocket = false);   // no finger pocket: the rim runs unbroken round the seat
    tube_solid();
  }
  insert_hole();
  if (plate == "flat") magnet_holes();
}

// The mount standing on the tube's end: the axis straight down, its end on the bed.
module printed() translate([0, 0, tube_len]) rotate([mount_angle, 0, 0]) translate(-o) mount();

if (print) printed();
else mount();
