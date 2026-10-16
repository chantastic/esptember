// M5Stack Stopwatch desk stand — parametric OpenSCAD source.
//
// The tripod mount brought back to the desk, after the Harness reference
// stand, but with the Stopwatch sitting on top rather than built in: a
// short, thick stem that stands on the desk and opens, in one vortex bloom,
// into a flat top that the device's four back magnets cling to. The top is
// slightly inset from the device's edge and leans back 26° from horizontal,
// the Harness's angle.
//
// It's built round a right-angle USB-C cable (MEOU, Amazon B0DQ8B8PFR): a
// flat head on the 6 o'clock port, whose flat ribbon runs straight back
// from the plug, drops past the stand's front, curls onto the desk and runs
// back through a filleted channel under the base. The stand is as low as
// that path allows. A nose at 6 o'clock, after the notch in the Harness's
// front, holds the head where it sits when plugged in: the cable stays
// parked when the device is off, and the device slides down the top onto
// the plug to dock. The flat run is long enough that the collar, where it
// joins the braided cable, lands well behind the stand.
//
// World frame: the desk is z = 0 and the stem's axis is the z axis. The
// viewer is at -y. The face leans back, so 12 o'clock is up and away (+y).
// Units are millimetres.
//
// Render:  openscad --backend=manifold --export-format binstl -o stopwatch-desk-stand.stl stopwatch-desk-stand.scad
// Print:   as it stands, on its base. No supports: the bloom stays within
//          45° of vertical all the way to the top's rim.

use <../lib/m5-stopwatch.scad>

/* [Top] */
face_tilt = 26;      // the top's lean from horizontal; the Harness reference measures 25.8°
face_lift = 0;       // extra height over the lowest the cable allows (see cable_face_z())
plate_d = 48;        // inside the back's Ø51.35 edge, where its R2 round-over starts, by about 1.7 mm
rim_h = 2;           // the rim's band below the top, before the bloom turns in
rim_round = 0.8;     // round-over where the rim meets the top
head_shift = 0;      // moves the top toward 12 o'clock (+y), against the stem's axis

/* [Magnets] */
magnet_pitch = 25;   // proven by the dock's seat test
magnet_hole_d = 6.2; // proven press fit
magnet_hole_t = 2.4; // proven depth: a 6 × 2 magnet sits 2 draft layers below the top
magnet_lead = 0.3;   // lead-in at the top

/* [Speaker] */
speaker_relief = 1.5;  // depth of a pocket under the device's speaker recess, following its outline out through
                       // the rim at 12 o'clock, so the speaker keeps the air gap m5_keepouts() asks for (0: flat)
speaker_grow = 0.5;    // the pocket's outline, grown from the recess's
speaker_round = 1;     // round-over where the pocket meets the top

/* [Stem] */
stem_r = 19;         // the stem's radius where it stands on the desk (Ø38)
base_chamfer = 0.6;  // 45° chamfer round the base, against elephant's foot
head_pull = 0.35;    // how long the bloom's curve holds the stem's line before turning out (fraction of its length)
rim_pull = 0.35;     // how long it holds the rim's line coming in

/* [USB-C cable] MEOU right-angle, B0DQ8B8PFR. The listing gives only the head's 5 mm thickness: */
/* every value marked "est." is read off its photos, so measure the real cable and correct them. */
usb_head_t = 5;      // the head's thickness, along the plug: from the listing
usb_head_w = 11;     // est.: its width, along the port's wide axis
usb_head_len = 16;   // est.: its length, from its rounded end past the plug to where the ribbon leaves
usb_plug_from_end = 5.5;  // est.: from that rounded end to the plug's centre
ribbon_w = 7;        // est.: the flat ribbon's width
ribbon_t = 2;        // est.: its thickness
ribbon_stiff = 2;    // est.: straight, stiff length out of the head before it can bend
ribbon_bend_r = 3;   // est.: the tightest inner radius it takes without fighting back
flat_len = 50;       // est.: ribbon length from the head to the collar (for the preview; the channel ignores it)
collar_len = 12;     // est.: the collar where the ribbon joins the braided cable
collar_d = 6;        // est.
braid_d = 3.5;       // est.: the braided cable

/* [Channel] */
channel_clear = 0.5; // each side of the ribbon, and over it
channel_round = 1;   // round-over on the channel's top edges
channel_fillet = 2;  // fillet where the channel opens through the stand's front and back
show_cable = false;  // add the cable, for previews and clearance tests

/* [Cable notch] the nose at 6 o'clock that holds the head */
notch = true;
notch_clear = 0.15;  // each side of the head, and under it: snug, since the walls hold it (tune after a test print)
notch_wall = 2;      // the side walls
notch_lip = 1.5;     // the front lips over the head's outer face, which take the plug's push when docking
notch_top = 13;      // the nose's top, above the stand's top along its normal (the device's face is at 15.5)
groove_clear = 0.3;  // round the ribbon in its groove; the groove is open at the front, to press the ribbon in
nose_round = 1.2;    // round-over on the nose's edges
notch_test = false;  // export only the nose and the stand's front, a quick print to check the head's fit

/* [1/4-20 heat-set insert] ZWMSSLL 1/4"-20x8x8, from the bottom. The cable channel runs under its spot, */
/* so it's off by default. */
insert = false;
insert_hole_d = 7.2; // matches the insert's smooth guide (d1 7.15)
insert_depth = 9.5;  // L 8.0 plus room for displaced plastic
insert_lead = 0.4;   // lead-in at the mouth, out to the knurl's 8.0

NA = 160;            // points round the stand
NH = 40;             // steps up the bloom
NL = 6;              // steps round the top's rim

t = face_tilt;

// The cable's path, in the plane x = 0. The plug's centre sits at the port's
// mid-height; the ribbon leaves the head's far end, centred in its
// thickness, heading down and back (-nf), runs ribbon_stiff straight, then
// bends (centreline radius rho) until it lies flat on the desk, heading
// back (+y) under the stand.
usb_f = [for (f = m5_features()) if (f[0] == "usb_c") f][0];
usb_from_back = (usb_f[3][0] + usb_f[3][1]) / 2;
r_edge = m5_r();                                     // the head stops against the device's widest edge
head_out = r_edge + usb_head_t / 2;                  // the ribbon's centre plane, out from the device's axis
head_back = usb_head_len - usb_plug_from_end;        // from the plug's centre to where the ribbon leaves
rho = ribbon_bend_r + ribbon_t / 2;
// The top's centre height that sets the ribbon down flat on the desk.
function cable_face_z() = ribbon_t / 2 + rho * (1 - sin(t)) + ribbon_stiff * cos(t)
  - (usb_from_back - head_back) * cos(t) + head_out * sin(t);
face_z = cable_face_z() + face_lift;
R = plate_d / 2;
xh = [1, 0, 0];
yp = [0, cos(t), sin(t)];                // 12 o'clock on the top
nf = [0, -sin(t), cos(t)];               // the top's normal, toward the viewer and up
fc = [0, head_shift, face_z];            // the top's centre
function u_at(a) = cos(a) * xh + sin(a) * yp;   // a = 90 is 12 o'clock, 270 is 6 o'clock

function bez(p0, p1, p2, p3, s) = let(m = 1 - s)
  m * m * m * p0 + 3 * m * m * s * p1 + 3 * m * s * s * p2 + s * s * s * p3;

// The ribbon's path points, in the top's frame of reference (world, x = 0).
rib_s = fc + (usb_from_back - head_back) * nf - head_out * yp;   // where it leaves the head
rib_a = rib_s - ribbon_stiff * nf;                               // where it starts to bend
rib_c = rib_a + rho * [0, cos(t), sin(t)];                       // the bend's centre
rib_e = [0, rib_c[1], rib_c[2] - rho];                           // flat on the desk
bend_len = rho * (90 - t) * PI / 180;
collar_y = rib_e[1] + flat_len - ribbon_stiff - bend_len;        // where the collar starts, under or behind the stand

// One line up the stand, from the desk to the top, at the top's angle a.
function line_at(a) = let(
  b = fc - rim_h * nf + R * u_at(a),                 // where the bloom meets the rim
  phi = atan2(b[1], b[0]),
  rh = [cos(phi), sin(phi), 0],
  w = stem_r * rh + [0, 0, base_chamfer],            // the stem, above its chamfer
  L = norm(b - w),
  base = [(stem_r - base_chamfer) * rh, w],
  bloom = [for (i = [1:NH]) bez(w, w + head_pull * L * [0, 0, 1], b - rim_pull * L * nf, b, i / NH)],
  band = [b + (rim_h - rim_round) * nf],
  edge = [for (i = [1:NL]) let(q = 90 * i / NL)
            fc + (R - rim_round + rim_round * cos(q)) * u_at(a) - rim_round * (1 - sin(q)) * nf])
  concat(base, bloom, band, edge);

module body() {
  lines = [for (k = [0:NA - 1]) line_at(360 * k / NA)];
  nr = len(lines[0]);
  pts = [for (j = [0:nr - 1], k = [0:NA - 1]) lines[k][j]];
  side = [for (j = [0:nr - 2], k = [0:NA - 1])
    let(a = j * NA + k, b = j * NA + (k + 1) % NA) [a, a + NA, b + NA, b]];
  polyhedron(pts, concat(side,
    [[for (k = [0:NA - 1]) k]],                          // the bottom, on the desk
    [[for (k = [NA - 1:-1:0]) (nr - 1) * NA + k]]));     // the top
}

// The top's own frame: its centre at the origin, the top facing +z, 12 o'clock at +y.
module on_top() translate(fc) rotate([t, 0, 0]) children();

module magnet_pockets() on_top() for (m = m5_magnets(magnet_pitch)) translate([m[0], m[1], 0]) {
  translate([0, 0, -magnet_hole_t]) cylinder(h = magnet_hole_t + 0.01, d = magnet_hole_d, $fn = 64);
  translate([0, 0, -magnet_lead]) cylinder(h = magnet_lead + 0.01, d1 = magnet_hole_d, d2 = magnet_hole_d + 2 * magnet_lead, $fn = 64);
}

// Under the speaker recess, open out through the rim, with its top edge
// rounded over: slices of the outline, grown by a quarter round toward the
// top and hulled in pairs (the outline is convex, so each hull is exact).
spk_r = min(speaker_round, speaker_relief);
function spk_grow(d) = spk_r - sqrt(max(0, spk_r * spk_r - (spk_r - d) * (spk_r - d)));   // at depth d below the top
module spk_slice(d) translate([0, 0, -d]) linear_extrude(0.01) offset(r = spk_grow(d), $fn = 32)
  m5_speaker_recess_2d(grow = speaker_grow);

module speaker_pocket() on_top() {
  translate([0, 0, -speaker_relief]) linear_extrude(speaker_relief + 0.01) m5_speaker_recess_2d(grow = speaker_grow);
  n = 10;
  for (i = [0:n - 1]) hull() { spk_slice(spk_r * i / n); spk_slice(spk_r * (i + 1) / n); }
  hull() { spk_slice(0); translate([0, 0, 0.5]) spk_slice(0); }        // clear above the top
}

// A loft along y of a U section (open to the desk), for the channel.
// sec(y) gives [half-width, height, top corner radius] at y.
module y_loft(y0, y1, n, sec) {
  NC = 8;
  ring = function(y) let(s = sec(y), hw = s[0], h = s[1], rc = min(s[2], hw, h)) concat(
    [[-hw, y, -1]],
    [for (i = [0:NC]) let(q = 90 * i / NC) [-hw + rc - rc * cos(q), y, h - rc + rc * sin(q)]],
    [for (i = [0:NC]) let(q = 90 * i / NC) [hw - rc + rc * sin(q), y, h - rc + rc * cos(q)]],
    [[hw, y, -1]]);
  rings = [for (i = [0:n]) ring(y0 + (y1 - y0) * i / n)];
  m = len(rings[0]);
  pts = [for (r = rings) each r];
  side = [for (i = [0:n - 1], k = [0:m - 1])
    let(a = i * m + k, b = i * m + (k + 1) % m) [a, a + m, b + m, b]];
  polyhedron(pts, concat(side, [[for (k = [0:m - 1]) k]], [[for (k = [m - 1:-1:0]) n * m + k]]));
}

// Fillet growth at a distance d inside a mouth: a quarter round of radius r.
function fillet_grow(d, r) = d <= 0 ? r : d >= r ? 0 : r - sqrt(r * r - (r - d) * (r - d));

// The channel under the base: the ribbon's section plus clearance, rounded
// along its top edges, and filleted where it opens through the stand's back
// (and its front, when there's no nose; with one, the groove feeds it).
module cable_channel() {
  y0 = notch ? rib_e[1] : rib_e[1] - 3;
  y1 = stem_r + 3;
  rib = [ribbon_w / 2 + channel_clear, ribbon_t + channel_clear, channel_round];
  sec = function(y) let(g = max(notch ? 0 : fillet_grow(y + stem_r, channel_fillet), fillet_grow(stem_r - y, channel_fillet)))
    rib + [g, g, g];
  y_loft(y0, y1, round((y1 - y0) / 0.4), sec);
}

// --- The nose and its notch ---------------------------------------------
// The port frame at 6 o'clock: u across (x), w out from the device's axis
// along -yp, v up the top's normal from the top.
function pf(u, w, v) = fc + u * xh - w * yp + v * nf;
head_v_top = usb_from_back + usb_plug_from_end;
head_v_bot = usb_from_back - head_back;
head_w_out = r_edge + usb_head_t;
nose_hw = usb_head_w / 2 + notch_clear + notch_wall;
nose_front = head_w_out + notch_clear + notch_lip;             // w of the nose's front face
groove_hw = ribbon_w / 2 + groove_clear;

// A rounded rectangle in section coordinates (u, n), corners of radius r.
function rrect(hw, n0, n1, r) = let(r2 = min(r, hw, (n1 - n0) / 2)) [
  for (c = [[hw - r2, n1 - r2, 0], [-hw + r2, n1 - r2, 90], [-hw + r2, n0 + r2, 180], [hw - r2, n0 + r2, 270]])
    for (i = [0:4]) let(q = c[2] + 90 * i / 4) [c[0] + r2 * cos(q), c[1] + r2 * sin(q)]];
// A thin prism of a section at p, with u along xh, n along N, thickness along T.
module sec_prism(p, N, T, sec) {
  m = len(sec);
  pts = concat([for (q = sec) p + q[0] * xh + q[1] * N], [for (q = sec) p + q[0] * xh + q[1] * N + 0.01 * T]);
  polyhedron(pts, concat([[for (i = [m - 1:-1:0]) i]], [[for (i = [0:m - 1]) m + i]],
    [for (i = [0:m - 1]) [i, (i + 1) % m, m + (i + 1) % m, m + i]]));
}
// The ribbon's path from the head down, round the bend and onto the desk:
// [point, outward normal, tangent, fraction along].
rib_path = concat(
  [for (i = [0:3]) [rib_s + ribbon_stiff * i / 3 * (-nf), -yp, -nf]],
  [for (i = [1:12]) let(q = (90 - t) * i / 12)
    [rib_c + rho * [0, -cos(t + q), -sin(t + q)], [0, -cos(t + q), -sin(t + q)], [0, sin(t + q), -cos(t + q)]]],
  [[rib_e + [0, 1.5, 0], [0, 0, -1], [0, 1, 0]]]);
function ease(i, n) = smooth01(i / n);
function smooth01(u) = u <= 0 ? 0 : u >= 1 ? 1 : (1 - cos(180 * u)) / 2;

module nose() {
  sec_at = function(v, n0) rrect(nose_hw, n0, nose_front, nose_round);
  // above the top: beside the device's edge, never inside it
  hull() for (v = [0, notch_top - nose_round])
    sec_prism(pf(0, 0, v), -yp, nf, rrect(nose_hw, r_edge + 0.05, nose_front, nose_round));
  hull() {
    sec_prism(pf(0, 0, notch_top - nose_round), -yp, nf, rrect(nose_hw, r_edge + 0.05, nose_front, nose_round));
    sec_prism(pf(0, 0, notch_top), -yp, nf, rrect(nose_hw - nose_round, r_edge + 0.05 + nose_round, nose_front - nose_round, nose_round));
  }
  // below the top: back into the stand, so it hooks the head's foot and joins the rim
  hull() for (v = [head_v_bot - 2, 0])
    sec_prism(pf(0, 0, v), -yp, nf, rrect(nose_hw, r_edge - 8, nose_front, nose_round));
  // down along the ribbon to the desk, narrowing from the head's walls to the ribbon's
  n = len(rib_path);
  for (i = [0:n - 2]) hull() for (j = [i, i + 1]) let(
    e = ease(j, 6),
    hw = nose_hw + e * (groove_hw + notch_wall - nose_hw),
    out = (nose_front - head_out) + e * (ribbon_t / 2 + 0.4 - (nose_front - head_out)))
    sec_prism(rib_path[j][0], rib_path[j][1], rib_path[j][2], rrect(hw, -8, out, nose_round));
}

// The head's pocket (open toward the device and at the top), the front
// window between the lips, and the ribbon's groove, open at the front.
module notch_cut() {
  translate(pf(0, 0, 0)) multmatrix([[xh[0], -yp[0], nf[0], 0], [xh[1], -yp[1], nf[1], 0], [xh[2], -yp[2], nf[2], 0], [0, 0, 0, 1]]) {
    // pocket: u across, w out, v up (in this frame z = v, y = w)
    translate([-(usb_head_w / 2 + notch_clear), r_edge - notch_clear, head_v_bot - notch_clear])
      cube([usb_head_w + 2 * notch_clear, usb_head_t + 2 * notch_clear, 40]);
    translate([-groove_hw, r_edge + 1, head_v_bot - notch_clear]) cube([2 * groove_hw, 30, 40]);
  }
  n = len(rib_path);
  for (i = [0:n - 2]) hull() for (j = [i, i + 1])
    sec_prism(rib_path[j][0], rib_path[j][1], rib_path[j][2],
      [[-groove_hw, -ribbon_t / 2 - groove_clear], [groove_hw, -ribbon_t / 2 - groove_clear], [groove_hw, 12], [-groove_hw, 12]]);
}

// The cable, for previews and clearance tests: the head, the ribbon down,
// round the bend and back under the stand, the collar and a stub of braid.
module cable() {
  pc = fc + usb_from_back * nf - r_edge * yp;          // the plug's centre, at the port
  // the head: a pill along nf, usb_head_t thick outward from the edge, the plug in its rounded end
  hull() for (e = [usb_plug_from_end - usb_head_w / 2, -head_back + usb_head_w / 2])
    translate(pc + e * nf - usb_head_t / 2 * yp) rotate([t - 90, 0, 0])
      cylinder(h = usb_head_t, d = usb_head_w, center = true, $fn = 32);
  // the ribbon: its section along its path, then flat on the desk to the collar
  rib = [[-ribbon_w / 2, -ribbon_t / 2], [ribbon_w / 2, -ribbon_t / 2], [ribbon_w / 2, ribbon_t / 2], [-ribbon_w / 2, ribbon_t / 2]];
  path = concat(rib_path, [[[0, collar_y, rib_e[2]], [0, 0, -1], [0, 1, 0]]]);
  for (i = [0:len(path) - 2]) hull() for (j = [i, i + 1])
    sec_prism(path[j][0], path[j][1], path[j][2], rib);
  // the collar and the braid
  translate([0, collar_y, collar_d / 2]) rotate([-90, 0, 0]) {
    cylinder(h = collar_len, d = collar_d, $fn = 32);
    cylinder(h = collar_len + 20, d = braid_d, $fn = 24);
  }
}

module insert_hole() {
  tip = insert_hole_d / 2;    // a 90° cone at the top, so its roof prints
  translate([0, 0, -0.01]) cylinder(h = insert_lead + 0.01, d1 = insert_hole_d + 2 * insert_lead, d2 = insert_hole_d, $fn = 64);
  translate([0, 0, -0.01]) cylinder(h = insert_depth + 0.01, d = insert_hole_d, $fn = 64);
  translate([0, 0, insert_depth]) cylinder(h = tip, r1 = insert_hole_d / 2, r2 = 0, $fn = 64);
}

module stand() difference() {
  intersection() {
    union() {
      body();
      if (notch) nose();
    }
    translate([-100, -100, 0]) cube(200);              // nothing below the desk
  }
  if (notch) notch_cut();
  magnet_pockets();
  if (speaker_relief > 0) speaker_pocket();
  if (insert) insert_hole();
  cable_channel();
}

if (notch_test) intersection() {
  stand();
  translate([-14, -40, 0]) cube([28, 22, 40]);       // the nose, the groove's bend and a sliver of the stand's front
}
else stand();
if (show_cable) %cable();
