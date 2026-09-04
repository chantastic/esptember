---
title: Styled Text
---


Day 01 put text on the screen.
Today we make it ours.

Color, size, typeface — one rung at a time, one flash per rung.
The final firmware is the top of the ladder:

```c
LV_FONT_DECLARE(montserrat_bi_48);

bsp_display_lock(0);
// True black — on an AMOLED the background is off, not painted.
lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), 0);

lv_obj_t *label = lv_label_create(lv_screen_active());
lv_label_set_text(label, "Hello, ESPtember!");
// Color: one line.
lv_obj_set_style_text_color(label, lv_color_hex(0xff5b04), 0);
// Typeface: the converted font above. Built-ins stop at regular.
lv_obj_set_style_text_font(label, &montserrat_bi_48, 0);
// 48px type doesn't fit a 368px panel on one line — typography
// is layout. Wrap at 90% width, center the lines.
lv_obj_set_width(label, lv_pct(90));
lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
lv_obj_center(label);
bsp_display_unlock();
```

## Rung 1: color

One line.

```diff
  lv_label_set_text(label, "Hello, ESPtember!");
+ lv_obj_set_style_text_color(label, lv_color_hex(0xff5b04), 0);
```

ESPtember orange on true black.
LVGL styles are calls on objects — no stylesheet, no cascade.

## Rung 2: size

Also one line — plus a flag.

```diff
+ lv_obj_set_style_text_font(label, &lv_font_montserrat_48, 0);
```

```diff
  # sdkconfig.defaults
+ CONFIG_LV_FONT_MONTSERRAT_48=y
```

The flag is the interesting part.
A microcontroller has no font system — fonts compile into the binary, and the built-in 48px Montserrat costs 78 KB of flash.

But 48px type doesn't fit a 368px panel.
Two more lines wrap and center it:

```diff
+ lv_obj_set_width(label, lv_pct(90));
+ lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
```

Typography is layout.

## Rung 3: typeface

The built-ins are Montserrat, regular weight, sizes 8 through 48.
Bold italic isn't on the menu.
Nothing else is either.

So we compile our own:

```sh
npx lv_font_conv --font Montserrat-BoldItalic.ttf \
  --size 48 --bpp 4 --range 0x20-0x7E \
  --format lvgl --lv-include lvgl.h --no-compress \
  -o montserrat_bi_48.c
```

The TTF becomes a C file.
Add it to the build, declare it, point the label at it:

```diff
+ LV_FONT_DECLARE(montserrat_bi_48);

- lv_obj_set_style_text_font(label, &lv_font_montserrat_48, 0);
+ lv_obj_set_style_text_font(label, &montserrat_bi_48, 0);
```

ASCII-only at 48px, bold italic: 46 KB.
You ship exactly the glyphs you use, at exactly the sizes you use.

## The blank screen

Our first conversion left out `--no-compress`.
The font compiled, linked, and drew.
The screen showed nothing.

`lv_font_conv` compresses glyphs by default, but LVGL ships with its font decompressor turned off — and a compressed glyph with no decompressor renders as nothing at all.
No error.
No warning.
Orange nothing on black nothing.

Pass `--no-compress`, or enable `LV_USE_FONT_COMPRESSED`.
Pick one, or render nothing.

## What we learned

Styles are calls, not stylesheets.
No cascade, no inheritance fights — the label is exactly what you set on it.

Fonts aren't installed.
They're compiled — every glyph, every size, paid for in flash.
The `--range` flag is your budget.

A big font isn't a style, it's a negotiation.
The panel is 368 pixels wide and doesn't care what you meant.

And the failure was silent, again.
Day 01: `ESP_OK` while the panel sat in reset.
Day 02: a font that compiled clean and drew nothing.
Watch the screen, not the return codes.
