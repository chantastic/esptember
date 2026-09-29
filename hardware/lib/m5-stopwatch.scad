// M5Stack Stopwatch (C152) reference geometry for ESPtember hardware.
//
// Load with `use <../lib/m5-stopwatch.scad>`. `use` imports modules and
// functions but not variables, so every value here is a function.
//
// Device frame: the device is centred at the origin, held lanyard-down.
//   +X  the viewer's right (BtnB side)
//   +Z  up the face, toward 12 o'clock
//   -Y  out of the display toward the viewer; +Y out of the back
// The face sits at y = -m5_t()/2 and the back at y = +m5_t()/2.
//
// Sources:
//   [D] M5Stack C152 size drawing, C152-StopWatch-model-size.pdf. It is drawn
//       rotated 180° from lanyard-down and uses first-angle projection.
//   [C] A working third-party holster, counterclock-m5-case-v2. Its seat is
//       Ø52.0, with magnet holes at ±12.5 and holes at (±20, 0).
//   [P] M5Stack product photos: KEYA/G2 on the left, KEYB/G1 on the right,
//       PWR lower left, speaker at the top of the back.
//   [F] Fit prints made from this file, on a Bambu Lab A1 mini with a 0.4 mm
//       nozzle at 0.24 mm layers. 2026-09-29: the puck dock's seat test
//       was a perfect fit on the user's unit, with m5_body(fit = 0.2) as the
//       pocket cutter (the tapered body below, face R1, back R2), and its
//       Ø6.2 magnet holes at 25.0 mm pitch worked, and magnets press-fit
//       into them without glue. The magnets are 6 × 2 mm: pressed flush
//       with the seat in the test's 2.6 mm through-holes, they left three
//       layer lines (0.2 + 2 × 0.24 = 0.68 mm) of wall below them, so
//       they're about 1.92 mm thick. Pockets are 2.4 deep, so a magnet sits
//       2 draft layers (0.48 mm) below the seat and can never stand proud. A straight Ø52.75 pocket
//       (fit 0.4) felt slightly large.
// Positions marked ~ are read off drawings or photos to about ±1 mm.

// The body is a shallow frustum, widest at the face. In [D]'s edge views the
// outline narrows by about 0.6 mm across the thickness (1190 -> ~1176 px at
// 22.9 px/mm). The edges are rounded: R1 at the face, R2 at the back. A seat
// that only grips the back few millimetres sees the narrow, rounded end.
function m5_d() = 51.95;            // [D] face-end diameter, the widest point
function m5_r() = m5_d() / 2;
function m5_d_back() = 51.35;       // ~[D] back-end diameter where the R2 fillet starts, ±0.1; [F] fits
function m5_t() = 15.5;             // [D] thickness
function m5_edge_r_face() = 1;      // [D] "R1"
function m5_edge_r_back() = 2;      // [D] "R2"
// Diameter of the straight side at a distance from the back (valid between the fillets).
function m5_d_at(from_back) = m5_d_back() + (m5_d() - m5_d_back()) * from_back / m5_t();

// Clock position -> unit vector in the device's XZ plane (12 = up, 3 = right).
function m5_clock(c) = [sin(30 * c), 0, cos(30 * c)];
// Axial position measured from the back -> device-frame y.
function m5_from_back(mm) = m5_t() / 2 - mm;

// Edge features: [name, clock, tangential width, axial span from the back, protrusion]
function m5_features() = [
  ["btn_a", 10.5, 9.0, [4.3, 11.2], 1.3],  // [D][P] KEYA/G2 = M5.BtnA, physical left
  ["btn_b",  1.5, 9.0, [4.3, 11.2], 1.3],  // [D][P] KEYB/G1 = M5.BtnB, physical right
  ["pwr",    7.5, 8.0, [5.7, 11.6], 0.6],  // [D][C] red power button; [C] recesses it from 5.5 mm
  ["usb_c",  6.0, 9.0, [6.4, 11.6], 0.0],  // [D] USB-C receptacle, with the lanyard hole beside it
  ["grove",  4.5, 10.0, [4.3, 10.7], 0.0]  // [D] HY2.0-4P (GND 5V G10 G11)
];

// Rear features, as [x, z] in the device frame.
function m5_magnets(pitch = 25.0) =                  // [C][F] 25.0 proven by the seat test; [D] draws 25.46
  [for (sx = [-1, 1], sz = [-1, 1]) [sx * pitch / 2, sz * pitch / 2]];
function m5_magnet_d() = 5.0;                        // [D]
function m5_rear_bosses() = [[-20, 0], [20, 0]];     // [D] 2-ø4 at 40 mm; [C] clears them with Ø4.2
function m5_speaker() = [0, 17, 10, 10];             // ~[D] grille centre x, z, then width, height

// The device itself: a tapered body with rounded edges. `fit` grows it
// outward and along the axis by the same amount, so it works as a pocket cutter.
module m5_body(fit = 0) {
  rf = m5_r() + fit;
  rb = m5_d_back() / 2 + fit;
  h = m5_t() / 2 + fit;
  e1 = m5_edge_r_face();
  e2 = m5_edge_r_back();
  rotate([90, 0, 0]) rotate_extrude($fn = 160) hull() {   // profile z: -h = back, +h = face
    translate([0, -h]) square([0.01, 2 * h]);
    translate([rb - e2, -h + e2]) circle(e2, $fn = 48);
    translate([rf - e1, h - e1]) circle(e1, $fn = 48);
  }
}

module m5_feature_box(f, radial_in, radial_out, extra_w = 0, extra_axial = 0) {
  span = f[3];
  rotate([0, 30 * f[1], 0])   // put the box at the feature's clock position
    translate([-(f[2] + extra_w) / 2, m5_from_back(span[1]) - extra_axial, m5_r() - radial_in])
      cube([f[2] + extra_w, span[1] - span[0] + 2 * extra_axial, radial_in + radial_out]);
}

// Device plus button nubs, for translucent previews.
module m5_ghost() {
  m5_body();
  for (f = m5_features()) if (f[4] > 0) m5_feature_box(f, 0.5, f[4]);
}

// Space that a stand must leave empty: finger room at each button and at
// PWR, plug room at USB-C and Grove, and an air gap behind the speaker.
// Intersecting a stand with m5_keepouts() must render empty. Parts that cut
// these windows should subtract m5_keepouts(grow = 0.1), so the test checks
// real clearance instead of two coincident surfaces.
module m5_keepouts(finger = 5, plug = 8, grow = 0) {
  for (f = m5_features())
    m5_feature_box(f, 0.5 + grow, ((f[0] == "usb_c" || f[0] == "grove") ? plug : finger) + grow,
                   extra_w = ((f[0] == "usb_c") ? 3.5 : 1) + 2 * grow, extra_axial = 0.5 + grow);
  s = m5_speaker();
  translate([s[0] - s[2] / 2 - grow, m5_t() / 2 + 0.01, s[1] - s[3] / 2 - grow])
    cube([s[2] + 2 * grow, 1.5 + grow, s[3] + 2 * grow]);
}
