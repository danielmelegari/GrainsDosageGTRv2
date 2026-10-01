#!/usr/bin/env python3
"""
Offline preview of the GrainsDosage editor in the "Astral / Filigree Green" skin.

This mirrors the native draw code (editor_mac.mm / editor_win.cpp) on the shared
1320x1360 canvas, reading the palette straight from src/skin_spec.h so the preview
can never drift from the real renderers. It is a SKIN preview: geometry and control
kinds match the plugin; parameter values are representative (matched to the
reference art) because there is no running DSP here.

Usage:  python3 tools/preview.py [out.png] [--scale 2.0]
"""
import re, sys, math
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont, ImageFilter

ROOT = Path(__file__).resolve().parents[1]
FONTS = ROOT / "tools" / "preview_fonts"

# ── palette: parse the real skin_spec.h so this stays single-source ───────────
def load_palette():
    txt = (ROOT / "src" / "skin_spec.h").read_text()
    pal = {}
    for name, r, g, b in re.findall(r"Rgb\s+(\w+)\s*\{\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}", txt):
        pal[name] = (int(r), int(g), int(b))
    return pal
P = load_palette()
def c(name, a=255):
    r, g, b = P[name]
    return (r, g, b, a)

CW, CH = 1320, 1360
SCALE = 2.0
for i, a in enumerate(sys.argv):
    if a == "--scale" and i + 1 < len(sys.argv):
        SCALE = float(sys.argv[i + 1])
OUT = Path(next((a for a in sys.argv[1:] if not a.startswith("--") and a != str(SCALE)), "editor-preview.png"))

img = Image.new("RGB", (int(CW * SCALE), int(CH * SCALE)), P["bgDeep"])
d = ImageDraw.Draw(img, "RGBA")
S = SCALE
def sx(v): return v * S

_font_cache = {}
def font(path, size):
    key = (path, round(size))
    if key not in _font_cache:
        _font_cache[key] = ImageFont.truetype(str(FONTS / path), max(6, round(size * S)))
    return _font_cache[key]
def LBL(size): return font("Jost.ttf", max(9, size))        # labels / values
def TTL(size): return font("CinzelDecorative.ttf", size)    # engraved title
def SER(size): return font("Cinzel.ttf", size)              # section caps

# optional artwork overlays (Phase 1 asset pack)
ASSETS = ROOT / "assets"
def load_rgba(name):
    p = ASSETS / name
    return Image.open(p).convert("RGBA") if p.exists() else None
NEBULA = load_rgba("xy_nebula.png")
FRAME = load_rgba("filigree-frame.png")
def paste_img(im, x, y, w, h):
    if im is None:
        return
    r = im.resize((max(1, int(sx(w))), max(1, int(sx(h)))), Image.LANCZOS)
    base = img.convert("RGBA")
    base.alpha_composite(r, (int(sx(x)), int(sx(y))))
    img.paste(base.convert("RGB"), (0, 0))

def box(x, y, w, h, fill, stroke=None, r=6, sw=1):
    d.rounded_rectangle([sx(x), sx(y), sx(x + w), sx(y + h)], radius=sx(r),
                        fill=fill, outline=stroke, width=max(1, round(sw * S)))
def rect(x, y, w, h, fill):
    d.rectangle([sx(x), sx(y), sx(x + w), sx(y + h)], fill=fill)
def line(x1, y1, x2, y2, color, w=1):
    d.line([sx(x1), sx(y1), sx(x2), sx(y2)], fill=color, width=max(1, round(w * S)))
def text(s, x, y, w, h, size, color, center=False, fnt=None, min_size=10):
    f = fnt or LBL(max(min_size, size))
    bb = d.textbbox((0, 0), s, font=f)
    tw, th = bb[2] - bb[0], bb[3] - bb[1]
    tx = sx(x) + (sx(w) - tw) / 2 if center else sx(x) + 2 * S
    ty = sx(y) + (sx(h) - th) / 2 - bb[1]
    d.text((tx, ty), s, font=f, fill=color)
def arc(cx, cy, radius, a0, a1, color, width=3):
    bb = [sx(cx - radius), sx(cy - radius), sx(cx + radius), sx(cy + radius)]
    d.arc(bb, a0, a1, fill=color, width=max(1, round(width * S)))

# ── panel with ornate-ish silver/taupe double border + corner flourishes ─────
def panel(x, y, w, h):
    box(x, y, w, h, c("panel"), c("filigree", 150), r=12, sw=2)
    box(x + 3, y + 3, w - 6, h - 6, None, c("borderDark"), r=10, sw=1)
    line(x + 15, y + 5, x + w - 15, y + 5, c("filigree", 120), 1)
    for side in (0, 1):
        xx = x + w - 6 if side else x + 6
        for t in range(0, 11):
            yy = y + 14 + (h - 28) * t / 10
            dx = 3 * math.sin(t * math.pi / 2)
            d.ellipse([sx(xx + (dx if side else -dx) - 1), sx(yy - 1),
                       sx(xx + (dx if side else -dx) + 1), sx(yy + 1)], fill=c("filigreeDim", 200))
    # corner nubs
    for (cxp, cyp) in [(x + 10, y + 10), (x + w - 10, y + 10), (x + 10, y + h - 10), (x + w - 10, y + h - 10)]:
        d.ellipse([sx(cxp - 3), sx(cyp - 3), sx(cxp + 3), sx(cyp + 3)], fill=c("filigree", 220))

# ── knob: dark metal cap, lime tick ring + lime value arc, pointer, readout ──
def knob(x, y, w, h, v, label, disp, bipolar=False):
    text(label, x, y, w, 16, 11, c("cream"), center=True, fnt=LBL(12))
    cx, cy, radius = x + w / 2, y + 43, 24
    d.ellipse([sx(cx - radius - 2), sx(cy - radius - 2), sx(cx + radius + 2), sx(cy + radius + 2)], fill=c("well"))
    d.ellipse([sx(cx - radius), sx(cy - radius), sx(cx + radius), sx(cy + radius)],
              fill=(34, 32, 40), outline=c("filigreeDim"), width=max(1, round(S)))
    d.ellipse([sx(cx - radius + 6), sx(cy - radius + 5), sx(cx + radius - 6), sx(cy + radius - 7)], fill=(46, 44, 54))
    arc(cx, cy, radius + 4, 135, 405, c("greenDim"), 3)        # unlit track
    for j in range(21):
        a = math.radians(135 + 270 * j / 20)
        col = c("green") if j / 20 <= v else c("greenDim")
        px, py = cx + 33 * math.cos(a), cy + 33 * math.sin(a)
        d.ellipse([sx(px - 1.5), sx(py - 1.5), sx(px + 1.5), sx(py + 1.5)], fill=col)
    zero = 405 if bipolar else 135
    angle = 135 + 270 * v
    arc(cx, cy, radius + 4, min(zero, angle), max(zero, angle), c("green"), 4)
    a = math.radians(angle)
    line(cx + 8 * math.cos(a), cy + 8 * math.sin(a), cx + 22 * math.cos(a), cy + 22 * math.sin(a), c("cream"), 3)
    box(x + 4, y + 74, w - 8, 19, c("well"), c("hairline"), r=4)
    text(disp, x, y + 74, w, 19, 12, c("readout"), center=True, fnt=LBL(12))

def slider(x, y, w, h, v, label, disp, bipolar=False):
    text(label, x, y, w, 13, 9, c("muted"), fnt=LBL(10))
    yy, xx, width = y + 20, x + 4, w - 8
    rect(xx, yy, width, 4, c("hairline"))
    start = 0.5 if bipolar else 0.0
    rect(xx + min(v, start) * width, yy, max(1, abs(v - start) * width), 4, c("green"))
    kx = xx + v * width
    box(kx - 6, yy - 7, 12, 16, c("cream"), c("filigreeDim"), r=3)
    if h >= 33:
        text(disp, x, y + 28, w, 12, 10, c("cream"), center=True)
    else:
        text(disp, x + 65, y, w - 65, 13, 9, c("cream"), center=True)

def select(x, y, w, h, label, disp):
    box(x, y, w, h, c("well"), c("borderDark"), r=6)
    stacked = h >= 38
    if stacked:
        text(label, x + 8, y + 4, w - 24, 11, 8, c("muted"), fnt=LBL(9))
    text(disp, x + 8, y + (18 if stacked else 9), w - 29, 18, 11, c("cream"))
    text("▾", x + w - 21, y + (17 if stacked else 8), 16, 18, 12, c("green"))

def toggle(x, y, w, h, on, label):
    box(x, y, w, h, c("greenDeep") if on else c("panelRaised"),
        c("green") if on else c("filigreeDim"), r=6)
    if on:
        d.ellipse([sx(x + 6), sx(y + h / 2 - 6), sx(x + 18), sx(y + h / 2 + 6)], fill=(36, 72, 18, 255))
    d.ellipse([sx(x + 9), sx(y + h / 2 - 3), sx(x + 15), sx(y + h / 2 + 3)],
              fill=c("green") if on else c("muted"))
    text(label, x + 20, y, w - 25, h, 10, c("cream") if on else c("muted"), center=True)

# ═══════════════════════════ HEADER ══════════════════════════════════════════
panel(8, 8, 1304, 73)
text("GRAINS DOSAGE", 360, 16, 600, 48, 30, c("title"), center=True, fnt=TTL(30))
box(420, 22, 462, 30, c("well"), c("green"), r=5)
text("Astral Fragments", 432, 22, 438, 30, 13, c("cream"), fnt=LBL(14))
text("‹", 400, 22, 20, 30, 16, c("muted"), center=True)
text("›", 884, 22, 16, 30, 16, c("muted"), center=True)
for (bx, lab) in [(900, "SAVE"), (1004, "LOAD")]:
    box(bx, 22, 96, 28, c("panelRaised"), c("filigree"), r=5)
    text(lab, bx, 22, 96, 28, 11, c("cream"), center=True, fnt=SER(11))
box(1118, 20, 60, 34, c("panelRaised"), c("filigreeDim"), r=5)
text("⋯", 1118, 16, 60, 34, 18, c("filigree"), center=True)
box(1218, 20, 60, 34, c("panelRaised"), c("filigreeDim"), r=5)
d.ellipse([sx(1240), sx(30), sx(1256), sx(46)], outline=c("filigree"), width=max(1, round(2 * S)))

# ═══════════════════════ ROW 1: three module panels ══════════════════════════
slotX = [16, 452, 888]
mod_titles = ["GRANULAR", "GLITCH BUFFER", "BEAT REPEATER"]
for slot in range(3):
    x = slotX[slot]
    panel(x, 98, 416, 404)
    d.rectangle([sx(x + 18), sx(112), sx(x + 30), sx(124)], outline=c("filigreeDim"), width=max(1, round(S)))
    text(mod_titles[slot], x + 40, 104, 230, 25, 15, c("cream"), fnt=SER(15))
    toggle(x + 238, 106, 68, 28, True, "ON")
    box(x + 316, 106, 87, 28, c("panelRaised"), c("filigree"), r=6)
    text("RANDOM", x + 319, 106, 81, 28, 10, c("cream"), center=True)
    for j in range(12):          # activity LEDs
        rect(x + 19 + j * 32, 486, 23, 4, c("green") if j < 9 else c("greenDim"))

# GRANULAR controls
gx = slotX[0]
gran = [("SIZE", 0.50, "120 ms", 0), ("DENSITY", 0.43, "43 %", 0), ("PITCH", 0.5, "0.00 st", 1),
        ("LOOKBACK", 0.30, "1.2 s", 0), ("CHAOS", 0.28, "28 %", 0), ("MIX", 0.70, "70 %", 0)]
for i, (lab, v, disp, bip) in enumerate(gran):
    knob(gx + 14 + (i % 4) * 99, 151 if i < 4 else 257, 90, 92, v, lab, disp, bool(bip))
box(gx + 208, 248, 196, 147, c("panelInset"), c("filigreeDim"), r=8)
text("MASTER OPTIONS", gx + 220, 252, 174, 17, 10, c("muted"), center=True)
knob(gx + 222, 274, 90, 92, 0.5, "SPEED", "1.00 x")
knob(gx + 321, 274, 90, 92, 0.42, "TRANSPOSE", "-12 st", True)
# waveform follow screen
sw = (gx + 14, 432, 388, 48)
box(*sw, c("panelInset"), c("hairline"), r=4)
peak = [abs(math.sin(b * 0.14)) * (0.3 + 0.7 * abs(math.sin(b * 0.03))) for b in range(128)]
for b in range(128):
    h = max(1, peak[b] * 30)
    lit = 0.30 <= (b + 1) / 128 <= 0.52
    rect(gx + 18 + b * 3.0, 446 + (30 - h) * 0.5, 2, h, c("green") if lit else (92, 97, 99))
text("GRAIN FOLLOW · 2.5 s", gx + 22, 433, 360, 11, 9, c("muted"))
text("PAN", gx + 14, 404, 40, 20, 10, c("muted"))
rect(gx + 56, 414, 300, 3, c("hairline"))
rect(gx + 56, 414, 150, 3, c("green"))
box(gx + 202, 409, 8, 12, c("green"), None, r=2)

# GLITCH controls
x = slotX[1]
select(x + 18, 151, 132, 46, "SLICE", "1/8")
toggle(x + 18, 210, 132, 28, False, "REVERSE")
knob(x + 164, 151, 112, 92, 0.62, "CHANCE", "62 %")
knob(x + 285, 151, 112, 92, 0.80, "MIX", "80 %")
select(x + 18, 294, 240, 46, "TRIGGER EVERY", "4")
slider(x + 18, 415, 116, 40, 0.28, "MOVE", "28 %")
slider(x + 150, 415, 116, 40, 0.71, "VARIATION", "71 %")
knob(x + 164, 257, 112, 92, 0.54, "TRIGGER", "54 %")
knob(x + 285, 257, 112, 92, 0.37, "REVERSE", "37 %")

# REPEATER controls + 16-step grid
x = slotX[2]
knob(x + 14, 151, 112, 92, 0.5, "HOLD", "1/8")
knob(x + 135, 151, 112, 92, 0.50, "LENGTH", "50 %")
knob(x + 256, 151, 112, 92, 0.76, "MIX", "76 %")
on_steps = {0, 4, 8, 9, 12, 13}
text("1  2  3  4  5  6  7  8   9 10 11 12 13 14 15 16", x + 19, 250, 380, 14, 9, c("muted"))
for i in range(16):
    a = x + 19 + (i % 8) * 48
    b = 271 + (i // 8) * 39
    on = i in on_steps
    box(a, b + 3, 43, 29, c("well"), c("borderDark"), r=5)
    box(a, b, 43, 28, c("greenDeep") if on else c("panelRaised"),
        c("green") if on else c("filigreeDim"), r=5)
    text(str(i + 1), a, b, 43, 16, 10, c("cream") if on else c("muted"), center=True)
rates = ["1/32", "1/16", "1/8", "1/4", "1/2", "1/1", "2/1", "4/1"]
for i, rt in enumerate(rates):
    sel = i == 2
    box(x + 19 + i * 48, 356, 43, 26, c("greenDeep") if sel else c("panel"),
        c("green") if sel else c("filigreeDim"), r=5)
    text(rt, x + 19 + i * 48, 356, 43, 26, 9, c("green") if sel else c("muted"), center=True)

# ═══════════════════════ WAVEFORM STRIP ══════════════════════════════════════
panel(16, 516, 1288, 110)
text("glass_choir_texture.wav", 36, 530, 260, 18, 11, c("muted"))
text("‹  ›", 300, 530, 40, 18, 12, c("green"))
text("GRAIN POSITION", 1000, 532, 120, 14, 9, c("muted"))
box(1130, 528, 150, 22, c("well"), c("filigreeDim"), r=4)
text("2.384 s / 7.603 s", 1130, 528, 150, 22, 10, c("readout"), center=True)
wf = (28, 556, 1272, 58)
box(*wf, c("panelInset"), c("hairline"), r=6)
mid = 556 + 29
for b in range(424):
    u = b / 424
    amp = (0.15 + 0.85 * abs(math.sin(u * 40) * math.sin(u * 7))) * 26
    lit = 0.44 <= u <= 0.52
    xx = 34 + b * 3
    rect(xx, mid - amp / 2, 2, amp, c("green") if lit else (60, 66, 72))

# ═══════════════════════ MOD tabs + scope + destinations ═════════════════════
panel(16, 640, 836, 236)
text("MOD 1", 32, 652, 100, 24, 13, c("cream"), fnt=SER(13))
for i in range(4):
    sel = i == 0
    box(171 + i * 133, 648, 120, 28, c("greenDeep") if sel else c("panel"),
        c("green") if sel else c("filigreeDim"), r=6)
    text(f"MOD {i+1}", 171 + i * 133, 648, 120, 28, 11, c("cream") if sel else c("muted"), center=True)
select(32, 690, 120, 34, "", "Sine")
box(170, 690, 54, 34, c("panel"), c("filigreeDim"), r=5); text("WAVE", 170, 690, 54, 34, 10, c("cream"), center=True)
box(230, 690, 54, 34, c("panel"), c("filigreeDim"), r=5); text("RND", 230, 690, 54, 34, 10, c("muted"), center=True)
scope = (32, 735, 600, 110)
box(*scope, c("panelInset"), c("hairline"), r=6)
pts = []
for i in range(0, 601, 4):
    u = i / 600
    y = 790 - 42 * math.sin(u * 2 * math.pi) * (0.6 + 0.4 * math.sin(u * 5))
    pts.append((sx(32 + i), sx(y)))
d.line(pts, fill=c("green"), width=max(1, round(2 * S)))
line(400, 735, 400, 845, c("cream"), 1)
knob(40, 760, 90, 92, 0.33, "RATE", "1/2")
knob(120, 760, 90, 92, 0.12, "GLIDE", "12 %")
# destinations
text("DESTINATIONS (6)", 652, 648, 200, 18, 11, c("muted"))
dests = [("Granular > Size", "56 %", 0), ("Granular > Pitch", "-28 %", 1),
         ("Filter > Cutoff", "42 %", 0), ("Repeater > Mix", "31 %", 0),
         ("Glitch > Chance", "-64 %", 1), ("Reverb > Length", "18 %", 0)]
for i, (dn, amt, neg) in enumerate(dests):
    yy = 674 + i * 30
    text(str(i + 1), 652, yy, 16, 24, 10, c("green"))
    select(672, yy, 150, 24, "", dn)
    box(828, yy, 20, 24, c("well"), c("hairline"), r=4)
    text(amt, 824, yy, 30, 24, 9, c("green"), center=True)

# ═══════════════════════ XY MORPH ════════════════════════════════════════════
panel(864, 640, 440, 236)
text("XY MORPH", 880, 652, 240, 24, 13, c("cream"), fnt=SER(13))
toggle(1185, 650, 101, 27, True, "ON")
pad = (880, 690, 168, 166)
box(*pad, c("panelInset"), c("violet", 120), r=8)
paste_img(NEBULA, 882, 692, 164, 162)
d = ImageDraw.Draw(img, "RGBA")
pcx, pcy = 880 + 84, 690 + 83
px, py = 880 + 0.75 * 168, 856 - 0.75 * 166
line(px, 690, px, 856, c("green", 90), 1)
line(880, py, 1048, py, c("green", 90), 1)
glow = Image.new("RGBA", img.size, (0, 0, 0, 0))
gd = ImageDraw.Draw(glow)
gd.ellipse([sx(px - 12), sx(py - 12), sx(px + 12), sx(py + 12)], fill=c("green", 160))
glow = glow.filter(ImageFilter.GaussianBlur(radius=int(6 * S)))
img.paste(Image.alpha_composite(img.convert("RGBA"), glow).convert("RGB"), (0, 0))
d = ImageDraw.Draw(img, "RGBA")
d.ellipse([sx(px - 5), sx(py - 5), sx(px + 5), sx(py + 5)], fill=c("green"), outline=c("cream"))
select(1062, 690, 224, 34, "X DESTINATION", "Filter Cutoff")
select(1062, 740, 224, 34, "Y DESTINATION", "Reverb Length")
knob(1095, 786, 90, 80, 0.67, "AMOUNT", "67 %")

# ═══════════════════════ RESLICE ═════════════════════════════════════════════
panel(16, 890, 1288, 96)
text("RESLICE", 36, 900, 108, 24, 14, c("cream"), fnt=SER(14))
toggle(148, 898, 72, 24, True, "ON")
select(240, 896, 110, 28, "WINDOW", "4/1")
knob(360, 892, 70, 70, 0.23, "STEP RND", "23 %")
select(500, 896, 90, 28, "RATE", "1/1")
text("RANDOM", 620, 900, 70, 20, 10, c("muted"))
box(700, 896, 70, 26, c("greenDeep"), c("green"), r=5); text("ONCE", 700, 896, 70, 26, 10, c("green"), center=True)
box(776, 896, 100, 26, c("panel"), c("filigreeDim"), r=5); text("CONTINUOUS", 776, 896, 100, 26, 9, c("muted"), center=True)
on_slice = {0, 2, 6, 11}
for i in range(16):
    xx = 32 + i * 79
    on = i in on_slice
    box(xx, 936, 72, 42, c("panelInset"), c("green") if on else c("filigreeDim"), r=5)
    for b in range(22):
        u = b / 22
        amp = (0.2 + 0.8 * abs(math.sin((i + u) * 6))) * 16
        rect(xx + 6 + b * 2.7, 957 - amp / 2, 2, amp, c("green") if on else (60, 66, 72))

# ═══════════════════════ GATER ═══════════════════════════════════════════════
panel(16, 992, 1288, 150)
text("GATER", 36, 1002, 104, 24, 14, c("cream"), fnt=SER(14))
toggle(148, 1000, 72, 24, True, "ON")
knob(236, 1000, 70, 70, 0.0, "MIN LENGTH", "1/32")
knob(336, 1000, 70, 70, 0.54, "SUSTAIN LEN", "54 %")
knob(436, 1000, 70, 70, 0.28, "LENGTH RND", "28 %")
knob(236, 1070, 70, 60, 0.19, "STEP RND", "19 %")
box(368, 1082, 80, 26, c("greenDeep"), c("green"), r=5); text("TIE", 368, 1082, 80, 26, 10, c("green"), center=True)
box(456, 1082, 80, 26, c("panel"), c("filigreeDim"), r=5); text("LATCH", 456, 1082, 80, 26, 10, c("muted"), center=True)
import random as _r
_r.seed(7)
for i in range(16):
    xx = 640 + i * 41
    h = _r.choice([0.2, 0.5, 0.8, 1.0, 0.35, 0.9])
    box(xx, 1016, 34, 110, c("panelInset"), c("hairline"), r=4)
    bh = 100 * h
    rect(xx + 3, 1122 - bh, 28, bh, c("green"))

# ═══════════════════════ FILTER + REVERB ═════════════════════════════════════
panel(16, 1148, 540, 120)
text("FILTER", 36, 1158, 100, 22, 13, c("cream"), fnt=SER(13))
toggle(148, 1156, 60, 22, True, "ON")
select(32, 1192, 100, 34, "MODEL", "Comb")
select(146, 1192, 70, 34, "ROOT", "C")
select(230, 1192, 90, 34, "SCALE", "Minor")
knob(330, 1168, 70, 92, 0.48, "CUTOFF", "2.4 kHz")
knob(410, 1168, 70, 92, 0.62, "RESONANCE", "0.62")
knob(480, 1168, 70, 92, 0.18, "DRIVE", "18 %")
panel(568, 1148, 736, 120)
text("REVERB", 588, 1158, 100, 22, 13, c("cream"), fnt=SER(13))
toggle(700, 1156, 60, 22, True, "ON")
knob(584, 1172, 70, 88, 0.72, "LENGTH", "7.2 s")
knob(664, 1172, 70, 88, 0.48, "MIX", "48 %")
select(750, 1180, 120, 24, "TYPE", "Cosmic Space")
select(750, 1210, 120, 24, "TRIGGER", "Input")
text("STEP TRIGGER", 884, 1160, 150, 16, 10, c("muted"))
lit_rev = {3, 4, 10, 14, 15}
for i in range(16):
    xx = 884 + i * 26
    on = i in lit_rev
    box(xx, 1180, 22, 20, c("greenDeep") if on else c("panel"), c("green") if on else c("filigreeDim"), r=3)
    text(str(i + 1), xx, 1200, 22, 12, 8, c("muted"), center=True)

# ═══════════════════════ MASTER BAR + meter ══════════════════════════════════
panel(16, 1274, 1288, 78)
text("MASTER", 36, 1292, 90, 22, 13, c("cream"), fnt=SER(13))
knob(150, 1288, 70, 70, 0.78, "DRY / WET", "78 %")
toggle(250, 1300, 90, 26, True, "NORMALIZE")
toggle(352, 1300, 90, 26, True, "LIMITER")
select(452, 1300, 80, 26, "", "-6 dB")
text("AUDIO ORDER", 556, 1286, 150, 14, 9, c("muted"))
flow = "Granular › Glitch › Repeater › Reslice › Gater › Filter › Reverb"
box(556, 1304, 360, 24, c("well"), c("filigreeDim"), r=5)
text(flow, 560, 1304, 356, 24, 9, c("cream"))
text("OUTPUT", 940, 1286, 100, 14, 10, c("cream"))
for j in range(24):
    lv = j / 24
    col = c("meterOff")
    if lv < 0.78:
        col = c("hot") if j > 20 else c("amber")
    rect(940 + j * 5, 1306, 3, 14, col)
text("-24", 940, 1322, 30, 12, 8, c("faint"))
text("0", 1050, 1322, 20, 12, 8, c("faint"))
box(1130, 1300, 150, 26, c("panelRaised"), c("filigreeDim"), r=5)
text("UI SIZE  100% ▾", 1130, 1300, 150, 26, 10, c("cream"), center=True)

# ── ornate filigree corner flourishes (Phase 1 artwork) ─────────────────────
def paste_rgba(im, x, y, w, h):
    if im is None:
        return
    r = im.resize((max(1, int(sx(w))), max(1, int(sx(h)))), Image.LANCZOS)
    base = img.convert("RGBA")
    base.alpha_composite(r, (int(sx(x)), int(sx(y))))
    img.paste(base.convert("RGB"), (0, 0))
if FRAME is not None:
    corner = FRAME.crop((0, 0, 300, 300))
    cs = 150
    paste_rgba(corner, 6, 4, cs, cs)
    paste_rgba(corner.transpose(Image.FLIP_LEFT_RIGHT), CW - 6 - cs, 4, cs, cs)
    paste_rgba(corner.transpose(Image.FLIP_TOP_BOTTOM), 6, CH - 4 - cs, cs, cs)
    paste_rgba(corner.transpose(Image.ROTATE_180), CW - 6 - cs, CH - 4 - cs, cs, cs)

img.save(OUT)
print(f"wrote {OUT} ({img.size[0]}x{img.size[1]})")
