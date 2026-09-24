#!/usr/bin/env python3
"""Render a deterministic round-screen visual acceptance reference as SVG."""

from __future__ import annotations

import argparse
import html
import json
from pathlib import Path


def text(value) -> str:
    return html.escape(str(value))


def action_label(value: str) -> str:
    labels = {
        "start_stop": "start/stop", "lap_click_reset_hold": "lap / hold reset",
        "key_down_up": "key", "clear_click_speed_hold": "clear / speed",
        "ptt_hold": "push to talk", "next_channel": "next channel",
        "record_hold": "record", "browse_next": "browse",
        "answer_true": "true", "answer_false": "false",
    }
    return labels.get(value, value.replace("_", " "))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("contract", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    contract = json.loads(args.contract.read_text())
    visual = contract["visual"]
    accent = visual.get("accent", "#ff5b04")
    lines = visual.get("lines", [])[:4]
    kind = visual.get("kind", "card")
    decorations = ""
    if kind == "meter":
        decorations = f'<rect x="92" y="222" width="284" height="18" rx="9" fill="#303038"/><rect x="92" y="222" width="{int(284 * visual.get("level", .62))}" height="18" rx="9" fill="{accent}"/>'
    elif kind == "radial":
        decorations = ''.join(f'<line x1="234" y1="233" x2="234" y2="{54 + i * 10}" stroke="{accent}" stroke-width="8" transform="rotate({i * 30} 234 233)" opacity="{.16 + i * .018}"/>' for i in range(12))
    elif kind == "list":
        decorations = ''.join(f'<rect x="82" y="{170 + i * 52}" width="304" height="42" rx="14" fill="#292930" stroke="{accent if i == 0 else "#454550"}" stroke-width="2"/>' for i in range(min(4, max(1, len(lines)))))
    elif kind == "pads":
        pads = visual.get("pads", [])[:4]
        positions = ((84, 88), (252, 88), (84, 246), (252, 246))
        status_colors = {"empty": "#555560", "stored": "#ffffff", "replacing": "#ff3b30"}
        decorations = ''.join(
            f'<rect x="{x}" y="{y}" width="132" height="118" rx="20" fill="{pad.get("color", accent)}"/>'
            f'<rect x="{x + 8}" y="{y + 8}" width="116" height="102" rx="15" fill="#fff" opacity=".08"/>'
            f'<text x="{x + 66}" y="{y + 137}" class="padlabel">{text(pad.get("label", f"PAD {i + 1}"))}</text>'
            + (f'<circle cx="{x + 119}" cy="{y + 132}" r="4" fill="{status_colors.get(pad.get("status"), "#555560")}"/>' if pad.get("status") else '')
            for i, (pad, (x, y)) in enumerate(zip(pads, positions))
        )
    line_svg = ''.join(f'<text x="234" y="{270 + i * 34}" class="line">{text(line)}</text>' for i, line in enumerate(lines))
    if kind == "list":
        line_svg = ''.join(f'<text x="104" y="{198 + i * 52}" class="listline" text-anchor="start">{text(line)}</text>' for i, line in enumerate(lines))
    controls = contract["controls"]
    if controls["profile"] == "c25k":
        footer_one = "A previous   ·   B next   ·   Both select"
        footer_two = "Hold both: back"
    else:
        footer_one = f"A {action_label(controls['left'])}   ·   B {action_label(controls['right'])}"
        footer_two = f"Both {action_label(controls['enter'])}   ·   Hold {action_label(controls['back'])}"
    if kind == "pads":
        footer_one = visual.get("footer", f"A  ◀     {visual.get('page', '1 / 2')}     ▶  B")
        footer_two = visual.get("footer_two", "")
        line_svg = ""
        header = f'''<text x="234" y="38" class="eyebrow">{text(visual.get("eyebrow", contract["title"].upper()))}</text>
<text x="234" y="65" class="padpage">{text(visual.get("title", "PAGE 1"))}</text>'''
    else:
        header = f'''<text x="234" y="54" class="eyebrow">{text(visual.get("eyebrow", contract["title"].upper()))}</text>
<text x="234" y="96" class="title">{text(visual.get("title", contract["title"]))}</text>
<text x="234" y="158" class="metric">{text(visual.get("metric", "READY"))}</text>'''
    svg = f'''<svg xmlns="http://www.w3.org/2000/svg" width="468" height="466" viewBox="0 0 468 466">
<rect width="468" height="466" fill="#000"/>
<circle cx="234" cy="233" r="232" fill="#101014"/>
<circle cx="234" cy="233" r="226" fill="none" stroke="#24242c" stroke-width="2"/>
<style>
text {{ font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif; fill: #fff; text-anchor: middle; }}
.eyebrow {{ font-size: 15px; font-weight: 700; letter-spacing: 2px; fill: {accent}; }}
.title {{ font-size: 30px; font-weight: 650; }}
.metric {{ font-size: 58px; font-weight: 750; }}
.line {{ font-size: 21px; }} .listline {{ font-size: 18px; text-anchor: start; }}
.padlabel {{ font-size: 13px; font-weight: 700; letter-spacing: 1px; }}
.padpage {{ font-size: 14px; fill: #a7a7b0; }}
.footer {{ font-size: 13px; fill: #c8c8d0; }}
</style>
{header}
{decorations}
{line_svg}
<text x="234" y="426" class="footer">{text(footer_one)}</text>
<text x="234" y="444" class="footer">{text(footer_two)}</text>
</svg>'''
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(svg)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
