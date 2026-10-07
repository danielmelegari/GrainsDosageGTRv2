#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// GrainsDosage — "Astral / Filigree Green" skin spec + built-in themes.
//
// Single, platform-neutral source of truth for the reskin. Both native
// renderers (editor_mac.mm / editor_win.cpp) and the offline preview tool
// (tools/preview.py) read ONLY from here, so the Cocoa build, the Win32 build
// and the CI screenshot can never drift apart.
//
// 0.14.0 — the skin became CUSTOMIZABLE. The original palette is kept as the
// default theme ("astral") and four more built-ins join it (ember, nebula,
// arctic, midnight). The runtime machinery — mutable active palette, skin-file
// parser, persistence — lives in skin_theme.h, which includes this file.
//
// Compatibility: the classic constexpr names below (green, amber, …) still
// exist so geometry-free code keeps compiling; renderers should prefer the
// LIVE palette accessors (skin::active() / theme::current()) so a theme switch
// recolours every element on the next frame.
//
// Palette sampled from the reference art: dark charcoal panels, ornate
// silver/taupe filigree borders, a SINGLE lime-green accent for every live
// element (knob arcs, ON pills, waveforms, step bars, LEDs, sequencer bars),
// an amber output meter and cool-grey engraved text. Colours are 8-bit sRGB.
//
// The canvas and every control rectangle live in the existing shared layout
// (canvasW x canvasH, see editor_layout_win.inl). These headers deliberately
// change ONLY colour/style, never geometry or parameter bindings.
// ─────────────────────────────────────────────────────────────────────────────
#include <string>
#include <cmath>
namespace aztec {
namespace skin {

struct Rgb { int r, g, b; };

// Logical design canvas (keep in sync with editor_mac.mm canvasW/canvasH and
// the 1320x1360 coordinate space used by editor_layout_win.inl).
constexpr double canvasW = 1632.0;
constexpr double canvasH = 1518.0;

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
constexpr Rgb onText    { 8,  16,  4};     // DARK label drawn on lit accent fills

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

// ════════════════════════════════════════════════════════════════════════════
// Customizable skin system (0.14.0)
//
// Every colour above also exists as a named ROLE (ColorKey). Themes are
// Palettes — plain structs that override any subset of roles; unset roles
// fall back to the astral defaults at compile time via kBase. Renderers
// resolve roles against the LIVE palette (skin::active()), which the SKIN
// menu and the skin file mutate at run time.
// ════════════════════════════════════════════════════════════════════════════

enum ColorKey {
  kBgDeep, kPanel, kPanelRaised, kPanelInset, kWell,
  kFiligree, kFiligreeDim, kBorderDark, kHairline,
  kAccent, kAccentBright, kAccentTrack, kAccentGlow,
  kTitle, kCream, kMuted, kFaint, kReadout, kOnText,
  kMeter, kMeterLow, kHot, kMeterOff,
  kViolet, kRelease,
  kColorCount
};
inline const char* colorName(int i) {
  static const char* names[kColorCount] = {
    "bg_deep","panel","panel_raised","panel_inset","well",
    "filigree","filigree_dim","border_dark","hairline",
    "accent","accent_bright","accent_track","accent_glow",
    "title","cream","muted","faint","readout","on_text",
    "meter","meter_low","hot","meter_off",
    "violet","release"};
  return i>=0&&i<kColorCount?names[i]:"";
}

// Astral defaults indexed by role — the fallback for partial themes.
inline Rgb baseRgb(int i) {
  const Rgb f[kColorCount] = {bgDeep,panel,panelRaised,panelInset,well,
    filigree,filigreeDim,borderDark,hairline,
    green,greenGlow,greenDim,greenDeep,
    title,cream,muted,faint,readout,onText,
    amber,amberLow,hot,meterOff,violet,release};
  return i>=0&&i<kColorCount?f[i]:Rgb{};
}

// Small colour helpers (clamp to 8-bit, used for derived shades).
inline int clamp255(int v){return v<0?0:(v>255?255:v);}
inline Rgb mix(Rgb a,Rgb b,double t){t=t<0.?0.:(t>1.?1.:t);return Rgb{int(a.r+(b.r-a.r)*t+.5),int(a.g+(b.g-a.g)*t+.5),int(a.b+(b.b-a.b)*t+.5)};}
inline Rgb scale(Rgb a,double f){return Rgb{clamp255(int(a.r*f+.5)),clamp255(int(a.g*f+.5)),clamp255(int(a.b*f+.5))};}

struct Palette {
  Rgb v[kColorCount];
  Palette(){for(int i=0;i<kColorCount;++i)v[i]=baseRgb(i);}
  Rgb at(int i) const {return i>=0&&i<kColorCount?v[i]:Rgb{};}
  void set(int i,Rgb c){if(i>=0&&i<kColorCount)v[i]=c;}
};

// Which roles finalize() recomputes from their parents.
inline bool isDerivedKey(int k){
  return k==kAccentBright||k==kAccentTrack||k==kAccentGlow||k==kMeterLow||k==kMeterOff;
}

// Rebuild the derived shades from the base keys so a customised accent (or a
// new theme) stays internally consistent: the unlit arc track, the toggle-ON
// glow and the meter tones all follow their parent colour automatically.
// "pinned" optionally protects roles that must keep their explicit value
// (nullptr => recompute every derived role).
inline void finalize(Palette& p,const bool* pinned=nullptr){
  auto skip=[&](int k){return pinned&&pinned[k];};
  if(!skip(kAccentBright))p.v[kAccentBright]=mix(p.at(kAccent),{255,255,255},.10);
  if(!skip(kAccentTrack)) p.v[kAccentTrack] =scale(p.at(kAccent),.23);
  if(!skip(kAccentGlow))  p.v[kAccentGlow]  =scale(p.at(kAccent),.30);
  if(!skip(kMeterLow))    p.v[kMeterLow]    =scale(p.at(kMeter),.52);
  if(!skip(kMeterOff))    p.v[kMeterOff]    =mix(p.at(kBgDeep),p.at(kMeter),.12);
  // Legibility guard: labels drawn on lit accent fills (ON pills, green pads,
  // RANDOM ONCE...) must clear WCAG-AA against the fill on EVERY theme and
  // custom skin. If the configured on-text colour is too close to the accent
  // glow, auto-pick black or white — whichever contrasts better with it.
  {
    auto relLum=[](Rgb c){auto lin=[](int v){double x=v/255.;return x<=.03928?x/12.92:std::pow((x+.055)/1.055,2.4);};
      return .2126*lin(c.r)+.7152*lin(c.g)+.0722*lin(c.b);};
    Rgb bg=p.at(kAccentGlow);double lb=relLum(bg);
    auto ratio=[&](Rgb t){double lt=relLum(t);double a=lt>lb?lt:lb,b=lt>lb?lb:lt;return (a+.05)/(b+.05);};
    if(ratio(p.at(kOnText))<4.5){
      Rgb blk{0,0,0},wht{255,255,255};
      p.v[kOnText]=ratio(blk)>=ratio(wht)?blk:wht;
    }
  }
}

// ── Built-in themes ───────────────────────────────────────────────────────────
// A theme lists only the roles it overrides; everything else inherits astral.
using ThemeDef = std::pair<ColorKey,Rgb>;
inline Palette makeTheme(const ThemeDef* entries,size_t n,const char* name=nullptr){
  (void)name;
  Palette p;
  bool pinned[kColorCount]={false};
  for(size_t i=0;i<n;++i){int k=int(entries[i].first);p.set(k,entries[i].second);pinned[k]=true;}
  // Derived shades recompute unless the theme pinned them explicitly.
  finalize(p,pinned);
  return p;
}

// Astral / Filigree Green — the original 0.13 palette, kept as the default.
inline Palette astral(){return Palette();}
// Ember / Copper — warm charcoal, orange accent, gold trim.
inline Palette ember(){
  static const ThemeDef e[]={
    {kBgDeep,{16,11,8}},{kPanel,{34,26,22}},{kPanelRaised,{45,34,28}},{kPanelInset,{20,14,11}},{kWell,{12,8,6}},
    {kFiligree,{186,152,104}},{kFiligreeDim,{110,88,60}},{kBorderDark,{54,42,32}},{kHairline,{70,56,44}},
    {kAccent,{255,146,64}},{kOnText,{20,10,4}},
    {kTitle,{228,208,182}},{kCream,{250,238,222}},{kMuted,{172,150,130}},{kFaint,{112,94,78}},{kReadout,{255,196,140}},
    {kMeter,{255,180,70}},{kHot,{255,72,52}},
    {kViolet,{232,120,196}},{kRelease,{242,80,74}}};
  return makeTheme(e,sizeof(e)/sizeof(*e),"ember");
}
// Nebula / Magenta — deep-space blue-violet plates, hot-pink accent.
inline Palette nebula(){
  static const ThemeDef e[]={
    {kBgDeep,{9,8,22}},{kPanel,{24,20,44}},{kPanelRaised,{33,28,58}},{kPanelInset,{14,11,28}},{kWell,{8,6,18}},
    {kFiligree,{142,132,188}},{kFiligreeDim,{86,78,120}},{kBorderDark,{40,36,66}},{kHairline,{56,52,88}},
    {kAccent,{255,96,192}},{kOnText,{20,4,14}},
    {kTitle,{206,198,232}},{kCream,{234,230,250}},{kMuted,{150,142,180}},{kFaint,{96,90,128}},{kReadout,{160,232,248}},
    {kMeter,{255,150,90}},{kHot,{255,70,70}},
    {kViolet,{150,120,255}},{kRelease,{255,84,110}}};
  return makeTheme(e,sizeof(e)/sizeof(*e),"nebula");
}
// Arctic / Platinum — cool slate greys, ice-blue accent.
inline Palette arctic(){
  static const ThemeDef e[]={
    {kBgDeep,{10,13,17}},{kPanel,{26,31,38}},{kPanelRaised,{36,42,52}},{kPanelInset,{15,19,25}},{kWell,{9,12,16}},
    {kFiligree,{172,184,200}},{kFiligreeDim,{104,116,132}},{kBorderDark,{44,52,64}},{kHairline,{62,72,86}},
    {kAccent,{96,220,255}},{kOnText,{4,14,20}},
    {kOnText,{10,14,20}},{kTitle,{214,224,236}},{kCream,{238,244,250}},{kMuted,{152,164,180}},{kFaint,{98,110,126}},{kReadout,{180,236,255}},
    {kMeter,{236,196,96}},{kHot,{255,92,72}},
    {kViolet,{150,170,255}},{kRelease,{255,104,96}}};
  return makeTheme(e,sizeof(e)/sizeof(*e),"arctic");
}
// Aurora / Lime — sampled 1:1 from assets/NEWGUI.jpeg (user mockup, 0.14.x).
// Pure-black deep bg, blue-charcoal panels, cool silver filigree, a bright
// spring-green accent everywhere live, mint readouts; the meter stays amber.
inline Palette aurora(){
  static const ThemeDef e[]={
    {kBgDeep,{3,2,10}},{kPanel,{22,22,32}},{kPanelRaised,{31,31,43}},{kPanelInset,{11,22,27}},{kWell,{8,6,14}},
    {kFiligree,{168,172,178}},{kFiligreeDim,{104,104,115}},{kBorderDark,{40,40,52}},{kHairline,{58,58,70}},
    {kAccent,{156,250,110}},{kAccentBright,{206,255,176}},{kAccentGlow,{182,253,166}},
    {kOnText,{6,16,6}},{kTitle,{224,228,231}},{kCream,{241,250,253}},{kMuted,{150,154,150}},{kFaint,{96,100,96}},{kReadout,{114,244,170}},
    {kViolet,{110,85,137}}};
  return makeTheme(e,sizeof(e)/sizeof(*e),"aurora");
}
// Midnight / Mono — near-black panels, silver trim, single white accent.
inline Palette midnight(){
  static const ThemeDef e[]={
    {kBgDeep,{6,6,8}},{kPanel,{18,18,22}},{kPanelRaised,{26,26,32}},{kPanelInset,{10,10,14}},{kWell,{5,5,8}},
    {kFiligree,{168,168,176}},{kFiligreeDim,{92,92,102}},{kBorderDark,{36,36,44}},{kHairline,{52,52,62}},
    {kAccent,{235,235,235}},{kOnText,{10,10,12}},
    {kTitle,{200,200,208}},{kCream,{240,240,244}},{kMuted,{150,150,158}},{kFaint,{92,92,100}},{kReadout,{200,220,230}},
    {kMeter,{220,220,220}},{kHot,{255,90,70}},
    {kViolet,{180,180,200}},{kRelease,{240,110,100}}};
  return makeTheme(e,sizeof(e)/sizeof(*e),"midnight");
}

// Purple panels, white type and cyan indicators from GRAINSDOSAGE_ALL.jpg.
inline Palette purple(){
 static const ThemeDef e[]={
  {kBgDeep,{13,10,28}},{kPanel,{99,53,176}},{kPanelRaised,{47,24,92}},
  {kPanelInset,{40,36,58}},{kWell,{15,11,28}},{kFiligree,{216,213,229}},
  {kFiligreeDim,{145,111,201}},{kBorderDark,{54,29,114}},{kHairline,{81,72,101}},
  {kAccent,{0,241,222}},{kAccentGlow,{0,81,77}},{kAccentBright,{114,255,239}},
  {kTitle,{255,255,255}},{kCream,{255,255,255}},{kMuted,{158,153,169}},
  {kFaint,{84,79,102}},{kReadout,{255,255,255}},{kOnText,{255,255,255}}};
 return makeTheme(e,sizeof(e)/sizeof(*e),"purple");
}
constexpr int themeCount = 7;
inline const char* themeName(int i){
  static const char* names[themeCount]={"astral","ember","nebula","arctic","aurora","midnight","purple"};
  return i>=0&&i<themeCount?names[i]:"astral";
}
inline const char* themeTitle(int i){
  static const char* titles[themeCount]={"ASTRAL GREEN","EMBER COPPER","NEBULA ROSE","ARCTIC PLATINUM","AURORA LIME","MIDNIGHT MONO","PURPLE MOCKUP"};
  return i>=0&&i<themeCount?titles[i]:"ASTRAL GREEN";
}
inline Palette themePalette(int i){
  switch(i){case 1:return ember();case 2:return nebula();case 3:return arctic();case 4:return aurora();case 5:return midnight();case 6:return purple();default:return astral();}
}
inline int themeIndex(const std::string& name){
  for(int i=0;i<themeCount;++i)if(name==themeName(i))return i;return -1;
}
// Resolve a colour-role name ("accent", "accent_track") to its key.
inline int keyForName(const std::string& n){for(int i=0;i<kColorCount;++i)if(n==colorName(i))return i;return -1;}

// ── Default theme at start-up ────────────────────────────────────────────────
// Aurora Lime is the shipped default since 0.14 (sampled from the user's
// NEWGUI mockup). The SKIN menu can switch to any other theme at run time,
// and a persisted skin file overrides this when present. Set to 0 for the
// original Astral Green look.
constexpr int kDefaultTheme = 6; // supplied purple mockup

// ── Active skin (renderer state) ─────────────────────────────────────────────
inline Palette& active(){static Palette p=themePalette(kDefaultTheme);return p;}
inline int& activeTheme(){static int i=kDefaultTheme;return i;}
inline bool& activeCustomized(){static bool c=false;return c;}
inline Rgb C(ColorKey k){return active().at(int(k));}

} // namespace skin
} // namespace aztec

