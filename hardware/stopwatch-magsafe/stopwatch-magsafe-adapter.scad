// M5Stack Stopwatch MagSafe adapter — parametric OpenSCAD source.
//
// A thin disc between the Stopwatch and the back of a MagSafe iPhone.
//   - Device side: four 6 × 2 mm magnets on the Stopwatch's 25 mm square,
//     facing its own back magnets (the proven pitch and press fit), and a
//     short lip at 6 o'clock with two pegs that reach into the device's
//     lanyard holes, the grip from the lanyard clip. Hook the device's bottom
//     onto the pegs, then swing its top down onto the magnets.
//   - Phone side: something for the phone's MagSafe ring to grab, at its
//     radius (about 23–27.5 mm), outside the device magnets (out to 20.8):
//       grip = "steel_ring": a stick-on 0.55 mm steel MagSafe ring, 55/45 mm,
//         in a shallow recess. Steel is pulled by any magnet pattern, so
//         MagSafe's undocumented polarity doesn't matter.
//       grip = "magnets": a ring of 6 × 2 mm magnets, all the same face out.
//         Test first: slide one magnet round the phone's MagSafe ring. If the
//         same face attracts all the way round, this works.
// The two magnet sets sit at different radii, so the disc is only as thick
// as a magnet plus a skin.
//
// Frame: XY is the device's face plane, 12 o'clock at +Y. The phone side is
// z = 0, which prints on the bed, and the device side is z = disc_t, with the
// device above it. Units are millimetres.
//
// Render:  openscad --backend=manifold --export-format binstl -o stopwatch-magsafe-adapter.stl stopwatch-magsafe-adapter.scad
// Print:   as exported, phone side down, no supports.

use <../lib/m5-stopwatch.scad>

grip = "steel_ring";    // "steel_ring" or "magnets"

/* [Layers] Bambu A1 mini, 0.4 nozzle: 0.2 first layer, then 0.16 (Optimal) or 0.24 (Draft) */
disc_t = 2.44;          // 0.2 + 14 × 0.16
pocket_floor = 0.36;    // the skin under each device magnet: the first layer plus one Optimal layer
                        // pockets are then 2.08 deep, for the 6 × 2 magnets (about 1.92 measured)

/* [Device magnets] */
magnet_pitch = 25;      // proven by the dock's seat test
magnet_hole_d = 6.2;    // the proven press fit
magnet_lead = 0.3;      // chamfer at each pocket's mouth

/* [Phone grip: steel ring] e.g. KraftGeek universal metal ring sticker: 55 OD, 45 ID, 0.55 thick */
ring_od = 56;           // KraftGeek lists 55 × 45; the first print's recess (from the listing) was 1 mm too small
ring_id = 44;           // across both edges, so these are the listed sizes plus and minus 1 mm
ring_t = 0.55;
ring_clear = 0.3;       // radial clearance each side of the ring
ring_recess = 0.68;     // a layer boundary on both Draft and Optimal; the ring sits 0.13 below flush

/* [Phone grip: magnets] */
phone_r = 25.3;         // the middle of MagSafe's magnet ring (about 46 ID to 54–56 OD)
phone_n = 16;           // around the ring, 9.9 mm apart
phone_depth = 2.08;     // pockets from the phone side, under a 0.36 skin

/* [Lanyard-hole lip] the clip's ring bottom, as a lip */
fit = 0.2;              // radial gap round the device's edge
lip_span = 60;          // the lip fades out this many degrees each side of 6 o'clock
lip_wrap = 4.3;         // how far its inner face reaches up the device's edge at 6 o'clock...
lip_bead = 0.8;         // ...plus its rounded rim, so it ends 5.1 mm from the back, short of the USB-C boss (5.7)
peg_root = 0.6;         // how far the pegs reach back into the lip, past its inner face; more would show through its curved outside
// The two lanyard holes, read off M5Stack's drawing and proven by the
// lanyard clip's peg test: rectangles about 3.3 × 2.5 mm, centred 2.6 mm
// either side of 6 o'clock, 2.6–5.2 mm from the back.
peg_x = 2.6;
peg_from_back = 3.9;
peg_w = 2.9;
peg_h = 1.8;            // 0.35 smaller each side, so the device can swing in on them
peg_len = 2.0;          // how far each peg reaches into its hole
peg_tip = 0.4;

/* [Disc] */
rim = 1.2;              // wall outside the grip
top_round = 0.8;        // round-over on the device side's edge
foot_chamfer = 0.3;     // on the bed side's edge, against elephant's foot

$fn = 96;

disc_r = (grip == "steel_ring" ? ring_od / 2 + ring_clear : phone_r + magnet_hole_d / 2) + rim;

module disc() rotate_extrude($fn = 240) polygon(concat(
  [[0, 0], [disc_r - foot_chamfer, 0], [disc_r, foot_chamfer], [disc_r, disc_t - top_round]],
  [for (a = [10:10:90]) [disc_r - top_round + top_round * cos(a), disc_t - top_round + top_round * sin(a)]],
  [[0, disc_t]]));

// A pocket for a 6 × 2 magnet, opening at z0 and running `depth` up
// (dir = 1) or down (dir = -1), with a lead-in chamfer at its mouth.
module pocket(z0, dir, depth) {
  translate([0, 0, dir > 0 ? z0 - 0.01 : z0 - depth]) cylinder(h = depth + 0.01, d = magnet_hole_d);
  translate([0, 0, z0]) mirror([0, 0, dir > 0 ? 0 : 1])
    translate([0, 0, -0.01]) cylinder(h = magnet_lead + 0.01, d1 = magnet_hole_d + 2 * magnet_lead + 0.02, d2 = magnet_hole_d);
}

// The device magnets: pockets open on the device side (up), over a skin on
// the phone side.
module device_pockets() for (m = m5_magnets(magnet_pitch)) translate([m[0], m[1], 0])
  pocket(disc_t, -1, disc_t - pocket_floor);

// The phone grip, from the bed side.
module grip_cut() {
  if (grip == "steel_ring")
    translate([0, 0, -0.01]) difference() {
      cylinder(h = ring_recess + 0.01, r = ring_od / 2 + ring_clear, $fn = 240);
      translate([0, 0, -0.01]) cylinder(h = ring_recess + 0.03, r = ring_id / 2 - ring_clear, $fn = 240);
    }
  else
    for (i = [0:phone_n - 1]) rotate(360 * i / phone_n) translate([0, phone_r, 0]) pocket(0, 1, phone_depth);
}

// The device's radius at a distance `ax` from its back: through the back
// fillet, then along the tapered side. Plus the fit gap.
rb = m5_d_back() / 2;
rf = m5_r();
e1 = m5_edge_r_face();
e2 = m5_edge_r_back();
function r_in(ax) = fit + (ax < e2
  ? rb - e2 + sqrt(max(0, e2 * e2 - (e2 - ax) * (e2 - ax)))
  : rb + (rf - rb) * (ax - e2) / (m5_t() - e2 - e1));

// The lip: a loft of cross-sections, as in the lanyard clip's ring, from
// 6 o'clock - lip_span to 6 o'clock + lip_span. Each section follows the
// device's edge at `fit` from just inside the disc up to a rounded rim. Its
// outside then sweeps down and out in an S-curve (a cubic Bezier, vertical
// at both ends) to the disc's own edge, just below its round-over, so the
// plate's rim rises into the lip. Its height fades to nothing toward the
// ends; there the S-curve dips inside the disc, and the lip merges into it.
function lip_f(a) = pow(cos(90 * (a - 180) / lip_span), 2);
function lip_w(a) = 0.05 + (lip_wrap - 0.05) * lip_f(a);
function lip_b(a) = 0.1 + (lip_bead - 0.1) * lip_f(a);
function bez(p0, p1, p2, p3, u) = pow(1 - u, 3) * p0 + 3 * pow(1 - u, 2) * u * p1 + 3 * (1 - u) * u * u * p2 + pow(u, 3) * p3;
LM = 12; LK = 10; LS = 16;
function lip_profile(a) = let(w = lip_w(a), b = lip_b(a), cr = r_in(w) + b,
    top = [r_in(w) + 2 * b, disc_t + w], foot = [disc_r - 0.02, disc_t - top_round],
    k = (top[1] - foot[1]) / 2)
  concat(
    [[r_in(0), disc_t - 0.5]],
    [for (i = [0:LM]) let(ax = 0.05 + (w - 0.05) * i / LM) [r_in(ax), disc_t + ax]],   // starting just above the disc, so no row lies in its face
    [for (j = [1:LK - 1]) let(p = 180 - 180 * j / LK) [cr + b * cos(p), disc_t + w + b * sin(p)]],
    [for (i = [0:LS]) bez(top, top - [0, k], foot + [0, k], foot, i / LS)],
    [[disc_r - 0.3, disc_t - top_round - 0.2]]);
LP = 1 + (LM + 1) + (LK - 1) + (LS + 1) + 1;
LN = 120;
function lip_pt(a, p) = [p[0] * sin(a), p[0] * cos(a), p[1]];

module lip() {
  angles = [for (i = [0:LN]) 180 - lip_span + 2 * lip_span * i / LN];
  pts = [for (a = angles) each [for (p = lip_profile(a)) lip_pt(a, p)]];
  sides = [for (i = [0:LN - 1], j = [0:LP - 1])
    let(a = i * LP + j, b = i * LP + (j + 1) % LP, c = (i + 1) * LP + (j + 1) % LP, d = (i + 1) * LP + j) [a, b, c, d]];
  polyhedron(pts, concat(sides, [[for (j = [LP - 1:-1:0]) j]], [[for (j = [0:LP - 1]) LN * LP + j]]));
}

// The pegs: rounded rectangles reaching in from the lip into the lanyard
// holes, with chamfered tips.
module peg_2d(inset) offset(r = 0.5 - inset / 2) square([peg_w - 1 - inset, peg_h - 1 - inset], center = true);
module pegs() for (sx = [-1, 1]) {
  r0 = r_in(peg_from_back) - fit;      // the device's surface there
  translate([sx * peg_x, 0, disc_t + peg_from_back]) rotate([90, 0, 0]) hull() {
    translate([0, 0, r0 + fit + peg_root]) linear_extrude(0.01) peg_2d(0);
    translate([0, 0, r0 - peg_len + peg_tip]) linear_extrude(0.01) peg_2d(0);
    translate([0, 0, r0 - peg_len]) linear_extrude(0.01) peg_2d(peg_tip);
  }
}

module adapter() difference() {
  union() { disc(); lip(); pegs(); }
  device_pockets();
  grip_cut();
}

adapter();
