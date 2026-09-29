// M5Stack Stopwatch desk cradle — parametric OpenSCAD source.
//
// Holds the Stopwatch upright on a desk, face tilted back, lanyard loop at the
// bottom. Two cheeks cup the rim near 3 and 9 o'clock. That sits between the
// buttons (10:30, 1:30) and the bottom-edge features (Grove 4:30, USB-C 6,
// power 7:30), so all of them stay clear. A back rest carries four magnet
// holes and a notch for the rear speaker. A tunnel under the back rest takes
// the cable out behind.
//
// Device geometry comes from ../lib/m5-stopwatch.scad. Units are millimetres.
//
// Render:  openscad --backend=manifold -o stopwatch-desk-cradle.stl stopwatch-desk-cradle.scad
// Preview (F5) also draws the device and, translucent, the space the cradle must leave clear.

use <../lib/m5-stopwatch.scad>

/* [Fit] */
fit = 0.2;           // clearance per side; the cut follows the body's taper and edge fillets

/* [Pose] */
tilt = 18;           // degrees the face leans back from vertical
lift = 14;           // lowest rim point to base top: lanyard loop and a right-angle USB-C plug

/* [Cradle] */
wall = 3.0;          // rim wall thickness
lip_overlap = 1.5;   // front lip reach over the bezel; keep it off the active display
lip_t = 2.0;         // front lip thickness
cheek_from = -12;    // cheeks start this far below horizontal (negative = above); buttons start 35° above
cheek_to = 28;       // and stop this far below horizontal; Grove and power start 35° below
back_t = 5.0;        // back rest thickness
back_top = 0.62;     // back rest top, as a fraction of the radius above centre (covers the top magnets)

/* [Base] */
base_t = 4.0;
base_w = 70;
base_front = 26;     // reach in front of the device centre
base_back = 24;      // reach behind the device centre
base_r = 6;          // corner radius

/* [Rear features] */
magnet_pitch = 25.0; // holster-proven; the drawing says 25.46
magnet_hole_d = 6.2; // proven press fit for magnets (puck dock seat test); also clears the device's own
magnet_hole_t = 2.4; // a 6 × 2 magnet (≈1.92) sits 0.48 (2 draft layers) below the seat, never proud
magnet_skin = 0;     // > 0 buries the magnets under this much filament (0.48 = two 0.24 mm layers on the A1 mini)
magnet_bore_d = 6.6; // with a skin, a looser channel behind each pocket to push the magnet in from behind
boss_hole_d = 4.2;   // clearance for the rear ø4 features at (±20, 0)
boss_hole_t = 1.5;
speaker_w = 10;      // notch behind the rear speaker
speaker_from = 11;   // starts this far above the device centre
header_window_d = 0; // > 0 cuts a round window through the back rest
cable_w = 12;        // tunnel under the back rest for a rear-exiting cable
cable_h = 9;

$fn = 128;

// Derived
R = m5_r();
Rin = R + fit;
Tin = m5_t() + 2 * fit;
gap_w = 2 * Rin * cos(cheek_to);
center_z = base_t + lift + R * cos(tilt) + m5_t() / 2 * sin(tilt);

// Device frame: X right, Z up the face, +Y out of the back of the device.
module pose() translate([0, 0, center_z]) rotate([-tilt, 0, 0]) children();

// Extrude a 2D shape drawn in the device's XZ plane between y0 and y1.
module slab_y(y0, y1) translate([0, y1, 0]) rotate([90, 0, 0]) linear_extrude(y1 - y0) children();

module sector2d(r_in, r_out, a0, a1, n = 48) {
  polygon(concat(
    [for (i = [0:n]) let(a = a0 + (a1 - a0) * i / n) r_out * [cos(a), sin(a)]],
    [for (i = [n:-1:0]) let(a = a0 + (a1 - a0) * i / n) r_in * [cos(a), sin(a)]]));
}

// Extend a part straight down to the desk.
module grounded() hull() { children(); linear_extrude(base_t) projection() children(); }

module cheek(a0, a1) {
  slab_y(-Tin / 2 - lip_t, Tin / 2 + back_t) sector2d(Rin, Rin + wall, a0, a1);
  slab_y(-Tin / 2 - lip_t, -Tin / 2) sector2d(R - lip_overlap, Rin + wall, a0, a1);
}

module back_rest()
  slab_y(Tin / 2, Tin / 2 + back_t) intersection() {
    circle(Rin + wall / 2);
    translate([-2 * R, -2 * R]) square([4 * R, 2 * R + back_top * R]);
  }

module base()
  translate([0, (base_back - base_front) / 2, 0])
    linear_extrude(base_t) offset(r = base_r) offset(delta = -base_r)
      square([base_w, base_front + base_back], center = true);

module cradle() difference() {
  union() {
    base();
    grounded() pose() cheek(180 + cheek_from, 180 + cheek_to);
    grounded() pose() cheek(360 - cheek_to, 360 - cheek_from);
    grounded() pose() back_rest();
  }
  pose() {
    m5_body(fit);                                                  // device body, tapered
    slab_y(-Tin / 2 - 60, -Tin / 2 + 0.01) circle(R - lip_overlap); // view of the face
    for (m = m5_magnets(magnet_pitch)) translate([m[0], 0, m[1]]) rotate([-90, 0, 0]) {
      if (magnet_skin > 0) {                    // press-fit pocket under a skin, loaded from behind
        translate([0, 0, Tin / 2 + magnet_skin]) cylinder(h = magnet_hole_t, d = magnet_hole_d);
        translate([0, 0, Tin / 2 + magnet_skin + magnet_hole_t - 0.01]) cylinder(h = 80, d = magnet_bore_d);
      } else {
        translate([0, 0, Tin / 2 - 0.01]) cylinder(h = magnet_hole_t + 0.01, d = magnet_hole_d);
      }
    }
    for (b = m5_rear_bosses())
      translate([b[0], Tin / 2 - 0.01, b[1]]) rotate([-90, 0, 0]) cylinder(h = boss_hole_t + 0.01, d = boss_hole_d);
    translate([-speaker_w / 2, Tin / 2 - 1, speaker_from]) cube([speaker_w, back_t + 40, 2 * R]);
    m5_keepouts(grow = 0.1);                                       // buttons, ports, speaker
    if (header_window_d > 0)
      slab_y(Tin / 2 - 0.01, Tin / 2 + back_t + 60) circle(d = header_window_d);
  }
  // Open the bottom edge (Grove, USB-C, lanyard, power) without cutting the base.
  intersection() {
    pose() translate([-gap_w / 2, -Tin / 2 - lip_t - 60, -R - 60]) cube([gap_w, 60 + lip_t + Tin, 60 + R]);
    translate([-100, -100, base_t]) cube([200, 200, 200]);
  }
  // Tunnel under the back rest so a cable can leave from the rear.
  translate([-cable_w / 2, -base_front - 1, base_t - 0.01]) cube([cable_w, base_front + base_back + 2, cable_h]);
}

cradle();
%pose() { m5_ghost(); m5_keepouts(); }
