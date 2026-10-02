// M5Stack Stopwatch lanyard clip — parametric OpenSCAD source.
//
// Hangs on the top edge of a lanyard badge, at its centre between the
// lanyard's two hooks, with the Stopwatch on the badge's front:
//   - a thin ring in front of the badge that wraps gently round the device's
//     edge, thickening toward 6 o'clock, where two small pegs drop into the
//     device's lanyard holes and carry its weight. USB-C stays free;
//   - a U over the badge's top edge;
//   - a backing behind the badge that holds four magnets. They pull on the
//     device's magnets through the badge, clamping it between them.
// To fit the device: hook its lanyard holes onto the pegs, then swing its
// top back into the ring. The magnets snap it to the badge.
//
// Device frame (as ../lib/m5-stopwatch.scad): X right, Z up to 12 o'clock,
// +Y out of the back. The device's back and the badge's front are at
// y = m5_t() / 2. Units are millimetres.
//
// Render:  openscad --backend=manifold --export-format binstl -o stopwatch-lanyard-clip.stl stopwatch-lanyard-clip.scad
// Print:   as exported, flat on the backing, with supports from the build
//          plate under the ring.

use <../lib/m5-stopwatch.scad>

part = "clip";       // "clip", or "peg_test": just the ring's bottom and its pegs, to check them in the lanyard holes

/* [Badge] */
card_t = 0.76;       // a CR80 card, the thickness of a credit card
slot_gap = 0.3;      // clearance through the U; the magnets pull the backing onto the badge
clip_w = 16;         // across the U, which must fit between the lanyard's hooks
bridge_t = 2.5;      // the U's thickness over the badge's edge
u_front = 2.6;       // how far the U reaches in front of the badge

/* [Ring] */
fit = 0.2;           // radial gap round the device's edge
wrap_side = 3.1;     // how far the ring reaches forward from the back, round most of the edge (its rim then ends 3.7 from the back, 0.6 short of the buttons and Grove)...
wrap_bottom = 4.3;   // ...and at 6 o'clock; plus half the ring's thickness for its rounded rim
ring_side = 1.2;     // the ring's thickness round most of the edge...
ring_bottom = 2.4;   // ...and at 6 o'clock, where it holds the pegs
bump_span = 40;      // the thicker bottom blends out over this many degrees each side of 6 o'clock
back_round = 0.6;    // round-over on the ring's back outer edge

/* [Lanyard-hole pegs] */
// The two lanyard holes, read off M5Stack's drawing (the bottom edge view):
// rectangles about 3.3 × 2.5 mm, centred 2.6 mm either side of 6 o'clock,
// 2.6–5.2 mm from the back.
peg_x = 2.6;
peg_from_back = 3.9;
peg_w = 2.9;         // across x, 0.2 smaller each side than the hole
peg_h = 1.8;         // axial, 0.35 smaller each side, so the device can swing in on them
peg_len = 2.0;       // how far each peg reaches into its hole (1.0 lined up well in the first print; deepened for stability)
peg_tip = 0.4;       // chamfer on the peg's tip

/* [Backing and magnets] */
backing_r = 22.5;    // behind the badge: just round the magnets
// Layer grid: Bambu A1 mini, 0.4 nozzle, 0.2 mm first layer, then 0.16 (Optimal,
// for real parts) or 0.24 (Draft, for tests). 2.12 is a boundary on both.
magnet_skin = 0.32;  // a skin over each magnet on the badge side, a floor for a dab of glue: two Optimal layers,
                     // a bridge and a solid layer over it; Draft prints it as one 0.24 layer
backing_t = 2.44;    // the 2.12 mm magnet pockets plus the skin: 0.2 + 14 × 0.16
magnet_pitch = 25;   // proven by the dock's seat test
magnet_hole_d = 6.2; // the proven press fit
fillet = 4;          // where the backing's circle flows into its neck
edge_r = 0.6;        // round-over on the backing's badge-side edges

N = 240;             // steps round the ring
$fn = 64;

// Derived positions.
Y0 = m5_t() / 2;                    // the device's back and the badge's front
Yb0 = Y0 + card_t + slot_gap;       // the backing's badge side
Yb1 = Yb0 + backing_t;              // the backing's back, on the bed
rb = m5_d_back() / 2;               // the body's radius where its back fillet ends
rf = m5_r();
e1 = m5_edge_r_face();
e2 = m5_edge_r_back();

// The device's radius at a distance `ax` from its back: through the back
// fillet, then along the tapered side. Plus the fit gap.
function r_in(ax) = fit + (ax < e2
  ? rb - e2 + sqrt(max(0, e2 * e2 - (e2 - ax) * (e2 - ax)))
  : rb + (rf - rb) * (ax - e2) / (m5_t() - e2 - e1));
ring_top = r_in(wrap_side) + ring_side;   // the ring's outer radius at 12 o'clock
card_top = ring_top + 0.2;                // the badge's top edge sits just above the ring
z_top = card_top + bridge_t;

// How much of the thicker bottom applies at angle a (0 = 12 o'clock, clockwise).
function bump(a) = let(d = abs(a - 180)) d < bump_span ? pow(cos(90 * d / bump_span), 2) : 0;
function wrap(a) = wrap_side + (wrap_bottom - wrap_side) * bump(a);
function thick(a) = ring_side + (ring_bottom - ring_side) * bump(a);

// The ring's cross-section at angle a, as (r, y): along the device's edge
// from the back, round the rim's bead, back along the outside, round the back
// outer edge, and along the back face (which closes the loop).
M = 14; K = 12; J = 4; Q = 6;
function profile(a) = let(w = wrap(a), t = thick(a), ro = r_in(w) + t, cr = r_in(w) + t / 2)
  concat(
    [for (i = [0:M]) let(ax = w * i / M) [r_in(ax), Y0 - ax]],
    [for (k = [1:K]) let(p = 180 - 180 * k / K) [cr + t / 2 * cos(p), Y0 - w - t / 2 * sin(p)]],
    [for (j = [1:J]) [ro, Y0 - w + (w - back_round) * j / J]],
    [for (q = [1:Q]) let(p = 90 * q / Q) [ro - back_round + back_round * cos(p), Y0 - back_round + back_round * sin(p)]]);
P = M + 1 + K + J + Q;
function pt(a, p) = [p[0] * sin(a), p[1], p[0] * cos(a)];

module ring() {
  pts = [for (i = [0:N - 1]) let(a = 360 * i / N) each [for (p = profile(a)) pt(a, p)]];
  faces = [for (i = [0:N - 1], j = [0:P - 1])
    let(a = i * P + j, b = i * P + (j + 1) % P, c = ((i + 1) % N) * P + (j + 1) % P, d = ((i + 1) % N) * P + j)
    [a, b, c, d]];
  polyhedron(pts, faces);
}

// The pegs: rounded rectangles reaching up from the ring's bottom into the
// lanyard holes, with chamfered tips.
module peg_2d(inset) offset(r = 0.5 - inset / 2) square([peg_w - 1 - inset, peg_h - 1 - inset], center = true);
module pegs() for (sx = [-1, 1]) {
  r0 = r_in(peg_from_back) - fit;      // the device's surface there
  translate([sx * peg_x, Y0 - peg_from_back, 0]) hull() {
    translate([0, 0, -r0 - fit - ring_bottom / 2]) linear_extrude(0.01) peg_2d(0);
    translate([0, 0, -r0 + peg_len - peg_tip]) linear_extrude(0.01) peg_2d(0);
    translate([0, 0, -r0 + peg_len]) linear_extrude(0.01) peg_2d(peg_tip);
  }
}

// The U over the badge's edge: a rounded block from just in front of the
// badge to the backing's back, less the slot the badge fills and the device.
module u_block() {
  rr = 1.0;
  difference() {
    hull() for (sx = [-1, 1], y = [Y0 - u_front + rr, Yb1 - rr], z = [ring_top - 3 + rr, z_top - rr])
      translate([sx * (clip_w / 2 - rr), y, z]) sphere(rr, $fn = 24);
    translate([-50, Y0, -100]) cube([100, Yb0 - Y0, 100 + card_top]);
    m5_body(fit);
  }
}

// The backing's outline in the XZ plane: a circle round the magnets, with a
// neck up into the U, blended in by `fillet`.
module backing_2d() offset(r = -fillet) offset(delta = fillet) union() {
  circle(backing_r, $fn = 160);
  translate([-(clip_w - 1) / 2, 0]) square([clip_w - 1, z_top - 1.5]);
}

// The backing, with its badge-side edges rounded by edge_r in thin slices.
// It prints flat on its back, so the slices fall on the layers anyway.
module backing() {
  n = 6;
  translate([0, Yb1, 0]) rotate([90, 0, 0]) {
    linear_extrude(backing_t - edge_r + 0.001) backing_2d();
    for (i = [0:n - 1]) let(h0 = backing_t - edge_r + edge_r * i / n, h1 = h0 + edge_r / n,
                            inset = edge_r - sqrt(edge_r * edge_r - pow(edge_r * (i + 1) / n, 2)))
      translate([0, 0, h0]) linear_extrude(h1 - h0 + 0.001) offset(delta = -inset) backing_2d();
  }
}

// Magnet pockets, open at the backing's back and closed by the skin on the
// badge side. The magnets press in from the back onto a dab of glue. In print
// the skin is the backing's top layer, bridged over each pocket.
module magnet_holes() for (m = m5_magnets(magnet_pitch)) translate([m[0], 0, m[1]]) {
  translate([0, Yb0 + magnet_skin, 0]) rotate([-90, 0, 0]) cylinder(h = backing_t - magnet_skin + 0.01, d = magnet_hole_d);
  translate([0, Yb1, 0]) rotate([-90, 0, 0])   // a 0.3 mm lead-in chamfer at the mouth
    cylinder(h = 0.6, d1 = magnet_hole_d + 0.6, d2 = magnet_hole_d - 0.6, center = true);
}

module clip() difference() {
  union() { ring(); pegs(); u_block(); backing(); }
  magnet_holes();
}

module peg_test() intersection() {
  clip();
  translate([-14, -20, -40]) cube([28, 20 + Y0, 40 - 18]);
}

// As printed: flat, the backing's back on the bed, 12 o'clock toward +Y.
// The peg test has no backing, so it stands on the ring's back face.
if (part == "peg_test") translate([0, 0, Y0]) rotate([-90, 0, 0]) peg_test();
else translate([0, 0, Yb1]) rotate([-90, 0, 0]) clip();
