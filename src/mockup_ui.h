#pragma once
// Shared visual and hit-test geometry for the supplied October 2026 mockup.
// Audio parameters remain in parameters.h; this file only renders their values.
#include "parameters.h"
#include "skin_theme.h"
#include "filter_sequencer.h"
#include "modulation.h"
#include <algorithm>
#include <functional>
#include <string>
#include <vector>

namespace aztec { namespace mockup {
constexpr double width=1632, height=1518;
struct Rect {double x,y,w,h; bool contains(double px,double py)const{return px>=x&&py>=y&&px<x+w&&py<y+h;}};
constexpr Rect preset{532,31,364,43}, previous{911,31,50,43}, next{964,31,50,43};
constexpr Rect load{1032,31,105,43}, save{1150,31,105,43};
constexpr Rect skinMenu{1040,1466,203,31}, zoomMenu{1252,1466,104,31};
constexpr Rect xy{1368,868,224,224}, scope{40,944,296,115}, wave{42,602,1220,189};
constexpr Rect random{918,503,195,46};
inline Rect tabRect(int i){return {16.+i*256,103,250,65};}
inline Rect lfoRect(int i){return {628.+i*105,860,90,46};}
inline Rect stepRect(int i){return {42.+i*76,610,70,80};}
inline int hitTab(double x,double y){for(int i=0;i<5;++i)if(tabRect(i).contains(x,y))return i;return -1;}
inline int hitLfo(double x,double y){for(int i=0;i<4;++i)if(lfoRect(i).contains(x,y))return i;return -1;}
inline int hitStep(double x,double y){for(int i=0;i<16;++i)if(stepRect(i).contains(x,y))return i;return -1;}
using Value=std::function<double(ParamID)>;
using Display=std::function<std::string(ParamID,double)>;
using skin::Rgb;
struct Painter {
 std::function<void(Rect,Rgb,Rgb,double)> box;
 std::function<void(const std::string&,Rect,double,Rgb,bool)> text;
 std::function<void(double,double,double,double,Rgb,double)> line;
 std::function<void(Rect,double)> knob;
};
enum Kind{Knob,Slider,Toggle,Select,Pad,Pan,PanMode};
struct Control{ParamID id;Rect r;Kind kind;std::string label;};
inline std::vector<Control> controls(const Value& value,int currentTab,int selectedLfo,int selectedRepeat,int selectedGate,int selectedReslice){
 std::vector<Control> result;
 #define ADD(ID,X,Y,W,H,K,L) result.push_back({ParamID(ID),{double(X),double(Y),double(W),double(H)},K,L})
 #include "editor_layout_win.inl"
 #undef ADD
 return result;
}
inline void renderControl(Painter& p,const Control& c,const Value& value,const Display& display,int lfo){
 const auto r=c.r;const double v=std::clamp(value(c.id),0.,1.);
 const auto white=skin::C(skin::kCream),muted=skin::C(skin::kMuted),accent=skin::C(skin::kAccent);
 const auto ink=skin::C(skin::kWell),edge=skin::C(skin::kFiligree),screen=skin::C(skin::kPanelInset);
 if(c.kind==Knob){
  const bool large=r.h>180;const double diameter=large?160:90;
  p.text(c.label,{r.x,r.y,r.w,large?30.:24.},large?24:18,white,true);
  p.knob({r.x+(r.w-diameter)/2,r.y+(large?42.:30.),diameter,diameter},v);
  Rect readout{r.x+(r.w-106)/2,r.y+r.h-27,106,27};
  p.box(readout,ink,edge,14);p.text(display(c.id,v),readout,large?20:18,white,true);
 }else if(c.kind==Toggle){
  p.box(r,screen,edge,std::min(18.,r.h/2));
  bool on=v>=.5;std::string title=c.label;
  if(c.label=="ON")title=on?"ON":"OFF";
  if(c.id==kFreeze&&on)title="FROZEN";
  double dot=r.h>35?8:6;
  p.box({r.x+std::min(26.,r.w*.13),r.y+(r.h-dot)/2,dot,dot},on?accent:muted,on?accent:muted,dot/2);
  p.text(title,{r.x+20,r.y,r.w-26,r.h},r.h>35?24:16,white,true);
 }else if(c.kind==Select){
  p.box(r,screen,edge,r.h/2);
  double shown=c.id==lfoID(lfo,lWave)&&value(kModWaveRnd0+lfo)>0?value(kUiModWave0+lfo):v;
  if(c.label.empty())p.text(display(c.id,shown),{r.x+8,r.y,r.w-16,r.h},18,white,true);
  else{double split=c.label=="DEST"?50.:std::min(r.w*.44,118.);p.line(r.x+split,r.y+1,r.x+split,r.y+r.h-1,edge,1);
   p.text(c.label,{r.x+4,r.y,split-8,r.h},r.h>35?18:14,muted,true);
   p.text(display(c.id,shown),{r.x+split+3,r.y,r.w-split-7,r.h},r.h>35?22:16,white,true);}
 }else if(c.kind==Slider||c.kind==Pan){
  // Compact routing amounts use segmented meters, like the mockup.
  if(c.id>=kLfoSlots0&&c.id<kLfoSpeed0){
   double amount=std::abs(v-.5)*2.;for(int i=0;i<10;++i){Rgb col=i<amount*10?accent:muted;p.box({r.x+i*r.w/10,r.y+4,r.w/10-3,r.h-8},col,col,0);}return;
  }
  bool compact=r.h<=46;double ty=r.y+(c.label.empty()?r.h/2:std::min(r.h-14,29.));
  if(!c.label.empty())p.text(c.label,{r.x,r.y,r.w,20},16,white,false);
  double track=r.w-(compact?48:8);track=std::max(20.,track);
  p.box({r.x,ty-5,track,10},ink,skin::C(skin::kBorderDark),5);
  p.box({r.x+4,ty-2,track-8,3},skin::C(skin::kHairline),skin::C(skin::kHairline),1);
  double knobX=r.x+5+v*(track-10);
  double thumb=std::min(30.,r.h-2);
  p.box({knobX-6,ty-thumb/2,12,thumb},ink,skin::C(skin::kBorderDark),6);
  p.box({knobX-2,ty-thumb/2+5,4,thumb-10},edge,edge,2);
  if(compact)p.text(display(c.id,v),{r.x+track+4,ty-12,44,24},14,white,false);
  else p.text(display(c.id,v),{r.x,r.y+r.h-20,r.w,20},15,white,false);
 }else if(c.kind==Pad){
  bool on=v>=.5;p.box(r,on?accent:ink,on?skin::C(skin::kAccentBright):skin::C(skin::kBorderDark),r.w/2);
  if(c.id>=kReverbStep0&&c.id<kReverbStep0+16&&int(std::round(value(kUiReverb)*15))==int(c.id-kReverbStep0))
   p.line(r.x+7,r.y+r.h-7,r.x+r.w-7,r.y+r.h-7,white,2);
 }
}
inline void render(Painter& p,const Value& value,const Display& display,const std::string& presetName,int tab,int lfo,int repeat,int gate,int reslice){
 auto panel=skin::C(skin::kPanel),edge=skin::C(skin::kFiligree),border=skin::C(skin::kBorderDark);
 auto white=skin::C(skin::kCream),muted=skin::C(skin::kMuted),ink=skin::C(skin::kWell),screen=skin::C(skin::kPanelInset),accent=skin::C(skin::kAccent);
 auto plate=[&](Rect r){p.box(r,panel,border,16);p.line(r.x+14,r.y+3,r.x+r.w-14,r.y+3,skin::C(skin::kFiligreeDim),1);};
 auto heading=[&](const char* s,Rect r){p.text(s,r,24,white,false);};
 auto button=[&](Rect r,const std::string& s){p.box(r,ink,edge,18);p.text(s,r,24,white,true);};
 p.box({0,0,width,height},skin::C(skin::kBgDeep),skin::C(skin::kBgDeep),0);
 plate({15,10,1280,82});plate({1298,10,320,82});
 p.text("GRAINSDOSAGE",{40,18,475,62},48,white,false);
 button(preset,presetName);button(previous,"<");button(next,">");button(load,"LOAD");button(save,"SAVE");
 p.text("INPUT DE-CLICKER",{1315,19,286,29},22,white,true);
 plate({15,151,1280,672});
 const char* tabs[]={"GRANULIZER","PRESLICER","BEAT REPEATER","RESLICE","GATER"};
 for(int i=0;i<5;++i){Rect r=tabRect(i);plate(r);p.text(tabs[i],{r.x+3,r.y+13,r.w-6,35},24,white,true);
  if(tab==i)p.box({r.x+3,r.y+r.h-6,r.w-6,14},panel,panel,0);}
 p.box({1302,103,312,720},skin::C(skin::kPanelRaised),edge,16);
 p.box({1309,115,297,46},panel,edge,18);p.text("MASTER OPTIONS",{1309,115,297,46},24,white,true);
 button(random,tab==3?"RANDOM ONCE":"RANDOM");
 if(tab==0){
  p.box(wave,screen,edge,18);
  double peak=.02;for(int i=0;i<128;++i)peak=std::max(peak,value(kUiWave0+i));
  for(int i=0;i<128;++i){double h=std::max(1.,value(kUiWave0+i)/peak*(wave.h-14));
   bool live=value(kUiGrainActive)>.5&&(i+1.)/128>=value(kUiGrainStart)&&i/128.<=value(kUiGrainEnd);
   p.line(wave.x+4+i*(wave.w-8)/128,wave.y+(wave.h-h)/2,wave.x+4+i*(wave.w-8)/128,wave.y+(wave.h+h)/2,live?accent:skin::C(skin::kFaint),10);}
  if(value(kUiGrainActive)>.5){double x=wave.x+value(kUiGrainHead)*wave.w;p.line(x,wave.y+4,x,wave.y+wave.h-4,white,1);}
 }else if(tab==1){
  p.text("SLICE LENGTH / TRIGGER INTERVAL / PROBABILITY",{52,621,1150,36},24,white,true);
  p.text(value(kUiGlitch)>.5?"PROCESSING SLICE":"WAITING FOR TRIGGER",{52,690,1150,36},22,accent,true);
 }else{
  for(int i=0;i<16;++i){Rect r=stepRect(i);bool on=false,selected=false;std::string s=std::to_string(i+1);
   if(tab==2){on=value(kRepeatStep0+i)>.5;selected=i==repeat;}
   if(tab==3){on=value(kResliceStep0+i)>.5;selected=i==reslice;s+=" > "+std::to_string(1+int(std::round(value(value(kResliceRndOn)>.5?kUiResliceSource0+i:kResliceIndex0+i)*15)));}
   if(tab==4){on=value(value(kGaterStepRnd)>.5?kUiGaterState0+i:kGaterState0+i)>.25;selected=i==gate;}
   p.box(r,on?skin::C(skin::kAccentGlow):ink,selected?white:edge,16);p.text(s,{r.x,r.y+6,r.w,25},16,white,true);
   if(tab==2)p.text(display(kRepeatRate0+i,value(kRepeatRate0+i)),{r.x+2,r.y+40,r.w-4,25},14,white,true);
   if(tab==4){p.text(value(kGaterRelease0+i)>.5?"REL":on?"WET":"OFF",{r.x,r.y+40,r.w,24},15,on?accent:muted,true);}
  }
  if(tab==4)p.text("CLICK: WET / OFF     SHIFT-CLICK: LATCH RELEASE",{50,725,1150,40},20,white,true);
 }
 plate({15,843,1038,270});plate({1072,843,546,270});heading("MODULATION",{58,863,330,35});
 for(int i=0;i<4;++i){Rect r=lfoRect(i);p.box(r,screen,edge,18);p.text(std::to_string(i+1),r,24,i==lfo?white:muted,true);}
 p.box(scope,screen,edge,16);
 qg::Lfo preview;double cycle=std::round(value(kUiLfoCycle0+lfo)*4294967295.-2147483648.);
 int64_t epoch=int64_t(std::round(value(kUiLfoEpoch0+lfo)*4294967295.-2147483648.));
 preview.prepare(0x13579BDFULL+uint64_t(lfo)*104729+(value(lfoID(lfo,lReset))>=.5?uint64_t(epoch)*0x9e3779b97f4a7c15ULL:0));
 qg::LfoSettings shape;shape.enabled=true;shape.beats=1.;shape.wave=int(std::round(value(value(kModWaveRnd0+lfo)>0?kUiModWave0+lfo:lfoID(lfo,lWave))*129.));
 shape.randomSteps=1+int(std::round(value(kRandomSteps0+lfo)*63.));shape.depth=value(lfoID(lfo,lDepth));shape.phase=0;shape.glide=.01+.99*value(lfoID(lfo,lGlide));
 double lastX=0,lastY=0;for(int i=0;i<=210;++i){double x=scope.x+5+i*(scope.w-10)/210,y=scope.y+scope.h/2-preview.process(shape,cycle+i/210.,48000.,false,0)*(scope.h/2-8);if(i)p.line(lastX,lastY,x,y,accent,2);lastX=x;lastY=y;}
 heading("MORPH XY",{1112,863,240,35});p.box(xy,screen,edge,16);
 p.line(xy.x+xy.w/2,xy.y,xy.x+xy.w/2,xy.y+xy.h,skin::C(skin::kHairline),1);
 p.line(xy.x,xy.y+xy.h/2,xy.x+xy.w,xy.y+xy.h/2,skin::C(skin::kHairline),1);
 double px=xy.x+12+value(kXYX)*(xy.w-24),py=xy.y+12+(1-value(kXYY))*(xy.h-24);
 p.box({px-12,py-12,24,24},accent,white,12);
 plate({15,1130,1603,180});p.line(766,1131,766,1308,border,4);
 heading("FILTER",{60,1138,90,36});p.text("FILTER SEQUENCER",{791,1138,225,36},20,white,false);
 for(int i=0;i<32;++i){double x=1018+i*18.;Rect r{x,1209,15,82};bool active=value(kFilterSeqOn)>.5&&i==int(std::round(value(kUiFilterSeqStep)*31.));
  p.box(r,ink,border,8);double n=(qg::filterPattern(int(std::round(value(kFilterSeqPattern)*63)),i)+1)*.5;
  if(value(kFilterSeqOn)>.5)p.box({x+3,1285-n*65,9,5+n*65},active?accent:skin::C(skin::kAccentTrack),active?accent:skin::C(skin::kAccentTrack),4);}
 plate({15,1326,1603,110});p.text("REVERB",{58,1337,94,36},20,white,false);
 plate({15,1453,1603,56});heading("OUTPUT",{60,1465,130,35});
 for(const auto& c:controls(value,tab,lfo,repeat,gate,reslice))renderControl(p,c,value,display,lfo);
 button(skinMenu,skin::activeTheme()==6?"SKIN   DEFAULT":"SKIN   "+theme::skinLabel());
 p.text("SIZE",zoomMenu,16,white,true);
}
}} // namespace aztec::mockup
