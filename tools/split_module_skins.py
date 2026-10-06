#!/usr/bin/env python3
"""Split the combined GrainsDosage skin into per-module PNGs.

Usage:
    python tools/split_module_skins.py

It reads:
    assets/GrainsDosage-skin.png

It writes:
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
"""

from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "assets" / "GrainsDosage-skin.png"
OUT = ROOT / "assets" / "modules"

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


def crop_module(src: Path, out_dir: Path, name: str, x: int, y: int, w: int, h: int):
    img = Image.open(src)
    crop = img.crop((x, y, x + w, y + h))
    out_path = out_dir / f"{name}.png"
    crop.save(out_path)
    print(f"Saved {out_path}")


def main():
    if not SRC.exists():
        raise FileNotFoundError(f"Missing source skin: {SRC}")

    OUT.mkdir(parents=True, exist_ok=True)

    for name, (x, y, w, h) in MODULES.items():
        crop_module(SRC, OUT, name, x, y, w, h)

    print("Done.")


if __name__ == "__main__":
    main()
