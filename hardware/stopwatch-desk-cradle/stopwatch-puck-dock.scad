// M5Stack Stopwatch puck dock — parametric OpenSCAD source.
//
// A ball, cut on top by the tilted seat and flat underneath to sit on the
// table, with no hard edges. The ball's centre sits on the device's axis,
// sized so the ball passes exactly through the rim, and the whole outside is
// one lofted surface along the ball's meridians. The rim is the same all the
// way round: it covers just the device's rounded back edge and rounds over
// about 1 mm thick against its side, so the dock ends at the device's
// silhouette. Buttons, power, USB-C and Grove all sit above it.
//
// The seat matches the device's back. Four magnet pockets sit under its
// magnets. A step fills the recessed logo segment at the bottom, and a deep
// finger pocket follows the speaker recess at the top: put a finger in it
// and pull the device out, pivoting on the step. An open, rounded channel
// under the foot takes the USB-C cable from front to back, and it leaves the
// ball at each end through a filleted arch. The cable lies in from below and
// never has to be threaded.
//
// Device geometry comes from ../lib/m5-stopwatch.scad. Units are millimetres.
//
// Render:  openscad --backend=manifold --export-format binstl -o stopwatch-puck-dock.stl stopwatch-puck-dock.scad
// Preview (F5) also draws the device and, translucent, the space the dock must leave clear.

use <../lib/m5-stopwatch.scad>

/* [Fit] */
fit = 0.2;           // clearance per side; the seat follows the body's taper and back fillet (proven)

/* [Pose] */
face_angle = 30;     // face tilt from horizontal (the reference harness uses ~26)
tip_front_z = 11.5;  // height of the rim's lowest (front) point: a straight USB-C plug up to 28 mm clears the desk

/* [Rim and ball] */
h_rim = 1.2;         // rim height up the device's side, all the way round: just over the back's R2 fillet
rim_t = 1.0;         // rim thickness where it rounds over against the device
ball_e = 10;         // the ball's centre sits this far behind the seat, on the device's axis
foot_relief = 0.4;   // the bottom edge steps in this much, to offset elephant's foot

/* [Seat features] */
magnet_pitch = 25.0; // holster-proven; the drawing says 25.46
magnet_hole_d = 6.2; // proven press fit for 6 mm magnets
magnet_hole_t = 2.4; // 10 draft layers: a 6 × 2 magnet (≈1.92) sits 0.48 below the seat, never proud
magnet_lead = 0.4;   // rounded-off lead-in at each pocket mouth
pull_depth = 6;      // finger pocket behind the speaker recess: depth below the seat at the grille centre;
                     // its floor is level in the printed dock, so the full depth sits on one layer
pull_margin = 0.3;   // its outline this much outside the recess's outer edge (clears the top magnets)
pull_round = 1.2;    // the trough's edge eases out this far past the outline, tangent to the seat
pull_flat = 0.45;    // share of the trough's half-width that's flat floor
pull_ease = 3.5;     // the trough eases in from nothing over this length at the tongue's tip
step_clear = 0.2;    // the bottom step stops this much short of the logo recess's depth...
step_gap = 0.3;      // ...and sits this much further out, clear of the device's ramp

/* [Cable] */
channel_r = 2.5;     // open channel under the foot: 5 mm wide with a round roof, for the cable only
channel_z = 2.0;     // roof centre height; the roof tops out at channel_z + channel_r (4.5 mm)
channel_soft = 0.8;  // rounded lips where the channel meets the desk
channel_root = 3.3;  // plan half-width at each end: a gentle hourglass
channel_waist = 0.45; // share of the length kept at cable width through the middle
channel_fillet = 1.5; // the channel's rounded edge where it leaves the ball, front and back

/* [Test print] */
seat_only = false;   // true prints just the seat and rim, lying flat, for a quick fit check
test_floor = 7.16;   // seat test plate: 0.2 + 29 × 0.24 on the A1 mini's draft grid, deep enough to keep the finger pocket's floor

$fn = 96;
N = 360;             // loft resolution around
K = 56;              // samples along each meridian
M = 8;               // rim round-over samples
L = 16;              // seat-wall samples, denser toward the fillet

// Derived. Device frame as in m5-stopwatch.scad: +Y points into the dock.
R = m5_r();
T = m5_t();
rb = m5_d_back() / 2;
e1 = m5_edge_r_face();
e2 = m5_edge_r_back();
t = 90 - face_angle;                 // device axis tilt from vertical
y_seat = T / 2 + fit;                // seat surface (pocket floor)
b = rim_t / 2;                       // round-over radius
uc = h_rim - b;                      // round-over centre height

function smooth(x) = x <= 0 ? 0 : x >= 1 ? 1 : (1 - cos(180 * x)) / 2;

// Pocket wall radius at height u above the seat: m5_body(fit)'s profile.
function pocket_r(u) =
  u <= e2 ? (rb + fit - e2) + sqrt(max(0, e2 * e2 - (e2 - u) * (e2 - u)))
          : (rb + fit) + (R - rb) * (u - e2) / (T + 2 * fit - e1 - e2);

// Device frame <-> world. The device centre sits at height cz.
r_tip = pocket_r(uc) + rim_t;        // the rim's outer edge
cz = tip_front_z + (y_seat - uc) * sin(t) + r_tip * cos(t);
function world(p) = [p[0], p[1] * cos(t) + p[2] * sin(t), cz - p[1] * sin(t) + p[2] * cos(t)];
function wdir(v) = [v[0], v[1] * cos(t) + v[2] * sin(t), -v[1] * sin(t) + v[2] * cos(t)];
// A profile point (radius rho, height u above the seat) at 2D angle a, in the world.
function pt(a, rho, u) = world([rho * cos(a), y_seat - u, rho * sin(a)]);

// The ball: centre C on the device axis behind the seat, radius Rs through the rim.
C = world([0, y_seat + ball_e, 0]);
Rs = sqrt(r_tip * r_tip + pow(ball_e + uc, 2));
foot_r0 = sqrt(Rs * Rs - C[2] * C[2]);            // the table cut: a circle around (0, C[1])
// Angle on a meridian (C + Rs(rh·cos + yh·sin)) where it reaches height z, on the dock's side.
function meridian_at_z(rh, yh, z) =
  let(A = rh[2], B = yh[2], q = (z - C[2]) / Rs, m = sqrt(A * A + B * B), ph = atan2(B, A))
  ph + acos(max(-1, min(1, q / m)));
// The ball's front and back surface on the centreline, at height z.
function ball_y(z, side) = C[1] + side * sqrt(Rs * Rs - pow(z - C[2], 2));

// One profile, from the foot's bottom edge round to the seat's rim.
function profile(a) =
  let(
    ri = pocket_r(uc),                            // rim's inner edge, on the seat wall
    tip = pt(a, r_tip, uc),                       // rim's outer edge, on the ball
    rh = wdir([cos(a), 0, sin(a)]),               // this profile's meridian of the ball: radial...
    yh = wdir([0, 1, 0]),                         // ...and along the device axis, into the dock
    th_t = atan2((tip - C) * yh, (tip - C) * rh), // the rim's angle on the meridian
    th_f = meridian_at_z(rh, yh, 0),              // where the meridian meets the table
    ring = [for (k = [0:K]) let(th = th_f + (th_t - th_f) * k / K) C + (rh * cos(th) + yh * sin(th)) * Rs],
    base = ring[0])
  concat(
    [[base[0], base[1], 0] - foot_relief * [base[0], base[1] - C[1], 0] / norm([base[0], base[1] - C[1]])],   // relieved bottom edge
    [for (k = [0:K]) ring[k] + [0, 0, k == 0 ? foot_relief : 0]],
    [for (j = [1:M]) let(g = 180 * j / M) pt(a, ri + b + b * cos(g), uc + b * sin(g))],   // round-over
    [for (k = [1:L]) let(u = uc * pow(1 - k / L, 2)) pt(a, pocket_r(u), u)]);            // seat wall, rim -> floor

module body() {
  P = [for (i = [0:N - 1]) profile(360 * i / N)];   // P[i][k]
  NP = len(P[0]);
  polyhedron(
    points = [for (k = [0:NP - 1]) for (i = [0:N - 1]) P[i][k]],
    faces = concat(
      [[for (i = [0:N - 1]) i]],                                        // foot
      [[for (i = [N - 1:-1:0]) (NP - 1) * N + i]],                       // seat floor
      [for (k = [0:NP - 2]) for (i = [0:N - 1]) let(j = (i + 1) % N)
        [k * N + i, (k + 1) * N + i, (k + 1) * N + j, k * N + j]]));
}

module pose() translate([0, 0, cz]) rotate([-t, 0, 0]) children();
module slab_y(y0, y1) translate([0, y1, 0]) rotate([90, 0, 0]) linear_extrude(y1 - y0) children();

// The channel is one lofted solid: a round-roofed U whose plan half-width
// changes along its length, a gentle hourglass. At each end it rolls over on
// a quarter-circle fillet placed where the fillet's top meets the ball
// (the ball overhangs there), then grows only slightly past it.
function channel_at(t) =                      // t from -1 (front end) to 1 (back end)
  channel_r + (channel_root - channel_r) * pow(max(0, (abs(t) - channel_waist) / (1 - channel_waist)), 2);
function channel_section(y, rp0, grow = 0) =
  let(m = 16, rz = channel_r * sqrt(rp0 / channel_r) + grow, rp = rp0 + grow)
  concat(
    [[rp + channel_soft, y, -2], [rp + channel_soft, y, 0]],
    [for (j = [0:m]) let(ph = 180 * j / m) [rp * cos(ph), y, channel_z + rz * sin(ph)]],
    [[-rp - channel_soft, y, 0], [-rp - channel_soft, y, -2]]);

module channel() {
  f = channel_fillet;
  y0 = C[1] - foot_r0 - 4;
  y1 = C[1] + foot_r0 + 4;
  tof = function(y) -1 + 2 * (y - y0) / (y1 - y0);
  top = channel_z + channel_r * sqrt(channel_root / channel_r) + f;   // the fillet's top at each end
  yf = ball_y(top, -1);
  yb = ball_y(top, 1);
  n = 60;
  head = concat([channel_section(y0, channel_root, f + 0.3)],
                [for (k = [10:-1:1]) let(ph = 90 * k / 10, y = yf + f * (1 - sin(ph)))
                   channel_section(y, channel_at(tof(y)), f * (1 - cos(ph)))]);
  mid = [for (k = [0:n]) let(y = yf + f + (yb - f - (yf + f)) * k / n) channel_section(y, channel_at(tof(y)))];
  tail = concat([for (k = [1:10]) let(ph = 90 * k / 10, y = yb - f * (1 - sin(ph)))
                   channel_section(y, channel_at(tof(y)), f * (1 - cos(ph)))],
                [channel_section(y1, channel_root, f + 0.3)]);
  secs = concat(head, mid, tail);
  n2 = len(secs) - 1;
  P = len(secs[0]);
  polyhedron(
    points = [for (sec = secs) for (q = sec) q],
    faces = concat(
      [[for (j = [P - 1:-1:0]) j]],
      [[for (j = [0:P - 1]) n2 * P + j]],
      [for (k = [0:n2 - 1]) for (j = [0:P - 1]) let(i2 = (j + 1) % P)
        [k * P + j, k * P + i2, (k + 1) * P + i2, (k + 1) * P + j]]));
}

// Finger pocket: a smooth trough that follows the speaker recess's outline
// and runs out through the top of the ball. It's lofted from sections in
// level (world x-z) planes along the recess's centreline. Each section has
// a flat floor band and then one S-curve each side, tangent to the floor and
// easing out tangent to the seat, so there's no separate lip. The floor is level at z_pull, so the full depth prints on
// one layer; near the tongue's tip the trough eases in from nothing.
function pull_half(s) =                        // recess half-width at height s up the face, plus margin
  let(hw = m5_speaker_recess_half(),
      i = max([for (k = [0:len(hw) - 1]) if (hw[k][0] <= s) k]))
  i >= len(hw) - 1 ? hw[len(hw) - 1][1] + pull_margin
    : hw[i][1] + (hw[i + 1][1] - hw[i][1]) * (s - hw[i][0]) / (hw[i + 1][0] - hw[i][0]) + pull_margin;
layer0 = 0.2;        // the A1 mini draft grid: first layer, then layer height
layer_h = 0.24;
z_pull_raw = world([0, y_seat, m5_speaker()[1]])[2] - pull_depth;
z_pull = layer0 + layer_h * round((z_pull_raw - layer0) / layer_h);   // snapped to a layer boundary

function pull_section(s) =
  let(p = world([0, y_seat, s]),               // the seat's centreline at s, in the world
      zs = p[2],
      tip = m5_speaker_recess_half()[0][0],
      d = max(0.05, (zs - z_pull) * smooth((s - tip) / pull_ease)),
      zb = zs - d,                               // floor: level at z_pull once at full depth
      a = max(0.2, pull_half(s)),
      xo = a + pull_round,                       // where the curve meets the seat
      xf = pull_flat * a, m = 24, top = 0.95)
  // One S-curve each side: tangent to the flat floor, up the wall, and easing
  // out tangent to the seat. It stops just short of tangency, 0.6% of the
  // depth below the seat, then turns up into air, so nothing lies on the seat.
  concat(
    [[xo + 1.5, p[1], zs + 4]],
    [for (j = [m:-1:0]) let(u = top * j / m) [xf + (xo - xf) * u / top, p[1], zb + d * smooth(u)]],
    [for (j = [0:m]) let(u = top * j / m) [-(xf + (xo - xf) * u / top), p[1], zb + d * smooth(u)]],
    [[-(xo + 1.5), p[1], zs + 4]]);

module finger_pocket() {
  s0 = m5_speaker_recess_half()[0][0] + 0.05;
  s1 = R + 22;                                   // well past the rim, out through the ball
  n = 90;
  secs = [for (k = [0:n]) pull_section(s0 + (s1 - s0) * k / n)];
  P = len(secs[0]);
  polyhedron(
    points = [for (sec = secs) for (q = sec) q],
    faces = concat(
      [[for (j = [0:P - 1]) j]],
      [[for (j = [P - 1:-1:0]) n * P + j]],
      [for (k = [0:n - 1]) for (j = [0:P - 1]) let(j2 = (j + 1) % P)
        [k * P + j, (k + 1) * P + j, (k + 1) * P + j2, k * P + j2]]));
}

// Bottom step: fills the device's recessed logo segment, with a matching ramp.
module bottom_step() pose() {
  lr = m5_logo_recess();
  intersection() {
    hull() {
      slab_y(y_seat - (lr[2] - step_clear), y_seat + 0.5) m5_segment_2d(lr[0] - step_gap);
      slab_y(y_seat - 0.01, y_seat + 0.5) m5_segment_2d(lr[1] - step_gap);
    }
    slab_y(y_seat - h_rim - 1, y_seat + 0.5) circle(R + fit);
  }
}

// pocket = false leaves out the finger pocket, for parts that reuse the seat
// without needing to pull the device out from the top (the tripod mount).
module dock(pocket = true) union() {
  difference() {
    body();
    pose() for (m = m5_magnets(magnet_pitch)) translate([m[0], y_seat, m[1]]) rotate([-90, 0, 0]) {
      translate([0, 0, -0.01]) cylinder(h = magnet_hole_t + 0.01, d = magnet_hole_d);
      translate([0, 0, -0.01]) cylinder(h = magnet_lead + 0.01, d1 = magnet_hole_d + 2 * magnet_lead, d2 = magnet_hole_d);
    }
    if (pocket) finger_pocket();
    channel();
  }
  bottom_step();
}

// The seat and rim alone, laid on the seat's back, for a quick fit print.
module seat_test(pocket = true)
  translate([0, 0, y_seat + test_floor]) rotate([-90, 0, 0]) intersection() {
    rotate([t, 0, 0]) translate([0, 0, -cz]) dock(pocket);
    slab_y(y_seat - h_rim - 5, y_seat + test_floor) circle(R + 20);
  }

if (seat_only) seat_test();
else {
  dock();
  %pose() { m5_ghost(); m5_keepouts(); m5_usb_plug(); }
}
