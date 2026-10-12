// Console smoke test for the customizable skin system (skin_spec.h + skin_theme.h).
// Build: g++ -std=c++17 -Isrc src/theme_smoke.cpp -o theme_smoke && ./theme_smoke
#include "skin_theme.h"
#include <cassert>
#include <cstdio>
using namespace aztec;
int main(){
  assert(theme::themes().size()==7);
  assert(skin::kDefaultTheme==6);
  assert(theme::findTheme("purple")==6);
  assert(skin::purple().at(skin::kAccent).g==241);
  assert(theme::findTheme("aurora")==4);
  assert(skin::themeIndex("aurora")==4);
  assert(theme::parseSkinText("theme = aurora\n"));
  assert(theme::currentTheme()==4);
  auto p=theme::current();
  assert(p.at(skin::kAccent).r==156&&p.at(skin::kAccent).g==250);   // sampled from NEWGUI.jpeg
  assert(p.at(skin::kBgDeep).r==3&&p.at(skin::kBgDeep).b==10);
  assert(p.at(skin::kReadout).r==114&&p.at(skin::kReadout).g==244);
  assert(p.at(skin::kMeter).r==232);                                 // amber inherited from astral
  assert(p.at(skin::kAccentTrack).r==skin::clamp255(int(156*.23+.5)));               // derived shades recompute
  assert(!theme::customized());
  assert(theme::parseSkinText("theme = aurora\naccent = #FF9240\n"));
  assert(theme::customized());
  assert(theme::current().at(skin::kAccent).r==255);
  // Unpinned derived shades follow the new accent automatically.
  assert(theme::current().at(skin::kAccentTrack).r==skin::clamp255(int(255*.23+.5)));
  assert(theme::skinLabel()=="Aurora Lime *");
  const char* path="/tmp/grainsdosage-theme-smoke.txt";
  assert(theme::saveSkinFile(path));                 // save the aurora+accent skin first
  theme::applySkin(0,{});
  theme::loadSkinFile(path);
  assert(theme::currentTheme()==4);                  // round-trip preserved
  assert(theme::current().at(skin::kAccent).r==255);
  // BUG FIX: explicitly-set derived roles are PINNED — finalize() must not
  // overwrite them with auto-calculated values.
  assert(theme::parseSkinText("theme = astral\naccent_track = #102030\n"));
  assert(theme::current().at(skin::kAccentTrack).r==0x10);
  assert(theme::current().at(skin::kAccentTrack).g==0x20);
  assert(theme::current().at(skin::kAccentTrack).b==0x30);
  assert(theme::parseSkinText("theme = astral\nmeter_low = #403020\n"));
  assert(theme::current().at(skin::kMeterLow).r==0x40);
  // Renderer-facing active palette stays in sync with theme state.
  assert(skin::active().at(skin::kMeterLow).r==0x40);
  assert(skin::activeTheme()==0&&skin::activeCustomized());
  theme::loadSkinFile("assets/aurora-skin.txt");                     // shipped sampled skin loads
  assert(theme::currentTheme()==4);
  assert(theme::current().at(skin::kReadout).r==114);
  std::printf("ALL THEME TESTS PASSED\n");
  return 0;
}

