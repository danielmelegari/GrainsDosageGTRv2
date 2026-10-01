#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// GrainsDosage — runtime skin/theme system (customizable GUI).
//
// The compile-time palettes live in skin_spec.h. This header adds the part the
// renderers use at run time: a mutable active Palette, the five built-in
// themes, per-colour overrides loaded from a small text file, and persistence
// helpers. Both editor_mac.mm and editor_win.cpp read colours exclusively
// through aztec::theme::current()/C(), so switching a theme recolours every
// element — including shades that used to be hard-coded literals.
//
// Skin file format (key = value, '#' comments):
//   theme  = ember            # astral | ember | nebula | arctic | aurora | midnight
//   accent = #FF9240          # any colour key (skin::colorName or camelCase)
// Colours not listed fall back to the named theme; derived shades are
// recomputed by skin::finalize() so one override stays consistent everywhere.
// The file is GUI state only: it is persisted next to the user's preferences
// and never travels inside audio/VST state.
// ─────────────────────────────────────────────────────────────────────────────
#include "skin_spec.h"
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
namespace aztec {
namespace theme {

struct Entry {const char* name;const char* title;skin::Palette (*make)();};
inline const std::vector<Entry>& themes() {
  static const std::vector<Entry> list = {
    {"astral","Astral Green",skin::astral},
    {"ember","Ember Copper",skin::ember},
    {"nebula","Nebula Rose",skin::nebula},
    {"arctic","Arctic Platinum",skin::arctic},
    {"aurora","Aurora Lime",skin::aurora},
    {"midnight","Midnight Mono",skin::midnight}};
  return list;
}
inline int findTheme(const std::string& name) {
  for(size_t i=0;i<themes().size();++i)if(name==themes()[i].name)return int(i);
  return -1;
}

inline skin::Palette& current() {static skin::Palette p=skin::astral();return p;}
inline int& currentTheme() {static int i=0;return i;}
inline bool& customized() {static bool c=false;return c;}

inline std::string skinLabel() {
  int i=currentTheme();
  std::string name=(i>=0&&i<int(themes().size()))?themes()[i].title:"Astral Green";
  return customized()?name+" *":name;
}

// Resolve an override key: accepts a semantic colour name ("accent",
// "accent_track") or the camelCase field spelling ("accentTrack").
inline int keyFor(const std::string& n) {
  int k=skin::keyForName(n);if(k>=0)return k;
  for(int i=0;i<skin::kColorCount;++i){
    std::string snake=skin::colorName(i),camel;bool up=false;
    for(char ch:snake){if(ch=='_'){up=true;continue;}camel+=up?char(std::toupper(unsigned(ch))):ch;up=false;}
    if(camel==n)return i;
  }
  return -1;
}

inline bool parseHex(const std::string& v,int& r,int& g,int& b) {
  std::string s;for(char ch:v)if(!std::isspace(unsigned(ch)))s+=ch;
  size_t comment=s.find_first_of("#;");
  // "value ; trailing comment" — cut at the marker unless it starts the value
  // (the usual "#RRGGBB" spelling).
  if(comment!=std::string::npos&&comment>0)s=s.substr(0,comment);
  for(auto& ch:s)ch=char(std::tolower(unsigned(ch)));
  if(!s.empty()&&s.front()=='#')s.erase(0,1);
  if(s.size()!=6)return false;
  for(char ch:s)if(!std::isxdigit(unsigned(ch)))return false;
  long n=std::strtol(s.c_str(),nullptr,16);
  r=int((n>>16)&255);g=int((n>>8)&255);b=int(n&255);return true;
}

// Apply a skin definition on top of a base theme index. Explicitly-set derived
// roles (accent_track, meter_low, …) are PINNED: finalize() only recomputes
// the derived shades the user did not touch, so nothing gets silently
// overwritten by auto-calculated values.
inline void applySkin(int base,const std::vector<std::pair<std::string,std::string>>& entries) {
  if(base<0||base>=int(themes().size()))base=0;
  current()=themes()[size_t(base)].make();currentTheme()=base;
  bool pinned[skin::kColorCount]={false};
  bool tuned=false;
  for(auto& kv:entries) {
    if(kv.first=="theme")continue;
    int key=keyFor(kv.first);if(key<0)continue;
    int r,g,b;if(!parseHex(kv.second,r,g,b))continue;
    current().set(key,skin::Rgb{r,g,b});pinned[key]=true;tuned=true;
  }
  if(tuned)skin::finalize(current(),pinned);
  // Keep the renderer-facing active palette in sync with the theme state.
  skin::active()=current();skin::activeTheme()=currentTheme();skin::activeCustomized()=tuned;
  customized()=tuned;
}

// Parse and apply a skin file body. A later "theme" line resets any earlier
// colour edits, exactly like re-choosing a theme in the SKIN menu.
inline bool parseSkinText(const std::string& text) {
  auto trim=[](std::string s){size_t a=s.find_first_not_of(" \t\r\n");if(a==std::string::npos)return std::string();size_t b=s.find_last_not_of(" \t\r\n");return s.substr(a,b-a+1);};
  std::vector<std::pair<std::string,std::string>> entries;
  std::istringstream in(text);std::string line;
  while(std::getline(in,line)) {
    line=trim(line);
    if(line.empty()||line[0]=='#'||line[0]==';')continue;
    size_t eq=line.find('=');if(eq==std::string::npos)continue;
    std::string key=trim(line.substr(0,eq)),value=trim(line.substr(eq+1));
    for(auto& ch:key)ch=char(std::tolower(unsigned(ch)));
    entries.emplace_back(key,value);
  }
  int base=currentTheme();bool ok=true;
  std::vector<std::pair<std::string,std::string>> group;
  for(auto& kv:entries) {
    if(kv.first=="theme") {
      std::string name;for(char ch:kv.second)if(!std::isspace(unsigned(ch)))name+=char(std::tolower(unsigned(ch)));
      int idx=findTheme(name);
      if(idx<0){ok=false;continue;}
      applySkin(base,group);base=idx;group.clear();
    } else group.emplace_back(kv);
  }
  applySkin(base,group);
  return ok;
}

// Load and apply a skin file. Missing file => defaults to Astral Green.
inline void loadSkinFile(const std::string& path) {
  std::ifstream f(path,std::ios::binary);
  if(!f.is_open()){applySkin(0,{});return;}
  std::ostringstream buf;buf<<f.rdbuf();
  if(!parseSkinText(buf.str()))applySkin(0,{});
}

// Persist the live palette so the choice survives relaunches. Every colour
// role is written — including derived shades that were explicitly pinned by
// the user — because a plain "key = value" file cannot distinguish a pinned
// shade from an auto-computed one. On load those explicit values are restored
// as-is (pinned), and finalize() only fills in anything missing.
inline bool saveSkinFile(const std::string& path) {
  std::ofstream f(path);if(!f.is_open())return false;
  int i=currentTheme();
  f<<"# GrainsDosage skin — edit values or add \"key = #RRGGBB\" overrides.\n";
  f<<"theme = "<<((i>=0&&i<int(themes().size()))?themes()[i].name:"astral")<<"\n";
  if(customized())for(int k=0;k<skin::kColorCount;++k){
    char buffer[32];skin::Rgb c=current().at(k);
    std::snprintf(buffer,sizeof(buffer),"%s = #%02X%02X%02X\n",skin::colorName(k),c.r,c.g,c.b);
    f<<buffer;
  }
  return f.good();
}

// Platform default location of the persisted skin. On Windows the editor uses
// %APPDATA%\GrainsDosage\skin.txt (see editor_win.cpp); elsewhere the smoke
// tests and tools use HOME-based storage. This helper covers POSIX builds.
inline std::string defaultPath() {
#ifdef _WIN32
  return "grainsdosage-skin.txt";
#else
  const char* home=std::getenv("HOME");
  if(home&&*home)return std::string(home)+"/Library/Preferences/com.danielmelegari.grainsdosage.skin.txt";
  return "/tmp/grainsdosage-skin.txt";
#endif
}

} // namespace theme
} // namespace aztec
