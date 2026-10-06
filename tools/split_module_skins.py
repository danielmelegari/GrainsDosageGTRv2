#!/usr/bin/env python3
"""Split the combined GrainsDosage skin into per-module PNGs.

Usage:
    python tools/split_module_skins.py

It reads:
    assets/GrainsDosage-skin.png   (factory background, 2048x1520 pixels)

It writes (1x logical canvas coordinates, 1320x1360):
    assets/modules/granulizer.png
    assets/modules/preslicer.png
    assets/modules/beatrepeater.png
    assets/modules/modulation.png
    assets/modules/morph.png
    assets/modules/reslice.png
    assets/modules/gater.png
    assets/modules/filter.png
    assets/modules/reverb.png
    assets/modules/filterseq.png
    assets/modules/masterout.png

and the @2x retina siblings:
    assets/modules/@2x/<name>.png  (same crops at 2x pixel size)

The module table below is the single generator-side mirror of
src/gui/modules_loader.h (kModuleImages). Keep both in sync; the plugin
reads positions from the C++ header, this tool only produces the artwork.

Per-module panel art: the shipped factory background is a photographic
nebula plate — the visible dark panels with filigree borders are drawn by
the editor at runtime (panel() in editor_mac.mm / editor_win.cpp, palette
in src/skin_spec.h). To make each PNG a faithful standalone representation
of its module, the tool composites the exact same vector panel style on top
of the nebula crop. Disable with --no-panel to get raw crops instead.
"""

import argparse
import sys
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "assets" / "GrainsDosage-skin.png"
OUT = ROOT / "assets" / "modules"

# name -> (canvasX, canvasY, width, height) in logical 1320x1360 units.
# MUST match kModuleImages in src/gui/modules_loader.h.
MODULES = {
    "granulizer": (16, 98, 416, 404),
    "preslicer": (452, 98, 416, 404),
    "beatrepeater": (888, 98, 416, 404),
    "modulation": (16, 516, 836, 236),
    "morph": (864, 516, 440, 236),
    "reslice": (16, 766, 1288, 108),
    "gater": (16, 886, 1288, 108),
    "filter": (16, 1006, 540, 136),
    "reverb": (568, 1006, 736, 136),
    "filterseq": (16, 1154, 1288, 128),
    "masterout": (16, 1294, 1288, 56),
}

SKIN_SPEC = ROOT / "src" / "skin_spec.h"


def load_palette():
    """Parse the live 'astral' palette straight from skin_spec.h."""
    import re
    txt = SKIN_SPEC.read_text()
    pal = {}
    for name, r, g, b in re.findall(
            r"Rgb\s+(\w+)\s*\{\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}", txt):
        pal[name] = (int(r), int(g), int(b))
    return pal


def mix(a, b, t):
    return tuple(round(x * (1 - t) + y * t) for x, y in zip(a, b))


def draw_panel(d, rect, scale, P):
    """Replicate the editor's panel() styling (macOS `panel:` / Win32 `panel`):
    outer rounded plate with filigree border, inner engraved border, top
    filigree divider and two dim vine strokes down the sides."""
    x, y, w, h = [v * scale for v in rect]
    radius = max(2, round(12 * scale))
    panel = P["panel"]
    filigree = P["filigree"]
    filigree_dim = P["filigreeDim"]
    border_dark = P["borderDark"]
    lw = max(1, round(scale))
    # Outer plate: panel fill, bright filigree border.
    d.rounded_rectangle([x, y, x + w, y + h], radius=radius, fill=panel,
                        outline=filigree, width=lw)
    # Inner engraving: same fill, dark border (mirrors box(inset, clear, borderDark)).
    ins = 3 * scale
    d.rounded_rectangle([x + ins, y + ins, x + w - ins, y + h - ins],
                        radius=max(1, radius - 2), fill=None,
                        outline=border_dark, width=lw)
    # Top divider line between the rounded corners.
    inset_x = 15 * scale
    d.line([(x + inset_x, y + 5 * scale), (x + w - inset_x, y + 5 * scale)],
           fill=filigree, width=lw)
    # Side vines: gentle S-curves approximated by quadratic segments.
    for side in (0, 1):
        vx = x + w - 5 * scale if side else x + 5 * scale
        lean = -2 * scale if side else 2 * scale
        p0 = (vx, y + 14 * scale)
        p1 = (vx + lean, y + h * 0.33)
        p2 = (vx - lean, y + h * 0.66)
        p3 = (vx, y + h - 14 * scale)
        d.line(bezier(p0, p1, p2, p3), fill=filigree_dim, width=lw)


def bezier(p0, p1, p2, p3, steps=40):
    """Cubic-ish path through control points (quadratic chain approximation)."""
    pts = []
    for i in range(steps + 1):
        t = i / steps
        mt = 1 - t
        # de Casteljau for cubic with implicit mid controls
        x = (mt**3 * p0[0] + 3 * mt**2 * t * p1[0] + 3 * mt * t**2 * p2[0]
             + t**3 * p3[0])
        y = (mt**3 * p0[1] + 3 * mt**2 * t * p1[1] + 3 * mt * t**2 * p2[1]
             + t**3 * p3[1])
        pts.append((x, y))
    return pts


def crop_module(img: Image.Image, name: str, rect, scale: int, out_dir: Path,
                palette, panel_art: bool):
    x, y, w, h = rect
    crop = img.crop((x * scale, y * scale, (x + w) * scale, (y + h) * scale))
    crop = crop.convert("RGBA")
    if panel_art:
        d = ImageDraw.Draw(crop, "RGBA")
        draw_panel(d, (0, 0, w, h), scale, palette)
    out_dir.mkdir(parents=True, exist_ok=True)
    out_path = out_dir / f"{name}.png"
    crop.save(out_path)
    print(f"Saved {out_path} ({crop.size[0]}x{crop.size[1]})")


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--src", default=str(SRC), help="factory background PNG")
    ap.add_argument("--out", default=str(OUT), help="output modules folder")
    ap.add_argument("--no-panel", action="store_true",
                    help="raw nebula crops without the vector panel overlay")
    args = ap.parse_args(argv)

    src = Path(args.src)
    out = Path(args.out)
    if not src.exists():
        raise SystemExit(f"Missing source skin: {src}")

    img = Image.open(src).convert("RGBA")
    pw, ph = img.size
    if (pw, ph) != (2048, 1520):
        print(f"warning: unexpected source size {pw}x{ph}, expected 2048x1520",
              file=sys.stderr)

    palette = load_palette()
    panel_art = not args.no_panel

    for name, rect in MODULES.items():
        crop_module(img, name, rect, 1, out, palette, panel_art)
        crop_module(img, name, rect, 2, out / "@2x", palette, panel_art)

    print("Done.")


if __name__ == "__main__":
    main()
