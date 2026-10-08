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
#include <chrono>

namespace aztec { namespace mockup {
constexpr double width=1632, height=1518;
struct Rect {double x,y,w,h; bool contains(double px,double py)const{return px>=x&&py>=y&&px<x+w&&py<y+h;}};
constexpr Rect preset{532,31,364,43}, previous{911,31,50,43}, next{964,31,50,43};
constexpr Rect load{1032,31,105,43}, save{1150,31,105,43};
constexpr Rect skinMenu{1040,1466,142,31}, zoomMenu{1192,1466,50,31};
constexpr Rect xy{1112,735,480,260}, scope{40,775,390,250}, wave{27,510,1331,112};
inline Rect randomRect(int tab){return {918,tab==3?576.:tab==2?389.:421.,tab==3?230.:195.,42};}
inline Rect tabRect(int i){return {28.+i*269,115,263,50};}
inline Rect lfoRect(int i){return {628.+i*105,680,90,46};}
inline Rect stepRect(int i,int tab=2){return {57.+i*81,tab==3?354.:tab==4?464.:450.,44,tab==3?88.:tab==4?108.:74.};}
constexpr ParamID stageEnabled[]={kGrainEnabled,kGlitchEnabled,kRepeatEnabled,kResliceEnabled,kGaterEnabled};
inline Rect ledRect(int i){auto r=tabRect(i);return {r.x+r.w-28,r.y+8,24,r.h-16};}
inline int hitLed(double x,double y){for(int i=0;i<5;++i)if(ledRect(i).contains(x,y))return i;return -1;}
inline int hitTab(double x,double y){for(int i=0;i<5;++i)if(tabRect(i).contains(x,y))return i;return -1;}
inline int hitLfo(double x,double y){for(int i=0;i<4;++i)if(lfoRect(i).contains(x,y))return i;return -1;}
inline int hitStep(double x,double y,int tab=2){for(int i=0;i<16;++i)if(stepRect(i,tab).contains(x,y))return i;return -1;}
using Value=std::function<double(ParamID)>;
using Display=std::function<std::string(ParamID,double)>;
// Shared insertion semantics: moving a stage shifts its neighbours, never swaps.
inline int hitRoute(double x,double y){return hitTab(x,y);}
inline int routeInsertion(double x,double y){if(y<108||y>173||x<20||x>1377)return -1;for(int i=0;i<5;++i)if(x<tabRect(i).x+tabRect(i).w/2)return i;return 5;}
inline std::array<int,5> routeChain(const Value& value){int n=int(std::round(value(kRoutingOrder)*120))-1;if(n>=0)return fiveModuleOrder(n);std::array<int,5> c{{0,1,2,3,4}};int old=int(std::round(value(kModuleOrder)*6));if(old<6){auto a=moduleOrders[std::clamp(old,0,5)];std::copy(a.begin(),a.end(),c.begin());}return c;}
inline double moveRoute(std::array<int,5> c,int from,int insertion){int to=insertion-(insertion>from?1:0);int stage=c[from];if(to>from)for(int i=from;i<to;++i)c[i]=c[i+1];else for(int i=from;i>to;--i)c[i]=c[i-1];c[to]=stage;for(int i=0;i<120;++i)if(fiveModuleOrder(i)==c)return (i+1)/120.;return 0;}
struct Motion {
 std::array<double,5> x{};std::array<double,16> gate{};bool ready=false;
 std::chrono::steady_clock::time_point last{};
 void update(const Value& value,int drag,int insertion){
  auto now=std::chrono::steady_clock::now();double dt=ready?std::clamp(std::chrono::duration<double>(now-last).count(),0.,.1):0.;last=now;
  auto chain=routeChain(value);if(drag>=0&&insertion>=0)chain=fiveModuleOrder(int(std::round(moveRoute(chain,drag,insertion)*120))-1);
  double ease=1-std::exp(-dt/.07);
  for(int i=0;i<5;++i){double target=tabRect(i).x;int stage=chain[i];x[stage]=ready?x[stage]+ease*(target-x[stage]):target;}
  int playing=int(std::round(value(kUiGaterStep)*15));
  for(int i=0;i<16;++i){double target=i==playing&&value(kGaterEnabled)>.5?std::min(value(kUiGaterPhase),value(kUiGaterLength0+i)):0.;gate[i]+=(1-std::exp(-dt/.025))*(target-gate[i]);}
  ready=true;
 }
};
struct WaveVisual {
 std::array<double,waveformBins> lo{},hi{};
 double active=0;
 void update(const Value& value){for(int i=0;i<waveformBins;++i){lo[i]+=.06*((value(kUiWaveLow0+i)*2-1)-lo[i]);hi[i]+=.06*((value(kUiWaveHigh0+i)*2-1)-hi[i]);}active+=.06*((value(kUiGrainActive)>.5?1.:0.)-active);}
};
using skin::Rgb;
struct Painter {
 std::function<void(Rect,Rgb,Rgb,double)> box;
 std::function<void(const std::string&,Rect,double,Rgb,bool)> text;
 std::function<void(double,double,double,double,Rgb,double)> line;
 std::function<void(Rect,double)> knob;
 std::function<void(const std::vector<std::pair<double,double>>&,Rgb)> polygon;
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
 if(c.id==kRoutingOrder){
  int order=int(std::round(v*120))-1;auto chain=order>=0?fiveModuleOrder(order):std::array<int,5>{{0,1,2,3,4}};
  bool parallel=false;if(order<0){int old=int(std::round(value(kModuleOrder)*6));parallel=old==6;if(!parallel){auto legacy=moduleOrders[std::clamp(old,0,5)];for(int i=0;i<3;++i)chain[i]=legacy[i];}}
  const char* names[]={"GRN","PRE","RPT","RSL","GAT"};std::string label;
  for(int i=0;i<5;++i){if(i)label+=(parallel&&i<3)?" + ":" > ";label+=names[chain[i]];}
  p.box(r,screen,edge,r.h/2);p.text(label,{r.x+6,r.y,r.w-12,r.h},16,white,true);return;
 }
 if(c.kind==Knob){
  const bool large=r.h>165;const double diameter=large?118:90;
  p.text(c.label,{r.x,r.y,r.w,large?30.:24.},large?24:18,white,true);
  p.knob({r.x+(r.w-diameter)/2,r.y+(large?32.:30.),diameter,diameter},v);
  double readWidth=large?r.w-20:106;Rect readout{r.x+(r.w-readWidth)/2,r.y+r.h-27,readWidth,27};
  p.box(readout,ink,edge,14);p.text(display(c.id,v),readout,large?18:16,white,true);
 }else if(c.kind==Toggle){
  p.box(r,screen,edge,std::min(18.,r.h/2));
  bool on=v>=.5;std::string title=c.label;
  if(c.label=="ON")title=on?"ON":"OFF";
  if(c.id==kFreeze&&on)title="FROZEN";
  double dot=r.h>35?8:6;
  p.box({r.x+std::min(26.,r.w*.13),r.y+(r.h-dot)/2,dot,dot},on?accent:muted,on?accent:muted,dot/2);
  p.text(title,{r.x+20,r.y,r.w-26,r.h},c.id==kStretchOn?18:(r.h>35?24:16),white,true);
 }else if(c.kind==Select){
  p.box(r,screen,edge,r.h/2);
  double shown=c.id==lfoID(lfo,lWave)&&value(kModWaveRnd0+lfo)>0?value(kUiModWave0+lfo):v;
  auto shownText=display(c.id,shown);if(c.id==kReverbRateV2&&v<.5/6){int old=value(kReverbSource)>=.5?kReverbRandomRate:kReverbGrid;shownText=display(old,value(old));}
  if(c.label.empty())p.text(shownText,{r.x+8,r.y,r.w-16,r.h},18,white,true);
  else{double split=c.label=="DEST"?50.:std::min(r.w*.44,118.);p.line(r.x+split,r.y+1,r.x+split,r.y+r.h-1,edge,1);
   p.text(c.label,{r.x+4,r.y,split-8,r.h},r.h>35?18:14,muted,true);
   p.text(shownText,{r.x+split+3,r.y,r.w-split-7,r.h},r.h>35?22:16,white,true);}
 }else if(c.kind==Slider||c.kind==Pan){
  // Compact routing amounts use segmented meters, like the mockup.
  if(c.id>=kLfoSlots0&&c.id<kLfoSpeed0){
   double amount=std::abs(v-.5)*2.;double meterWidth=r.w-54;
   for(int i=0;i<20;++i){Rgb col=i<amount*20?accent:skin::C(skin::kHairline);p.box({r.x+i*meterWidth/20,r.y+5,meterWidth/20-3,r.h-10},col,col,0);}
   p.text(std::to_string(int(std::round(amount*100)))+"%",{r.x+meterWidth+6,r.y,48,r.h},16,white,true);return;
  }
  bool compact=r.h<=46;double ty=r.y+(c.label.empty()?r.h/2:(compact?r.h-11:31.));
  if(!c.label.empty())p.text(c.label,{r.x,r.y,r.w,18},15,white,false);
  double track=r.w-(compact?48:8);track=std::max(20.,track);
  p.box({r.x,ty-5,track,10},ink,skin::C(skin::kBorderDark),5);
  p.box({r.x+4,ty-2,track-8,3},skin::C(skin::kHairline),skin::C(skin::kHairline),1);
  double knobX=r.x+5+v*(track-10);
  double thumb=c.label.empty()?std::min(26.,r.h-2):20.;
  p.box({knobX-9,ty-thumb/2,18,thumb},white,accent,4);
  p.box({knobX-2,ty-thumb/2+3,4,thumb-6},ink,ink,1);
  if(compact)p.text(display(c.id,v),{r.x+track+4,ty-12,44,24},14,white,false);
  else p.text(display(c.id,v),{r.x,r.y+r.h-18,r.w,18},15,white,false);
 }else if(c.kind==Pad){
  bool on=v>=.5;p.box(r,on?accent:ink,on?skin::C(skin::kAccentBright):skin::C(skin::kBorderDark),r.w/2);
  if(c.id>=kReverbStep0&&c.id<kReverbStep0+16&&int(std::round(value(kUiReverb)*15))==int(c.id-kReverbStep0))
   p.line(r.x+7,r.y+r.h-7,r.x+r.w-7,r.y+r.h-7,white,2);
 }
}
inline void render(Painter& p,const Value& value,const Display& display,const std::string& presetName,int tab,int lfo,int repeat,int gate,int reslice,WaveVisual* visual=nullptr,int drag=-1,int insertion=-1,Motion* motion=nullptr){
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
 plate({15,103,1365,541});
 const char* tabs[]={"GRANULIZER","PRESLICER","BEAT REPEATER","RESLICE","GATER"};
 if(motion)motion->update(value,drag,insertion);
 auto chain=routeChain(value);
 int moving=drag>=0?chain[drag]:-1;
 if(drag>=0&&insertion>=0&&insertion!=drag&&insertion!=drag+1)
  chain=fiveModuleOrder(int(std::round(moveRoute(chain,drag,insertion)*120.))-1);
 for(int i=0;i<5;++i){int stage=chain[i];Rect r=tabRect(i);if(motion)r.x=motion->x[stage];bool selected=stage==tab;
  p.box(r,selected?panel:skin::C(skin::kPanelRaised),stage==moving?accent:(selected?edge:border),12);
  p.text(tabs[stage],{r.x+5,r.y,r.w-37,r.h},21,white,true);
  auto led=ledRect(i);led.x+=r.x-tabRect(i).x;bool on=value(stageEnabled[stage])>=.5;
  p.box({led.x+6,led.y+(led.h-10)/2,10,10},on?accent:skin::C(skin::kHairline),on?accent:muted,5);
 }
 if(drag>=0&&insertion>=0){double x=insertion==5?tabRect(4).x+tabRect(4).w+3:tabRect(insertion).x-3;p.line(x,118,x,162,accent,3);}
 p.box({1384,103,234,541},skin::C(skin::kPanelRaised),edge,16);
 p.box({1391,115,220,46},panel,edge,18);p.text("MASTER OPTIONS",{1391,115,220,46},19,white,true);
 button(randomRect(tab),tab==3?"RANDOM ONCE":"RANDOM");
 if(tab==0){
  p.box(wave,screen,edge,18);
  WaveVisual direct;if(!visual){for(int i=0;i<waveformBins;++i){direct.lo[i]=value(kUiWaveLow0+i)*2-1;direct.hi[i]=value(kUiWaveHigh0+i)*2-1;}direct.active=value(kUiGrainActive);visual=&direct;}
  const double scale=wave.h/2-6,mid=wave.y+wave.h/2;
  auto envelope=[&](int first,int last){std::vector<std::pair<double,double>> pts;for(int i=first;i<=last;++i)pts.push_back({wave.x+3+i*(wave.w-6)/(waveformBins-1),mid-visual->hi[i]*scale});for(int i=last;i>=first;--i)pts.push_back({wave.x+3+i*(wave.w-6)/(waveformBins-1),mid-visual->lo[i]*scale});return pts;};
  auto base=skin::mix(screen,muted,.35);p.polygon(envelope(0,waveformBins-1),base);
  p.line(wave.x+3,mid,wave.x+wave.w-3,mid,skin::C(skin::kHairline),1);
  if(visual->active>.01){double start=std::min(value(kUiGrainStart),value(kUiGrainEnd)),end=std::max(value(kUiGrainStart),value(kUiGrainEnd));
   int first=std::clamp(int(start*(waveformBins-1)),0,waveformBins-1),last=std::clamp(int(std::ceil(end*(waveformBins-1))),first,waveformBins-1);
   auto lit=skin::mix(base,accent,.45*visual->active);p.polygon(envelope(first,last),lit);
   double x1=wave.x+3+start*(wave.w-6),x2=wave.x+3+end*(wave.w-6);
   p.line(x1,wave.y+wave.h-7,x2,wave.y+wave.h-7,lit,3);
   double x=wave.x+3+value(kUiGrainHead)*(wave.w-6);p.line(x,wave.y+6,x,wave.y+wave.h-6,lit,1.5);
  }

 }else if(tab==1){
  p.text("SLICE LENGTH / TRIGGER INTERVAL / PROBABILITY",{52,510,1150,36},24,white,true);
  p.text(value(kUiGlitch)>.5?"PROCESSING SLICE":"WAITING FOR TRIGGER",{52,569,1150,36},22,accent,true);
 }else{
  for(int i=0;i<16;++i){Rect r=stepRect(i,tab);bool on=false,selected=false;std::string s=std::to_string(i+1);
   if(tab==2){on=value(kRepeatStep0+i)>.5;selected=i==repeat;}
   if(tab==3){on=value(kResliceStep0+i)>.5;selected=i==reslice;s+=" > "+std::to_string(1+int(std::round(value(value(kResliceRndOn)>.5?kUiResliceSource0+i:kResliceIndex0+i)*15)));}
   if(tab==4){on=value(value(kGaterStepRnd)>.5?kUiGaterState0+i:kGaterState0+i)>.25;selected=i==gate;}
   if(tab==4){
    p.box(r,ink,selected?white:border,12);
    double length=std::clamp(value(kUiGaterLength0+i),.05,1.);if(value(kGaterLengthRnd)<.5)length=.05+.95*value(kGaterLength0+i);
    double h=(r.h-8)*length;
    if(on)p.box({r.x+4,r.y+r.h-4-h,r.w-8,h},skin::mix(ink,accent,.25),skin::mix(ink,accent,.25),8);
    double progress=motion?motion->gate[i]:(i==int(std::round(value(kUiGaterStep)*15))?std::min(value(kUiGaterPhase),length):0.);
    if(on&&progress>.001){double rise=(r.h-8)*progress;p.box({r.x+4,r.y+r.h-4-rise,r.w-8,rise},accent,accent,8);}
   }else p.box(r,on?accent:ink,selected?white:(on?skin::C(skin::kAccentBright):border),r.w/2);
   p.text(s,{r.x-20,r.y+r.h+5,r.w+40,26},tab==3?16:20,white,true);
   int play=tab==2?int(std::round(value(kUiStep)*15)):tab==3?int(std::round(value(kUiResliceStep)*15)):int(std::round(value(kUiGaterStep)*15));
   if(i==play)p.line(r.x+6,r.y+r.h-8,r.x+r.w-6,r.y+r.h-8,white,2);
   if(tab==4&&value(kGaterRelease0+i)>.5)p.box({r.x+9,r.y+10,14,14},skin::C(skin::kRelease),white,7);

  }
  if(tab==4)p.text("CLICK: WET / OFF     SHIFT-CLICK: LATCH RELEASE",{50,612,1250,26},17,white,true);
 }
 plate({15,662,1038,451});plate({1072,662,546,451});heading("MODULATION",{58,680,330,35});
 for(int i=0;i<4;++i){Rect r=lfoRect(i);p.box(r,screen,edge,18);p.text(std::to_string(i+1),r,24,i==lfo?white:muted,true);}
 p.box(scope,screen,edge,16);
 qg::Lfo preview;double cycle=std::round(value(kUiLfoCycle0+lfo)*4294967295.-2147483648.);
 int64_t epoch=int64_t(std::round(value(kUiLfoEpoch0+lfo)*4294967295.-2147483648.));
 preview.prepare(0x13579BDFULL+uint64_t(lfo)*104729+(value(lfoID(lfo,lReset))>=.5?uint64_t(epoch)*0x9e3779b97f4a7c15ULL:0));
 qg::LfoSettings shape;shape.enabled=true;shape.beats=1.;shape.wave=int(std::round(value(value(kModWaveRnd0+lfo)>0?kUiModWave0+lfo:lfoID(lfo,lWave))*129.));
 shape.randomSteps=1+int(std::round(value(kRandomSteps0+lfo)*63.));shape.depth=value(lfoID(lfo,lDepth));shape.phase=0;shape.glide=.01+.99*value(lfoID(lfo,lGlide));
 double lastX=0,lastY=0;for(int i=0;i<=210;++i){double x=scope.x+5+i*(scope.w-10)/210,y=scope.y+scope.h/2-preview.process(shape,cycle+i/210.,48000.,false,0)*(scope.h/2-8);if(i)p.line(lastX,lastY,x,y,accent,2);lastX=x;lastY=y;}
 double cursor=scope.x+5+std::clamp(value(kUiLfoPhase0+lfo),0.,1.)*(scope.w-10);
 p.line(cursor,scope.y+5,cursor,scope.y+scope.h-5,skin::mix(screen,white,.65),1.5);
 heading("MORPH XY",{1112,680,300,35});p.box(xy,screen,edge,16);
 p.line(xy.x+xy.w/2,xy.y,xy.x+xy.w/2,xy.y+xy.h,skin::C(skin::kHairline),1);
 p.line(xy.x,xy.y+xy.h/2,xy.x+xy.w,xy.y+xy.h/2,skin::C(skin::kHairline),1);
 double px=xy.x+12+value(kXYX)*(xy.w-24),py=xy.y+12+(1-value(kXYY))*(xy.h-24);
 p.box({px-12,py-12,24,24},accent,white,12);
 plate({15,1130,1603,180});p.line(766,1131,766,1308,border,4);
 heading("FILTER",{60,1138,90,36});p.text("FILTER SEQUENCER",{791,1138,225,36},20,white,false);
 for(int i=0;i<32;++i){double x=1018+i*18.;Rect r{x,1209,15,82};bool active=value(kFilterSeqOn)>.5&&i==int(std::round(value(kUiFilterSeqStep)*31.));
  p.box(r,ink,border,8);double n=(qg::filterPattern(int(std::round(value(kFilterSeqPattern)*63)),i)+1)*.5;
  if(value(kFilterSeqOn)>.5)p.box({x+3,1285-n*65,9,5+n*65},active?accent:skin::mix(accent,ink,.4),active?white:skin::mix(accent,ink,.4),4);}
 plate({15,1326,1603,110});p.text("REVERB",{58,1337,94,36},20,white,false);
 plate({15,1453,1603,56});heading("OUTPUT",{60,1465,130,35});
 for(const auto& c:controls(value,tab,lfo,repeat,gate,reslice))renderControl(p,c,value,display,lfo);
 p.box(skinMenu,ink,edge,16);p.text("SKIN",skinMenu,16,white,true);
 p.text("SIZE",zoomMenu,16,white,true);
}
}} // namespace aztec::mockup
