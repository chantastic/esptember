# Stopwatch MagSafe adapter

A thin disc that puts the Stopwatch on the back of a MagSafe iPhone. It's
2.44 mm thick.

- **Device side:** four 6 × 2 mm magnets on the Stopwatch's 25 mm square
  grab the device's own back magnets, using the dock's proven pitch and
  press fit.
- **Phone side:** something for the phone's MagSafe ring to grab. It sits at
  the ring's radius, about 23–27.5 mm, outside the device magnets, which
  reach 20.8 mm.

The two sets sit at different radii, so their pockets never stack. The disc
is only as thick as one magnet plus a two-layer skin.

![The adapter on a phone, with the Stopwatch ghosted below](preview.png)

| Phone side (steel ring) | Device side | Phone side (magnet ring) |
| --- | --- | --- |
| ![Phone side](phone-side.png) | ![Device side](device-side.png) | ![Magnet variant](magnets-variant.png) |

## Two ways to grip the phone

### `grip = "steel_ring"` (default, recommended): `stopwatch-magsafe-adapter.stl`

A stick-on steel MagSafe ring, 55 mm OD × 45 mm ID × 0.55 mm, sits in a
0.64 mm recess on the phone side. Steel is pulled by any magnet pattern, so
MagSafe's polarity doesn't matter. Apple doesn't publish that polarity, and I
couldn't find a reliable breakdown of it.

- **Size:** Ø58 mm, about 3 mm wider than the Stopwatch all round.
- **Part:** [KraftGeek universal metal ring sticker](https://kraftgeek.com/collections/bfcm-rec/products/universal-metal-ring-sticker)
  (55/45/0.55 mm, 2.5 g). Any 0.5 mm "metal ring for MagSafe" of about that
  size fits. Set `ring_od`, `ring_id` and `ring_t` to match.
- **Fit:** the recess allows 0.3 mm round the ring, and the ring sits
  0.09 mm below flush.
- **Rotation:** steel doesn't self-align, so the adapter can turn on the
  phone, held only by friction. The device can't turn on the adapter: its
  four magnets lock it.

### `grip = "magnets"`: `stopwatch-magsafe-adapter-magnets.stl`

Sixteen more 6 × 2 mm magnets sit in a ring at r = 25.3, all with the same
face toward the phone. Nothing extra to buy.

- **Size:** Ø59.2 mm.
- **Test first:** hold one magnet flat on the back of the phone and slide it
  round the MagSafe ring. If the same face attracts all the way round, this
  variant works with that face out. If it pulls at some angles and pushes at
  others, use the steel ring.
- **Pockets:** these open on the phone side, with a 0.32 mm skin toward the
  device, so the magnet faces touch the phone. Add a thin film or felt
  there if you don't want bare metal on the glass.

## Assembly

1. Put a dab of glue in each device-side pocket. They open on the face that
   prints on the bed. Press in the four magnets, each oriented to attract
   the Stopwatch's magnet opposite it. Let the device do the orienting:
   seat each magnet on it, then press it into the pocket.
2. Then fit the phone grip:
   - **Steel ring:** peel it and stick it into the recess on the phone side,
     adhesive down.
   - **Magnet ring:** glue the sixteen magnets in with the tested face out.
3. Snap the adapter onto the phone's MagSafe, then the Stopwatch onto the
   adapter, 12 o'clock up.

## Printing

Print it as exported, device side down, without supports or a brim, on
Optimal (0.16 mm layers). The depths sit on layer lines shared with Draft:

- **Device pockets:** 2.12 mm deep, ending on a line both profiles share.
- **Skin:** 2.12–2.44 mm, two Optimal layers or one Draft layer. It's
  bridged over each pocket, and that's the only overhang.
- **Steel ring's recess:** its floor is at 1.80 mm, an Optimal layer line.

The device side prints on the bed, so it comes out flat. Both edges are
softened: a 0.8 mm round-over on the phone side, and a 0.3 mm chamfer on the
bed side against elephant's foot.

## Parameters

| Parameter | Value | Notes |
| --- | --- | --- |
| `grip` | "steel_ring" | or "magnets" |
| `pocket_depth`, `skin` | 2.12, 0.32 | the disc is 2.44 thick |
| `magnet_pitch`, `magnet_hole_d` | 25, 6.2 | proven by the dock's seat test |
| `ring_od`, `ring_id`, `ring_t` | 55, 45, 0.55 | the steel ring |
| `ring_clear`, `ring_recess` | 0.3, 0.64 | |
| `phone_r`, `phone_n` | 25.3, 16 | the magnet ring: MagSafe's mid-radius, 9.9 mm apart |
| `rim` | 1.2 | wall outside the grip |

## Sources

- MagSafe ring sizes: ring magnets for cases are usually about 54–56 mm OD
  and 46 mm ID
  ([Medium](https://medium.com/@duanh148/what-size-is-magsafe-magnet-7b14f5d6c450),
  [Total Element](https://totalelement.com/products/56-x-46-x-1-5mm-magsafe-neodymium-rare-earth-magnet-phone-attachment-ring-assembly-4-pack)).
- On polarity, MakerWorld's DIY MagSafe rings say only to "pay attention
  to the direction" ([example](https://makerworld.com/models/2121665)).
  Apple doesn't publish it, and a vendor's claim that it's a Halbach array
  ([HSMAG](https://www.hsmagnets.com/blog/what-magsafe-magnet-on-the-iphone-12-is/))
  is unconfirmed.

## CAD evidence (2026-10-13)

- `mesh_check.py`:
  - **Steel ring:** one shell, genus 0, 9,984 triangles, 58.0 × 58.0 ×
    2.4 mm.
  - **Magnets:** one shell, genus 0, 17,280 triangles, 59.2 × 59.2 ×
    2.4 mm.

  No non-manifold edges, inconsistent windings or near-duplicate vertices.
  The only downward faces are the four bridged skins (121 mm²).

## Physical evidence

None yet.
