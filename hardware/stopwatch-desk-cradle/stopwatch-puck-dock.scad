// M5Stack Stopwatch puck dock — parametric OpenSCAD source.
//
// A low, tilted puck: a thin seat plate on a skirt that sweeps in to a small
// round foot, in the style of a case-less display harness. The seat follows a
// proven open holster. A low lip wraps only the lower half, so both buttons
// (10:30 and 1:30), the power button, USB-C and Grove stay fully exposed.
// Four magnet holes sit under the device's rear magnets, and a notch clears
// the rear speaker. A tunnel under the device carries the USB-C cable from
// the front slot to a port in the back of the skirt.
//
// Device geometry comes from ../lib/m5-stopwatch.scad. Units are millimetres.
//
// Render:  openscad --backend=manifold -o stopwatch-puck-dock.stl stopwatch-puck-dock.scad
// Preview (F5) also draws the device and, translucent, the space the dock must leave clear.

use <../lib/m5-stopwatch.scad>

/* [Fit] */
fit = 0.2;           // clearance per side; the pocket follows the body's taper and back fillet

/* [Pose and proportions] */
face_angle = 30;     // face tilt from horizontal (the reference harness uses ~26)
lip_h = 7.0;         // lip height up the device's side, from its back; the back 2 mm is its R2 fillet
wall = 2.8;          // lip and seat-rim thickness
floor_t = 2.84;      // seat plate: 0.2 + 11 × 0.24, so the flat seat test lands on the A1 mini's draft layer grid
lip_from = 2.5;      // the lip runs clockwise from this clock position...
lip_to = 9.5;        // ...through 6 o'clock to this one
skirt_min = 3.0;     // shoulder height at its lowest (front) point
foot_d = 44;         // round foot diameter
foot_shift = 2;      // foot centre behind the device centre
flare = 1.8;         // skirt curvature: 1 = straight cone, higher = more waisted
edge = 0.8;          // chamfer on the seat rim
foot_chamfer = 0.6;  // bottom chamfer, also offsets elephant's foot
shoulder = 0.4;      // step where the seat plate overhangs the skirt; keeps the seam clean

/* [Rear features] */
magnet_pitch = 25.0; // holster-proven; the drawing says 25.46
magnet_hole_d = 6.2; // proven press fit for magnets; also clears the device's own
magnet_hole_t = 2.4; // 10 draft layers: a 6 × 2 magnet (≈1.92, 8 layers) sits 2 layers (0.48) below the seat, never proud
magnet_skin = 0;     // > 0 buries the magnets under this much filament (0.48 = two 0.24 mm layers on the A1 mini)
magnet_bore_d = 6.6; // with a skin, a looser channel behind each pocket to push the magnet in from behind
boss_hole_d = 4.2;   // clearance for the rear ø4 features at (±20, 0)
boss_hole_t = 1.5;
speaker_w = 10;      // notch behind the rear speaker
speaker_from = 11;   // the notch starts this far above the device centre
speaker_t = 5;       // how deep the notch reaches behind the seat

/* [Cable] */
slot_w = 14;         // front slot at 6 o'clock for the USB-C plug and lanyard
slot_depth = 7;      // how far the slot reaches up from the bottom rim
tunnel_w = 5.5;      // tunnel under the device, front slot to rear port
tunnel_h = 4.5;      // straight-wall height; a 45° roof adds tunnel_w / 2
tunnel_floor = 1.2;  // skin between the tunnel and the desk

/* [Test print] */
seat_only = false;   // true prints just the seat and lip, lying flat, for a quick fit check

$fn = 160;
N = 160;             // skirt loft resolution around
K = 24;              // skirt loft resolution up

// Derived. Device frame as in m5-stopwatch.scad: +Y points into the dock.
R = m5_r();
T = m5_t();
Rin = R + fit;
Tin = T + 2 * fit;
Rc = Rin + wall;
t = 90 - face_angle;              // device axis tilt from vertical
y_seat = Tin / 2;                 // seat surface
y_sh = y_seat + floor_t;          // shoulder plane
y_lip = T / 2 - lip_h;            // lip top, measured from the device's back
cz = skirt_min + y_sh * sin(t) + Rc * cos(t);

function world(p) = [p[0], p[1] * cos(t) + p[2] * sin(t), cz - p[1] * sin(t) + p[2] * cos(t)];
function clock_angle(c) = 90 - 30 * c;   // clock position -> 2D angle in the device XZ plane

module pose() translate([0, 0, cz]) rotate([-t, 0, 0]) children();
module slab_y(y0, y1) translate([0, y1, 0]) rotate([90, 0, 0]) linear_extrude(y1 - y0) children();

module wedge2d(r, a0, a1, n = 48)
  polygon(concat([[0, 0]], [for (i = [0:n]) let(a = a0 + (a1 - a0) * i / n) r * [cos(a), sin(a)]]));

// Skirt: loft from the chamfered foot up to the shoulder ring.
module skirt() {
  foot = [for (i = [0:N - 1]) let(a = 360 * i / N)
    [(foot_d / 2) * cos(a), foot_shift + (foot_d / 2) * sin(a), foot_chamfer]];
  rs = Rc - shoulder;
  sh = [for (i = [0:N - 1]) let(a = 360 * i / N) world([rs * cos(a), y_sh, rs * sin(a)])];
  cap = [for (i = [0:N - 1]) let(a = 360 * i / N) world([rs * cos(a), y_sh - 1, rs * sin(a)])];  // inside the plate
  ring0 = [for (i = [0:N - 1]) let(a = 360 * i / N)
    [(foot_d / 2 - foot_chamfer) * cos(a), foot_shift + (foot_d / 2 - foot_chamfer) * sin(a), 0]];
  rings = concat([ring0], [for (k = [0:K]) let(s = k / K, e = pow(s, flare))
    [for (i = [0:N - 1]) [foot[i][0] + (sh[i][0] - foot[i][0]) * e,
                          foot[i][1] + (sh[i][1] - foot[i][1]) * e,
                          foot[i][2] + (sh[i][2] - foot[i][2]) * s]]], [cap]);
  L = len(rings);
  polyhedron(
    points = [for (r = rings) for (p = r) p],
    faces = concat(
      [[for (i = [0:N - 1]) i]],
      [[for (i = [N - 1:-1:0]) (L - 1) * N + i]],
      [for (k = [0:L - 2]) for (i = [0:N - 1]) let(j = (i + 1) % N)
        [k * N + i, (k + 1) * N + i, (k + 1) * N + j, k * N + j]]));
}

// Seat plate and lip as one solid, so their outer walls are a single surface.
// The open top half is then cut down to the plate, with a chamfered rim.
module seat() pose() difference() {
  slab_y(y_lip, y_sh) circle(Rc);
  intersection() {
    difference() {
      slab_y(y_lip - 1, y_seat + edge) circle(Rc + 1);
      translate([0, y_seat + edge + 0.001, 0]) rotate([90, 0, 0])
        cylinder(h = edge + 0.002, r1 = Rc, r2 = Rc - edge);
    }
    slab_y(y_lip - 2, y_seat + edge + 1) wedge2d(2 * Rc, clock_angle(lip_from), clock_angle(lip_to) + 360);
  }
}

module front_slot() pose()
  translate([-slot_w / 2, y_lip - 30, -R - 30]) cube([slot_w, 30 + y_seat - y_lip, 30 + slot_depth]);

// Level tunnel with a 45° roof, so it prints without supports.
module tunnel()
  rotate([90, 0, 0]) linear_extrude(120, center = true)
    polygon([[-tunnel_w / 2, tunnel_floor], [tunnel_w / 2, tunnel_floor],
             [tunnel_w / 2, tunnel_floor + tunnel_h], [0, tunnel_floor + tunnel_h + tunnel_w / 2],
             [-tunnel_w / 2, tunnel_floor + tunnel_h]]);

module dock() difference() {
  union() { skirt(); seat(); }
  pose() {
    m5_body(fit);                                                    // seat pocket, tapered like the body
    m5_keepouts(grow = 0.1);                                         // windows for Grove, USB-C and power
    for (m = m5_magnets(magnet_pitch)) translate([m[0], 0, m[1]]) rotate([-90, 0, 0]) {
      if (magnet_skin > 0) {                    // press-fit pocket under a skin, loaded from behind
        translate([0, 0, y_seat + magnet_skin]) cylinder(h = magnet_hole_t, d = magnet_hole_d);
        translate([0, 0, y_seat + magnet_skin + magnet_hole_t - 0.01]) cylinder(h = 80, d = magnet_bore_d);
      } else {
        translate([0, 0, y_seat - 0.01]) cylinder(h = magnet_hole_t + 0.01, d = magnet_hole_d);
      }
    }
    for (b = m5_rear_bosses())
      translate([b[0], y_seat - 0.01, b[1]]) rotate([-90, 0, 0]) cylinder(h = boss_hole_t + 0.01, d = boss_hole_d);
    translate([-speaker_w / 2, y_seat - 1, speaker_from]) cube([speaker_w, speaker_t + 1, Rc]);
  }
  hull() {                                                           // front slot, down to the desk
    front_slot();
    translate([0, 0, -1]) linear_extrude(1) projection() front_slot();
  }
  intersection() {                                                   // tunnel: from the slot to the back
    tunnel();
    translate([-50, world([0, y_seat, -R])[1] - 8, -1]) cube([100, 100, 50]);
  }
}

// The seat and lip alone, laid on the seat's back, for a quick fit print.
module seat_test()
  translate([0, 0, y_sh]) rotate([-90, 0, 0]) intersection() {
    rotate([t, 0, 0]) translate([0, 0, -cz]) dock();
    slab_y(y_lip - 1, y_sh) circle(Rc + 1);
  }

if (seat_only) seat_test();
else {
  dock();
  %pose() { m5_ghost(); m5_keepouts(); }
}
