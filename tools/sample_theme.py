#!/usr/bin/env python3
"""Sample a GUI mockup (JPG/PNG) and emit an aztec theme() function + skin.txt.

Usage:
  python3 tools/sample_theme.py assets/NEWGUI.jpeg aurora "Aurora Lime" \
      --set bg_deep=3,2,10 --set panel=22,22,32 --set accent=156,250,110 ...

Roles are the colour keys from src/skin_spec.h (bg_deep, panel, panel_raised,
panel_inset, well, filigree, filigree_dim, border_dark, hairline, accent,
accent_bright, accent_track, accent_glow, title, cream, muted, faint, readout,
meter, meter_low, hot, meter_off, violet, release).

Colours not passed through --set fall back to the astral defaults, exactly like
makeTheme() in C++. The script prints a ready-to-paste `inline Palette <name>()`
block and writes assets/<name>-skin.txt (theme = <name> plus every override),
which skin_theme.h::loadSkinFile() consumes directly.
"""
import re, sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    Image = None

ROOT = Path(__file__).resolve().parents[1]
SPEC = ROOT / "src" / "skin_spec.h"

def base_palette():
    """Parse the constexpr Rgb defaults from skin_spec.h (single source)."""
    txt = SPEC.read_text()
    pal = {}
    for name, r, g, b in re.findall(r"constexpr Rgb\s+(\w+)\s*\{\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}", txt):
        pal[name] = (int(r), int(g), int(b))
    # role -> constexpr field spelling used by baseRgb()
    roles = {
        "bg_deep":"bgDeep","panel":"panel","panel_raised":"panelRaised","panel_inset":"panelInset","well":"well",
        "filigree":"filigree","filigree_dim":"filigreeDim","border_dark":"borderDark","hairline":"hairline",
        "accent":"green","accent_bright":"greenGlow","accent_track":"greenDim","accent_glow":"greenDeep",
        "title":"title","cream":"cream","muted":"muted","faint":"faint","readout":"readout",
        "meter":"amber","meter_low":"amberLow","hot":"hot","meter_off":"meterOff","violet":"violet","release":"release"}
    return {k: pal[v] for k, v in roles.items()}, roles

def finalize(p):
    """Mirror skin::finalize() so derived shades stay consistent."""
    mix  = lambda a,b,t: tuple(int(x+(y-x)*t+.5) for x,y in zip(a,b))
    scale= lambda a,f:   tuple(max(0,min(255,int(x*f+.5))) for x in a)
    p["accent_bright"] = mix(p["accent"],(255,255,255),.10)
    p["accent_track"]  = scale(p["accent"],.23)
    p["accent_glow"]   = scale(p["accent"],.30)
    p["meter_low"]     = scale(p["meter"],.52)
    p["meter_off"]     = mix(p["bg_deep"],p["meter"],.12)
    return p

def sample(image, role, box):
    """Median RGB of a rectangle in the mockup — the honest way to pick colours."""
    if Image is None: sys.exit("Pillow required for sampling: pip install pillow")
    im = Image.open(image).convert("RGB").crop(box)
    px = list(im.getdata())
    px.sort(key=lambda t: sum(t))
    return px[len(px)//2]

def main():
    args = sys.argv[1:]
    if len(args) < 3: sys.exit(__doc__)
    image, name, title = args[0], args[1].lower(), args[2]
    sets = {}
    i = 3
    while i < len(args):
        if args[i] == "--set":
            key, val = args[i+1].split("=", 1); sets[key.strip()] = [int(v) for v in val.split(",")]
            i += 2
        elif args[i] == "--box":                       # --box role=image,rect
            key, spec = args[i+1].split("=", 1)
            img, rect = spec.split(",", 1)
            x, y, w, h = [int(v) for v in rect.split(":")]
            sets[key] = sample(img, key, (x, y, x+w, y+h)); i += 2
        else: sys.exit(f"unknown arg {args[i]}")
    defaults, _ = base_palette()
    unknown = set(sets) - set(defaults)
    if unknown: sys.exit(f"unknown roles: {sorted(unknown)}")
    pinned = dict(sets)
    p = dict(defaults); p.update(sets)
    p = finalize(p); p.update({k:v for k,v in pinned.items() if k in
        ("accent_bright","accent_track","accent_glow","meter_low","meter_off")})
    camel = {"bg_deep":"kBgDeep","panel":"kPanel","panel_raised":"kPanelRaised","panel_inset":"kPanelInset",
             "well":"kWell","filigree":"kFiligree","filigree_dim":"kFiligreeDim","border_dark":"kBorderDark",
             "hairline":"kHairline","accent":"kAccent","accent_bright":"kAccentBright","accent_track":"kAccentTrack",
             "accent_glow":"kAccentGlow","title":"kTitle","cream":"kCream","muted":"kMuted","faint":"kFaint",
             "readout":"kReadout","meter":"kMeter","meter_low":"kMeterLow","hot":"kHot","meter_off":"kMeterOff",
             "violet":"kViolet","release":"kRelease"}
    lines = [f"// {title} — sampled from {Path(image).name}.",
             f"inline Palette {name}(){{",
             "  static const ThemeDef e[]={"]
    row = "    "
    for role in defaults:
        if role not in sets: continue
        r,g,b = p[role]
        entry = f"{{{camel[role]},{{{r},{g},{b}}}}},"
        if len(row)+len(entry) > 96: lines.append(row.rstrip(",")); row = "    "
        row += entry
    if row.strip(", "): lines.append(row.rstrip(","))
    lines += ["  return makeTheme(e,sizeof(e)/sizeof(*e),\"" + name + "\");", "}"]
    print("\n".join(lines))
    out = ROOT/"assets"/f"{name}-skin.txt"
    with open(out,"w") as f:
        f.write(f"# GrainsDosage skin — sampled from {Path(image).name}\ntheme = {name}\n")
        for role in sets:
            r,g,b = p[role]; f.write(f"{role} = #{r:02X}{g:02X}{b:02X}\n")
    print(f"\n# wrote {out.relative_to(ROOT)}", file=sys.stderr)

if __name__ == "__main__":
    main()
