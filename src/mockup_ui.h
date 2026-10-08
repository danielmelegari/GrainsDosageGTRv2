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
constexpr double contentWidth=1083, rackInset=0, width=1083, height=1452;
struct Rect {double x,y,w,h; bool contains(double px,double py)const{return px>=x&&py>=y&&px<x+w&&py<y+h;}};
constexpr Rect preset{419,41,172,39}, previous{600,42,32,38}, next{638,42,32,38};
constexpr Rect load{677,42,63,38}, save{747,42,68,38};
constexpr Rect skinMenu{884,1354,60,29}, zoomMenu{951,1354,60,29};
constexpr Rect xy{762,576,246,221}, scope{77,602,249,188}, wave{76,395,756,49};
inline Rect randomRect(int tab){return {tab==3?601.:630.,451,tab==3?148.:119.,32};}
inline Rect tabRect(int i){return {68.+i*155.2,156,150,38};}
inline Rect lfoRect(int i){return {451.+i*65,533,57,35};}
inline Rect stepRect(int i,int tab=2){return {84.+i*46.3,tab==3?329.:tab==4?369.:360.,25,tab==3?65.:tab==4?42.:44.};}
constexpr ParamID stageEnabled[]={kGrainEnabled,kGlitchEnabled,kRepeatEnabled,kResliceEnabled,kGaterEnabled};
inline Rect ledRect(int i){auto r=tabRect(i);return {r.x+r.w-23,r.y+5,20,r.h-10};}
inline int hitLed(double x,double y){for(int i=0;i<5;++i)if(ledRect(i).contains(x,y))return i;return -1;}
inline int hitTab(double x,double y){for(int i=0;i<5;++i)if(tabRect(i).contains(x,y))return i;return -1;}
inline int hitLfo(double x,double y){for(int i=0;i<4;++i)if(lfoRect(i).contains(x,y))return i;return -1;}
inline int hitStep(double x,double y,int tab=2){for(int i=0;i<16;++i)if(stepRect(i,tab).contains(x,y))return i;return -1;}
using Value=std::function<double(ParamID)>;
using Display=std::function<std::string(ParamID,double)>;
// Shared insertion semantics: moving a stage shifts its neighbours, never swaps.
inline int hitRoute(double x,double y){return hitTab(x,y);}
inline int routeInsertion(double x,double y){if(y<150||y>200||x<62||x>844)return -1;for(int i=0;i<5;++i)if(x<tabRect(i).x+tabRect(i).w/2)return i;return 5;}
inline std::array<int,5> routeChain(const Value& value){int n=int(std::round(value(kRoutingOrder)*120))-1;if(n>=0)return fiveModuleOrder(n);std::array<int,5> c{{0,1,2,3,4}};int old=int(std::round(value(kModuleOrder)*6));if(old<6){auto a=moduleOrders[std::clamp(old,0,5)];std::copy(a.begin(),a.end(),c.begin());}return c;}
inline double moveRoute(std::array<int,5> c,int from,int insertion){int to=insertion-(insertion>from?1:0);int stage=c[from];if(to>from)for(int i=from;i<to;++i)c[i]=c[i+1];else for(int i=from;i>to;--i)c[i]=c[i-1];c[to]=stage;for(int i=0;i<120;++i)if(fiveModuleOrder(i)==c)return (i+1)/120.;return 0;}
struct Motion {
 std::array<double,5> x{};std::array<double,16> gate{};std::array<double,2> meters{};bool ready=false;
 std::chrono::steady_clock::time_point last{};
 void update(const Value& value,int drag,int insertion){
  auto now=std::chrono::steady_clock::now();double dt=ready?std::clamp(std::chrono::duration<double>(now-last).count(),0.,.1):0.;last=now;
  auto chain=routeChain(value);if(drag>=0&&insertion>=0)chain=fiveModuleOrder(int(std::round(moveRoute(chain,drag,insertion)*120))-1);
  double ease=1-std::exp(-dt/.07);
  for(int i=0;i<5;++i){double target=tabRect(i).x;int stage=chain[i];x[stage]=ready?x[stage]+ease*(target-x[stage]):target;}
  int playing=int(std::round(value(kUiGaterStep)*15));
  for(int i=0;i<16;++i){double target=i==playing&&value(kGaterEnabled)>.5?std::min(value(kUiGaterPhase),value(kUiGaterLength0+i)):0.;gate[i]+=(1-std::exp(-dt/.025))*(target-gate[i]);}
  for(int ch=0;ch<2;++ch){double target=value(kUiOutputL+ch);meters[ch]+=(1-std::exp(-dt/(target>meters[ch]?.075:.3)))*(target-meters[ch]);}
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
 // Source rectangles are top-left pixel coordinates in the bundled PNG.
 std::function<void(int,Rect,Rect)> image;
 std::function<void(Rect,Rgb,Rgb,double)> box;
 std::function<void(const std::string&,Rect,double,Rgb,bool)> text;
 std::function<void(double,double,double,double,Rgb,double)> line;
 std::function<void(Rect,double)> knob;
 std::function<void(const std::vector<std::pair<double,double>>&,Rgb)> polygon;
};
enum Kind{Knob,Slider,Toggle,Select,Pad,Pan,PanMode,VSlider};
struct Control{ParamID id;Rect r;Kind kind;std::string label;};
inline std::vector<Control> controls(const Value& value,int currentTab,int selectedLfo,int selectedRepeat,int selectedGate,int selectedReslice){
 std::vector<Control> result;
 #define ADD(ID,X,Y,W,H,K,L) result.push_back({ParamID(ID),{double(X),double(Y),double(W),double(H)},K,L})
 #include "editor_layout_win.inl"
 #undef ADD
 return result;
}
enum Art {Backplate, Atlas};
enum Sprite {KnobFace, Pill, Tab, ActiveTab, Scope, XY, Meter, Track, Thumb, Led, Amber, Locked, Unlocked};
inline Rect spriteRect(Sprite sprite){
 static constexpr Rect frames[]={
 {354,55,238,238},{632,116,297,139},{955,125,284,122},{19,422,309,148},
 {329,344,285,269},{649,345,267,267},{939,405,303,174},
 {630,792,358,75},{1060,738,87,162},{101,1039,137,139},{400,1033,141,147},
 {702,1010,152,194},{996,981,188,224}};
 return frames[int(sprite)];
}
inline void sprite(Painter& p,Sprite art,Rect r){if(p.image)p.image(Atlas,r,spriteRect(art));}
inline void pill(Painter& p,Rect r){if(!p.image)return;auto s=spriteRect(Pill);double cap=std::min(r.w/2,r.h*.51);p.image(Atlas,{r.x,r.y,cap,r.h},{s.x,s.y,70,s.h});p.image(Atlas,{r.x+cap,r.y,r.w-2*cap,r.h},{s.x+70,s.y,s.w-140,s.h});p.image(Atlas,{r.x+r.w-cap,r.y,cap,r.h},{s.x+s.w-70,s.y,70,s.h});}
inline void rackKnob(Painter& p,Rect r,double v){
 sprite(p,KnobFace,r);
 double cx=r.x+r.w/2,cy=r.y+r.h/2,rad=r.w*.29;
 double a=(-225.+270*v)*3.141592653589793/180.;
 p.line(cx+rad*.28*std::cos(a),cy+rad*.28*std::sin(a),cx+rad*.9*std::cos(a),cy+rad*.9*std::sin(a),{20,20,22},std::max(2.,r.w*.035));
}
inline void renderControl(Painter& p,const Control& c,const Value& value,const Display& display,int lfo){
 const auto r=c.r;const double v=std::clamp(value(c.id),0.,1.);
 const Rgb white{224,221,221},muted{174,169,177},accent{54,238,204},ink{20,22,24};
 if(c.kind==Knob){
  double diameter=std::min(r.w-18,r.h-44);
  p.text(c.label,{r.x,r.y,r.w,21},r.w>130?18:13,white,true);
  rackKnob(p,{r.x+(r.w-diameter)/2,r.y+22,diameter,diameter},v);
  Rect read{r.x+10,r.y+r.h-22,r.w-20,22};pill(p,read);p.text(display(c.id,v),read,12,white,true);
 }else if(c.kind==Toggle){
  bool on=v>=.5;
  if(c.id>=kMixLock0&&c.id<kUiOutputL){sprite(p,on?Locked:Unlocked,{r.x,r.y,18,r.h});p.text(on?"LOCK":"FREE",{r.x+21,r.y,r.w-21,r.h},10,white,false);return;}
  pill(p,r);double d=std::min(17.,r.h*.6);Rect led{r.x+8,r.y+(r.h-d)/2,d,d};
  if(on)sprite(p,Led,led);else p.box({led.x+4,led.y+4,d-8,d-8},{91,74,80},{60,54,61},d/2);
  std::string title=c.label=="ON"?(on?"ON":"OFF"):c.label;if(c.id==kFreeze&&on)title="FROZEN";
  p.text(title,{r.x+22,r.y,r.w-27,r.h},std::min(15.,r.h*.43),white,true);
 }else if(c.kind==Select){
  pill(p,r);double shown=c.id==lfoID(lfo,lWave)&&value(kModWaveRnd0+lfo)>0?value(kUiModWave0+lfo):v;
  auto label=display(c.id,shown);if(c.id==kReverbRateV2&&v<.5/6){int old=value(kReverbSource)>=.5?kReverbRandomRate:kReverbGrid;label=display(old,value(old));}
  if(c.label.empty())p.text(label,{r.x+4,r.y,r.w-8,r.h},std::min(13.,r.h*.48),white,true);
  else{double split=c.label=="DEST"?34:std::min(r.w*.42,71.);p.line(r.x+split,r.y+5,r.x+split,r.y+r.h-5,{91,83,94},.7);
   p.text(c.label,{r.x+5,r.y,split-8,r.h},std::min(10.,r.h*.4),muted,true);
   p.text(label,{r.x+split+3,r.y,r.w-split-8,r.h},std::min(12.,r.h*.45),white,true);}
 }else if(c.kind==VSlider){
  double x=r.x+r.w/2,top=r.y+5,bottom=r.y+r.h-42,y=bottom-v*(bottom-top);
  p.box({x-3,top,6,bottom-top+8},ink,{13,12,16},3);
  for(int i=0;i<17;++i){double yy=top+i*(bottom-top)/16;p.line(x-22,yy,x-8,yy,muted,.7);p.line(x+8,yy,x+22,yy,muted,.7);}
  // A horizontal fader cap shares the metal finish of the thumb atlas.
  p.box({x-17,y-7,34,14},{104,104,107},{37,34,40},2);p.line(x-15,y-2,x+15,y-2,{208,209,211},2);
  p.text(c.label,{r.x,r.y+r.h-29,r.w,16},11,white,true);p.text(display(c.id,v),{r.x,r.y+r.h-13,r.w,15},10,white,true);
 }else if(c.kind==Slider||c.kind==Pan){
  if(c.id>=kLfoSlots0&&c.id<kLfoSpeed0){double amount=std::abs(v-.5)*2,mw=r.w-33;for(int i=0;i<20;++i){auto col=i<amount*20?accent:Rgb{37,24,45};p.box({r.x+i*mw/20,r.y+3,mw/20-2,r.h-6},col,col,0);}p.text(std::to_string(int(std::round(amount*100)))+"%",{r.x+mw+3,r.y,30,r.h},10,white,true);return;}
  bool compact=r.h<40;double y=r.y+(c.label.empty()?r.h*.5:(compact?r.h-7:29));
  if(!c.label.empty())p.text(c.label,{r.x,r.y,r.w,17},12,white,false);
  double track=r.w-(compact?32:0);sprite(p,Track,{r.x,y-5,track,10});sprite(p,Thumb,{r.x+3+v*(track-12),y-8,9,16});
  if(compact)p.text(display(c.id,v),{r.x+track+3,y-9,29,18},10,white,false);
  else p.text(display(c.id,v),{r.x,r.y+r.h-19,r.w,18},12,white,false);
 }else if(c.kind==Pad){bool on=v>=.5;p.box(r,on?accent:ink,on?Rgb{110,255,224}:Rgb{10,12,14},r.w/2);if(on)p.line(r.x+3,r.y+5,r.x+3,r.y+r.h-5,{110,255,223},1);
  if(c.id>=kReverbStep0&&c.id<kReverbStep0+16&&int(std::round(value(kUiReverb)*15))==int(c.id-kReverbStep0))p.line(r.x+3,r.y+r.h-5,r.x+r.w-3,r.y+r.h-5,white,1.5);
 }
}
inline void render(Painter& raw,const Value& value,const Display& display,const std::string& presetName,int tab,int lfo,int repeat,int gate,int reslice,WaveVisual* visual=nullptr,int drag=-1,int insertion=-1,Motion* motion=nullptr){
 Painter& p=raw;
 if(p.image)p.image(Backplate,{0,0,width,height},{0,0,1083,1452});
 auto panel=skin::C(skin::kPanel),edge=skin::C(skin::kFiligree),border=skin::C(skin::kBorderDark);
 Rgb white{224,221,221},muted{174,169,177},ink{20,22,24},screen{15,34,34},accent{54,238,204};
 auto button=[&](Rect r,const std::string& s){pill(p,r);p.text(s,r,14,white,true);};
 button(preset,presetName);button(previous,"<");button(next,">");button(load,"LOAD");button(save,"SAVE");
 p.text("INPUT DE-CLICKER",{835,40,172,20},13,white,true);
 const char* tabs[]={"GRANULIZER","PRESLICER","BEAT REPEATER","RESLICE","GATER"};
 if(motion)motion->update(value,drag,insertion);
 auto chain=routeChain(value);
 int moving=drag>=0?chain[drag]:-1;
 if(drag>=0&&insertion>=0&&insertion!=drag&&insertion!=drag+1)
  chain=fiveModuleOrder(int(std::round(moveRoute(chain,drag,insertion)*120.))-1);
 for(int i=0;i<5;++i){int stage=chain[i];Rect r=tabRect(i);if(motion)r.x=motion->x[stage];bool selected=stage==tab;
  sprite(p,selected?ActiveTab:Tab,r);
  p.text(tabs[stage],{r.x+14,r.y,r.w-36,r.h},10,white,true);
  p.text("✥",{r.x+3,r.y,16,r.h},14,muted,true);
  auto led=ledRect(i);led.x+=r.x-tabRect(i).x;bool on=value(stageEnabled[stage])>=.5;
  if(on)sprite(p,Led,{led.x,led.y+(led.h-17)/2,17,17});else p.box({led.x+5,led.y+(led.h-6)/2,6,6},ink,muted,3);
 }
 if(drag>=0&&insertion>=0){double x=insertion==5?tabRect(4).x+tabRect(4).w+3:tabRect(insertion).x-3;p.line(x,157,x,192,accent,2);}

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
  p.text("SLICE LENGTH / TRIGGER INTERVAL / PROBABILITY",{76,381,746,27},14,white,true);
  p.text(value(kUiGlitch)>.5?"PROCESSING SLICE":"WAITING FOR TRIGGER",{76,415,746,25},14,accent,true);
 }else{
  for(int i=0;i<16;++i){Rect r=stepRect(i,tab);bool on=false,selected=false;std::string s=std::to_string(i+1);
   if(tab==2){on=value(kRepeatStep0+i)>.5;selected=i==repeat;}
   if(tab==3){on=value(kResliceStep0+i)>.5;selected=i==reslice;s+=" > "+std::to_string(1+int(std::round(value(value(kResliceRndOn)>.5?kUiResliceSource0+i:kResliceIndex0+i)*15)));}
   if(tab==4){on=value(value(kGaterStepRnd)>.5?kUiGaterState0+i:kGaterState0+i)>.25;selected=i==gate;}
   if(tab==4){
    p.box(r,ink,selected?white:border,12);
    double length=std::clamp(value(kUiGaterLength0+i),.05,1.);if(value(kGaterLengthRnd)<.5)length=.05+.95*value(kGaterLength0+i);
    double h=(r.h-8)*length;
    if(on)p.box({r.x+4,r.y+r.h-4-h,r.w-8,h},skin::mix(ink,accent,.65),skin::mix(ink,accent,.65),8);
    double progress=motion?motion->gate[i]:(i==int(std::round(value(kUiGaterStep)*15))?std::min(value(kUiGaterPhase),length):0.);
    if(on&&progress>.001){double rise=(r.h-8)*progress;p.box({r.x+4,r.y+r.h-4-rise,r.w-8,rise},accent,accent,8);}
   }else p.box(r,on?accent:ink,selected?white:(on?skin::C(skin::kAccentBright):border),r.w/2);
   p.text(s,{r.x-9,r.y+r.h+4,r.w+18,21},tab==3?9:13,white,true);
   int play=tab==2?int(std::round(value(kUiStep)*15)):tab==3?int(std::round(value(kUiResliceStep)*15)):int(std::round(value(kUiGaterStep)*15));
   if(i==play)p.line(r.x+6,r.y+r.h-8,r.x+r.w-6,r.y+r.h-8,white,2);
   if(tab==4&&value(kGaterRelease0+i)>.5)p.box({r.x+9,r.y+10,14,14},skin::C(skin::kRelease),white,7);

  }
  if(tab==4)p.text("CLICK: WET / OFF     SHIFT-CLICK: LATCH RELEASE",{77,451,540,32},11,white,true);
 }

 for(int i=0;i<4;++i){Rect r=lfoRect(i);pill(p,r);p.text(std::to_string(i+1),r,17,i==lfo?white:muted,true);}
 sprite(p,Scope,scope);
 qg::Lfo preview;double cycle=std::round(value(kUiLfoCycle0+lfo)*4294967295.-2147483648.);
 int64_t epoch=int64_t(std::round(value(kUiLfoEpoch0+lfo)*4294967295.-2147483648.));
 preview.prepare(0x13579BDFULL+uint64_t(lfo)*104729+(value(lfoID(lfo,lReset))>=.5?uint64_t(epoch)*0x9e3779b97f4a7c15ULL:0));
 qg::LfoSettings shape;shape.enabled=true;shape.beats=1.;shape.wave=int(std::round(value(value(kModWaveRnd0+lfo)>0?kUiModWave0+lfo:lfoID(lfo,lWave))*129.));
 shape.randomSteps=1+int(std::round(value(kRandomSteps0+lfo)*63.));shape.depth=value(lfoID(lfo,lDepth));shape.phase=0;shape.glide=.01+.99*value(lfoID(lfo,lGlide));
 double lastX=0,lastY=0;for(int i=0;i<=210;++i){double x=scope.x+15+i*(scope.w-30)/210,y=scope.y+scope.h/2-preview.process(shape,cycle+i/210.,48000.,false,0)*(scope.h/2-17);if(i)p.line(lastX,lastY,x,y,accent,2);lastX=x;lastY=y;}
 double cursor=scope.x+15+std::clamp(value(kUiLfoPhase0+lfo),0.,1.)*(scope.w-30);
 p.line(cursor,scope.y+14,cursor,scope.y+scope.h-14,skin::mix(screen,white,.65),1.5);
 sprite(p,XY,xy);

 double px=xy.x+12+value(kXYX)*(xy.w-24),py=xy.y+12+(1-value(kXYY))*(xy.h-24);
 sprite(p,Led,{px-10,py-10,20,20});
 for(int i=0;i<32;++i){double x=522+i*15.3;Rect r{x,1026,10,69};bool active=value(kFilterSeqOn)>.5&&i==int(std::round(value(kUiFilterSeqStep)*31.));
  p.box(r,ink,border,5);double n=(qg::filterPattern(int(std::round(value(kFilterSeqPattern)*63)),i)+1)*.5;
  if(value(kFilterSeqOn)>.5)p.box({x+1,1092-n*63,8,3+n*63},active?white:accent,accent,4);
  if(i%4==0)p.text(std::to_string(i+1),{x-5,1098,23,16},10,muted,true);}
 if(value(kReverbSource)>=.5)p.text("RANDOM IMPULSE",{255,1296,382,32},16,muted,true);
 else for(int i=0;i<16;++i)p.text(std::to_string(i+1),{250.+i*24,1381,25,20},11,white,true);
 for(int channel=0;channel<2;++channel){double x=683+channel*166,y=1198;
  sprite(p,Meter,{x,y,160,92});
  double level=motion?motion->meters[channel]:value(channel?kUiOutputR:kUiOutputL);double db=20*std::log10(std::max(.00001,level));double angle=(-144.+108*std::clamp((db+36)/39.,0.,1.))*3.141592653589793/180.;
  p.line(x+80,y+79,x+80+57*std::cos(angle),y+79+57*std::sin(angle),{153,38,29},1.3);
 }
 for(const auto& c:controls(value,tab,lfo,repeat,gate,reslice))renderControl(p,c,value,display,lfo);
 pill(p,skinMenu);p.text("SKIN",skinMenu,11,white,true);
 pill(p,zoomMenu);p.text("SIZE",zoomMenu,11,white,true);
}
}} // namespace aztec::mockup
