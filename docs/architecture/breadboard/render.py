"""Breadboard-style wiring diagram (Fritzing-like) from breadboard.yml.

Usage:  python docs/architecture/breadboard/render.py
Output: docs/architecture/breadboard/breadboard.svg

Layout model (all units px, 6 px ~ 1 mm, header pitch 15.24 px = 2.54 mm):
  parts : boards drawn at real size with pin headers in real order
  rails : vertical bus strips (power / I2C) - any wire may land on any height
  wires : orthogonal routes  pin -> stub -> via points -> stub -> pin/rail
"""
import os
import sys
import yaml
from xml.sax.saxutils import escape

HERE = os.path.dirname(os.path.abspath(__file__))
PITCH = 15.24
NORMAL = {"left": (-1, 0), "right": (1, 0), "top": (0, -1), "bottom": (0, 1)}


def load(path):
    with open(path, encoding="utf-8") as f:
        return yaml.safe_load(f)


def pin_positions(part):
    """Return {pin_name: (x, y, side)} for one part."""
    out = {}
    x0, y0, w, h = part["x"], part["y"], part["w"], part["h"]
    for hd in part.get("headers", []):
        side = hd["side"]
        pins = hd["pins"]
        pitch = hd.get("pitch", PITCH)
        inset = hd.get("inset", 9)
        span = (len(pins) - 1) * pitch
        length = h if side in ("left", "right") else w
        start = hd.get("start", (length - span) / 2)
        for i, name in enumerate(pins):
            if name is None:
                continue
            d = start + i * pitch
            if side == "left":
                p = (x0 + inset, y0 + d)
            elif side == "right":
                p = (x0 + w - inset, y0 + d)
            elif side == "top":
                p = (x0 + d, y0 + inset)
            else:
                p = (x0 + d, y0 + h - inset)
            out[str(name)] = (p[0], p[1], side, hd.get("label_inside", True))
    return out


def route(p1, side1, via, end, side2, stub1, stub2, end_is_rail):
    n1 = NORMAL[side1]
    pts = [(p1[0], p1[1]), (p1[0] + n1[0] * stub1, p1[1] + n1[1] * stub1)]
    for v in via or []:
        last = pts[-1]
        if "x" in v:
            pts.append((v["x"], last[1]))
        else:
            pts.append((last[0], v["y"]))
    last = pts[-1]
    if end_is_rail:
        pts.append((end[0], last[1]))  # rails accept the wire at its current height
        return pts
    n2 = NORMAL[side2]
    q2 = (end[0] + n2[0] * stub2, end[1] + n2[1] * stub2)
    if last[0] != q2[0] and last[1] != q2[1]:
        if side2 in ("left", "right"):
            pts.append((last[0], q2[1]))
        else:
            pts.append((q2[0], last[1]))
    pts.append(q2)
    pts.append((end[0], end[1]))
    return pts


def svg_part(part, pins):
    s = []
    x, y, w, h = part["x"], part["y"], part["w"], part["h"]
    color = part.get("color", "#2b2b2b")
    s.append(f'<g class="part">')
    s.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="6" fill="{color}" stroke="#111" stroke-width="1.5"/>')
    for d in part.get("decor", []):
        kind = d["kind"]
        if kind == "rect":
            s.append(f'<rect x="{x + d["x"]}" y="{y + d["y"]}" width="{d["w"]}" height="{d["h"]}" rx="{d.get("rx", 2)}" fill="{d["fill"]}" stroke="{d.get("stroke", "none")}"/>')
        elif kind == "circle":
            s.append(f'<circle cx="{x + d["x"]}" cy="{y + d["y"]}" r="{d["r"]}" fill="{d["fill"]}" stroke="{d.get("stroke", "none")}" stroke-width="2"/>')
        elif kind == "text":
            s.append(f'<text x="{x + d["x"]}" y="{y + d["y"]}" class="silk" text-anchor="{d.get("anchor", "middle")}" font-size="{d.get("size", 9)}">{escape(d["text"])}</text>')
    # title
    tx = x + w / 2
    ty = y + part.get("title_y", h / 2)
    s.append(f'<text x="{tx}" y="{ty}" class="title" text-anchor="middle">{escape(part["title"])}</text>')
    if part.get("subtitle"):
        s.append(f'<text x="{tx}" y="{ty + 13}" class="sub" text-anchor="middle">{escape(part["subtitle"])}</text>')
    if part.get("warn"):
        s.append(f'<text x="{x + w / 2}" y="{y + h + 14}" class="warn" text-anchor="middle">{escape(part["warn"])}</text>')
    # pins + silkscreen names
    for name, (px, py, side, inside) in pins.items():
        name = name.split("#")[0]
        s.append(f'<rect x="{px - 4}" y="{py - 4}" width="8" height="8" fill="#d4a017" stroke="#6b5200"/>')
        if not inside:
            continue
        off = 9
        if side == "left":
            s.append(f'<text x="{px + off}" y="{py + 3}" class="silk">{escape(name)}</text>')
        elif side == "right":
            s.append(f'<text x="{px - off}" y="{py + 3}" class="silk" text-anchor="end">{escape(name)}</text>')
        elif side == "top":
            s.append(f'<text transform="translate({px + 3},{py + off}) rotate(90)" class="silk">{escape(name)}</text>')
        else:
            s.append(f'<text transform="translate({px + 3},{py - off}) rotate(-90)" class="silk">{escape(name)}</text>')
    s.append("</g>")
    return "\n".join(s)


def svg_rail(rail):
    s = []
    x, y, h = rail["x"], rail["y"], rail["h"]
    lines = rail["lines"]
    w = len(lines) * rail.get("spacing", 18) + 16
    s.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="4" fill="#f4f1e8" stroke="#b9b29c"/>')
    for i, ln in enumerate(lines):
        lx = x + 8 + rail.get("spacing", 18) * (i + 0.5)
        ln["_x"] = lx
        s.append(f'<line x1="{lx}" y1="{y + 22}" x2="{lx}" y2="{y + h - 8}" stroke="{ln["color"]}" stroke-width="2" stroke-dasharray="2 3"/>')
        s.append(f'<text transform="translate({lx + 3},{y + 6}) rotate(90)" class="rail">{escape(ln["name"])}</text>')
    s.append(f'<text x="{x + w / 2}" y="{y + h + 14}" class="sub" text-anchor="middle">{escape(rail["title"])}</text>')
    return "\n".join(s)


def label_box(x, y, text, vertical):
    tw = 6.2 * len(text) + 6
    if vertical:
        return (f'<g transform="translate({x},{y}) rotate(-90)">'
                f'<rect x="{-tw / 2}" y="-6.5" width="{tw}" height="13" rx="3" class="memo"/>'
                f'<text x="0" y="3.5" class="memo-t" text-anchor="middle">{escape(text)}</text></g>')
    return (f'<rect x="{x - tw / 2}" y="{y - 6.5}" width="{tw}" height="13" rx="3" class="memo"/>'
            f'<text x="{x}" y="{y + 3.5}" class="memo-t" text-anchor="middle">{escape(text)}</text>')


def main():
    cfg = load(os.path.join(HERE, "breadboard.yml"))
    parts = {p["id"]: p for p in cfg["parts"]}
    pins = {pid: pin_positions(p) for pid, p in parts.items()}
    rails = {r["id"]: r for r in cfg.get("rails", [])}

    body_rails = [svg_rail(r) for r in rails.values()]  # also computes line x
    rail_x = {f'{rid}.{ln["name"]}': ln["_x"] for rid, r in rails.items() for ln in r["lines"]}

    wires_svg, labels_svg, dots_svg = [], [], []
    stub_default = cfg.get("stub", 40)
    for w in cfg["wires"]:
        a_part, a_pin = w["from"].split(".", 1)
        ax, ay, aside, _ = pins[a_part][a_pin]
        stub1 = w.get("stub", parts[a_part].get("stub", stub_default))
        if w["to"] in rail_x:
            end, side2, is_rail, stub2 = (rail_x[w["to"]], None), None, True, 0
        else:
            b_part, b_pin = w["to"].split(".", 1)
            bx, by, bside, _ = pins[b_part][b_pin]
            end, side2, is_rail = (bx, by), bside, False
            stub2 = w.get("stub2", parts[b_part].get("stub", stub_default))
        pts = route((ax, ay), aside, w.get("via"), end, side2, stub1, stub2, is_rail)
        d = " ".join(f"{px:.1f},{py:.1f}" for px, py in pts)
        wires_svg.append(f'<polyline points="{d}" class="wire-o"/>')
        wires_svg.append(f'<polyline points="{d}" class="wire" stroke="{w["color"]}"/>')
        if is_rail:
            dots_svg.append(f'<circle cx="{pts[-1][0]}" cy="{pts[-1][1]}" r="4" fill="{w["color"]}" stroke="#222"/>')
        # memo label : role of the pin, on the stub next to the module
        memo = w.get("memo", a_pin)
        if memo and not parts[a_part].get("no_memo"):
            n = NORMAL[aside]
            dist = w.get("memo_at", cfg.get("memo_at", stub1 * 0.55))
            labels_svg.append(label_box(ax + n[0] * dist, ay + n[1] * dist, memo, aside in ("top", "bottom")))
        memo2 = w.get("memo2")
        if memo2 is None and not is_rail and not parts[b_part].get("no_memo"):
            memo2 = b_pin
        if memo2 and not is_rail:
            n = NORMAL[side2]
            dist = w.get("memo2_at", min(cfg.get("memo_at", 24), stub2 * 0.55) if stub2 > 20 else stub2 * 0.55)
            labels_svg.append(label_box(end[0] + n[0] * dist, end[1] + n[1] * dist, memo2, side2 in ("top", "bottom")))

    W, H = cfg["canvas"]["w"], cfg["canvas"]["h"]
    nc = cfg.get("notes", {"x": 20, "y": H - 60, "lines": []})
    notes = "".join(
        f'<text x="{nc["x"]}" y="{nc["y"] + 16 * i}" class="note">{escape(t)}</text>'
        for i, t in enumerate(nc["lines"]))
    svg = f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" width="{W}" height="{H}" font-family="Malgun Gothic, Apple SD Gothic Neo, Noto Sans KR, Arial, sans-serif">
<style>
  .title {{ fill:#fff; font-size:12px; font-weight:700; }}
  .sub {{ fill:#ddd; font-size:9.5px; }}
  .silk {{ fill:#fff; font-size:8.5px; }}
  .warn {{ fill:#c0392b; font-size:10px; font-weight:700; }}
  .rail {{ fill:#555; font-size:9px; font-weight:700; }}
  .wire {{ fill:none; stroke-width:3.2; stroke-linejoin:round; stroke-linecap:round; }}
  .wire-o {{ fill:none; stroke:#222; stroke-width:5.4; stroke-linejoin:round; stroke-linecap:round; }}
  .memo {{ fill:#fffbe0; stroke:#8a7a2a; stroke-width:0.8; }}
  .memo-t {{ fill:#222; font-size:9px; font-weight:700; }}
  .note {{ fill:#333; font-size:11px; }}
  .h1 {{ fill:#222; font-size:16px; font-weight:700; }}
</style>
<rect width="{W}" height="{H}" fill="#fafafa"/>
<text x="20" y="28" class="h1">{escape(cfg.get("title", ""))}</text>
{"".join(body_rails)}
{"".join(svg_part(p, pins[pid]) for pid, p in parts.items())}
{"".join(wires_svg)}
{"".join(dots_svg)}
{"".join(labels_svg)}
{notes}
</svg>
'''
    out = os.path.join(HERE, "breadboard.svg")
    with open(out, "w", encoding="utf-8", newline="\n") as f:
        f.write(svg)
    print("wrote", out)


if __name__ == "__main__":
    sys.exit(main())
