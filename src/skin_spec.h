#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// GrainsDosage — "Astral / Filigree Green" skin spec.
//
// Single, platform-neutral source of truth for the reskin. Both native
// renderers (editor_mac.mm / editor_win.cpp) and the offline preview tool
// (tools/preview.py) read ONLY from here, so the Cocoa build, the Win32 build
// and the CI screenshot can never drift apart.
//
// Palette sampled from the reference art: dark charcoal panels, ornate
// silver/taupe filigree borders, a SINGLE lime-green accent for every live
// element (knob arcs, ON pills, waveforms, step bars, LEDs, sequencer bars),
// an amber output meter and cool-grey engraved text. Colours are 8-bit sRGB.
//
// The canvas and every control rectangle live in the existing shared layout
// (canvasW x canvasH, see editor_layout_win.inl). This header deliberately
// changes ONLY colour/style, never geometry or parameter bindings.
// ─────────────────────────────────────────────────────────────────────────────
namespace aztec {
namespace skin {

struct Rgb { int r, g, b; };

// Logical design canvas (keep in sync with editor_mac.mm canvasW/canvasH and
// the 1320x1360 coordinate space used by editor_layout_win.inl).
constexpr double canvasW = 1320.0;
constexpr double canvasH = 1360.0;

// ── Backgrounds / plates ────────────────────────────────────────────────────
constexpr Rgb bgDeep      {11, 10, 18};   // outermost charcoal behind the frame
constexpr Rgb panel       {25, 25, 34};   // module plate fill
constexpr Rgb panelRaised {33, 33, 45};   // raised sub-plate (master options, buttons)
constexpr Rgb panelInset  {13, 13, 20};   // recessed screens (waveform, scope, XY)
constexpr Rgb well        { 8,  8, 14};   // deepest wells (knob shadow, readouts)

// ── Filigree / borders (silver-taupe metal) ─────────────────────────────────
constexpr Rgb filigree    {156, 142, 120}; // bright taupe-silver highlight
constexpr Rgb filigreeDim {96,  88,  80};  // mid metal / vine strokes
constexpr Rgb borderDark  {44,  40,  54};  // engraved inner shadow
constexpr Rgb hairline    {60,  56,  70};  // thin neutral dividers / tracks

// ── Accent: the single lime green used for ALL live elements ─────────────────
constexpr Rgb green     {156, 255, 89};    // canonical accent (arcs, ON, bars)
constexpr Rgb greenGlow {182, 255, 103};   // brighter rim / highlight
constexpr Rgb greenDim  {36,  52,  24};    // unlit arc track / inactive tick
constexpr Rgb greenDeep {24,  48,  12};    // toggle-ON inner glow

// ── Text ─────────────────────────────────────────────────────────────────────
constexpr Rgb title   {216, 210, 196};     // engraved ivory for GRAINS DOSAGE
constexpr Rgb cream   {230, 240, 250};     // primary labels / values
constexpr Rgb muted   {150, 144, 160};     // secondary labels (cool grey)
constexpr Rgb faint   {96,  90, 108};      // tertiary / captions
constexpr Rgb readout {174, 224, 232};     // knob numeric readout (cool cyan)

// ── Meter / warnings ──────────────────────────────────────────────────────────
constexpr Rgb amber    {232, 161, 60};     // output meter body
constexpr Rgb amberLow {120, 96, 40};      // meter unlit
constexpr Rgb hot      {255, 80, 48};      // meter peak / clip
constexpr Rgb meterOff {20, 24, 20};       // meter empty segment

// ── Secondary live colours (kept on-brand with the cosmic art) ───────────────
constexpr Rgb violet  {170, 117, 245};     // XY nebula geometry / sustain overlay
constexpr Rgb release {242, 89, 82};       // gater latch-release state

// ── Line weights / radii (canvas units) ──────────────────────────────────────
constexpr double panelRadius = 12.0;
constexpr double boxRadius   = 6.0;
constexpr double knobRadius  = 24.0;

} // namespace skin
} // namespace aztec
