// M5Stack Stopwatch MagSafe adapter — parametric OpenSCAD source.
//
// A thin disc between the Stopwatch and the back of a MagSafe iPhone.
//   - Device side: four 6 × 2 mm magnets on the Stopwatch's 25 mm square,
//     facing its own back magnets (the proven pitch and press fit).
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
// Frame: XY is the device's face plane, 12 o'clock at +Y. The device side is
// z = 0, which prints on the bed, and the phone side is z = disc_t.
// Units are millimetres.
//
// Render:  openscad --backend=manifold --export-format binstl -o stopwatch-magsafe-adapter.stl stopwatch-magsafe-adapter.scad
// Print:   as exported, device side down, no supports.

use <../lib/m5-stopwatch.scad>

grip = "steel_ring";    // "steel_ring" or "magnets"

/* [Layers] Bambu A1 mini, 0.4 nozzle: 0.2 first layer, then 0.16 (Optimal) or 0.24 (Draft) */
pocket_depth = 2.12;    // the 6 × 2 magnets; a layer boundary on both Draft and Optimal
skin = 0.32;            // over each pocket: two Optimal layers (one on Draft), a floor for glue
disc_t = pocket_depth + skin;   // 2.44 = 0.2 + 14 × 0.16

/* [Device magnets] */
magnet_pitch = 25;      // proven by the dock's seat test
magnet_hole_d = 6.2;    // the proven press fit
magnet_lead = 0.3;      // chamfer at each pocket's mouth

/* [Phone grip: steel ring] e.g. KraftGeek universal metal ring sticker: 55 OD, 45 ID, 0.55 thick */
ring_od = 55;
ring_id = 45;
ring_t = 0.55;
ring_clear = 0.3;       // radial clearance each side of the ring
ring_recess = 0.64;     // so its floor is at 1.80 = 0.2 + 10 × 0.16, and the ring sits just below flush

/* [Phone grip: magnets] */
phone_r = 25.3;         // the middle of MagSafe's magnet ring (about 46 ID to 54–56 OD)
phone_n = 16;           // around the ring, 9.9 mm apart

/* [Disc] */
rim = 1.2;              // wall outside the grip
top_round = 0.8;        // round-over on the phone side's edge
foot_chamfer = 0.3;     // on the bed side's edge, against elephant's foot

$fn = 96;

disc_r = (grip == "steel_ring" ? ring_od / 2 + ring_clear : phone_r + magnet_hole_d / 2) + rim;

module disc() rotate_extrude($fn = 240) polygon(concat(
  [[0, 0], [disc_r - foot_chamfer, 0], [disc_r, foot_chamfer], [disc_r, disc_t - top_round]],
  [for (a = [10:10:90]) [disc_r - top_round + top_round * cos(a), disc_t - top_round + top_round * sin(a)]],
  [[0, disc_t]]));

// A pocket for a 6 × 2 magnet, opening at z0 and running pocket_depth up
// (dir = 1) or down (dir = -1), with a lead-in chamfer at its mouth.
module pocket(z0, dir) {
  translate([0, 0, dir > 0 ? z0 - 0.01 : z0 - pocket_depth]) cylinder(h = pocket_depth + 0.01, d = magnet_hole_d);
  translate([0, 0, z0]) mirror([0, 0, dir > 0 ? 0 : 1])
    translate([0, 0, -0.01]) cylinder(h = magnet_lead + 0.01, d1 = magnet_hole_d + 2 * magnet_lead + 0.02, d2 = magnet_hole_d);
}

// The device magnets: pockets open on the device side (the bed), with the
// skin on the phone side, bridged over each pocket.
module device_pockets() for (m = m5_magnets(magnet_pitch)) translate([m[0], m[1], 0]) pocket(0, 1);

// The phone grip.
module grip_cut() {
  if (grip == "steel_ring")
    translate([0, 0, disc_t - ring_recess]) difference() {
      cylinder(h = ring_recess + 0.01, r = ring_od / 2 + ring_clear, $fn = 240);
      translate([0, 0, -0.01]) cylinder(h = ring_recess + 0.03, r = ring_id / 2 - ring_clear, $fn = 240);
    }
  else
    for (i = [0:phone_n - 1]) rotate(360 * i / phone_n) translate([0, phone_r, 0]) pocket(disc_t, -1);
}

module adapter() difference() {
  disc();
  device_pockets();
  grip_cut();
}

adapter();
