#pragma once
// The approved three-page layout shares drawing and input geometry on both platforms.
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
constexpr double contentWidth=1083,rackInset=0,width=1083,height=1050,bodyTop=202;
struct Rect {double x,y,w,h;bool contains(double px,double py)const{return px>=x&&py>=y&&px<x+w&&py<y+h;}};
struct Viewport {double scale,x,y;Viewport(double w,double h):scale(std::max(.01,std::min(w/width,h/height))),x((w-width*scale)/2),y((h-height*scale)/2){} Rect screen(Rect r)const{return {x+r.x*scale,y+r.y*scale,r.w*scale,r.h*scale};} std::pair<double,double> logical(double px,double py)const{return {(px-x)/scale,(py-y)/scale};}};
enum class RenderPass {All,Static,Dynamic};
using Value=std::function<double(ParamID)>;using Display=std::function<std::string(ParamID,double)>;
struct RackState {int page=0;std::array<double,3> scroll{};double offset()const{return scroll[page];}Rect position(Rect r)const{r.y-=offset();return r;}};
constexpr Rect preset{419,41,172,39},previous{600,42,32,38},next{638,42,32,38},load{677,42,63,38},save{747,42,68,38};
constexpr Rect skinMenu{836,bodyTop+1300,80,30},zoomMenu{926,bodyTop+1300,80,30};
constexpr Rect xy{211,270,660,660},scope{78,290,285,258};
// One geometry contract for drawing, clicks, drags and host automation.
inline Rect xySurface(Rect r=xy){return {r.x+18,r.y+18,r.w-36,r.h-36};}
inline Rect xyTravel(Rect r=xy){auto a=xySurface(r);return {a.x+12,a.y+12,a.w-24,a.h-24};}
inline double xyNormalized(double v){return std::isfinite(v)?std::clamp(v,0.,1.):.5;}
inline std::pair<double,double> xyPoint(Rect r,double x,double y){auto a=xyTravel(r);return {a.x+xyNormalized(x)*a.w,a.y+(1.-xyNormalized(y))*a.h};}
inline std::pair<double,double> xyValue(Rect r,double x,double y){auto a=xyTravel(r);return {xyNormalized((x-a.x)/a.w),xyNormalized(1.-(y-a.y)/a.h)};}
constexpr ParamID stageEnabled[]={kGrainEnabled,kGlitchEnabled,kRepeatEnabled,kResliceEnabled,kGaterEnabled};
constexpr const char* moduleNames[]={"GRANULIZER","PRESLICER","BEAT REPEATER","RESLICE","GATER"};
inline Rect pageRect(int i){return {60.+i*323,133,315,52};}
inline int hitPage(double x,double y){for(int i=0;i<3;++i)if(pageRect(i).contains(x,y))return i;return -1;}
inline std::array<int,4> routeChain(const Value& value){int n=int(std::round(value(kRoutingOrder)*120))-1;if(n>=0)return fourModuleOrder(n);std::array<int,4> c{{0,2,3,4}};int old=int(std::round(value(kModuleOrder)*6));if(old<6){int i=0;for(int stage:moduleOrders[std::clamp(old,0,5)])if(stage!=1)c[i++]=stage;}return c;}
inline double moveRoute(std::array<int,4> c,int from,int insertion){from=std::clamp(from,0,3);insertion=std::clamp(insertion,0,4);int to=insertion-(insertion>from?1:0),stage=c[from];if(to>from)for(int i=from;i<to;++i)c[i]=c[i+1];else for(int i=from;i>to;--i)c[i]=c[i-1];c[to]=stage;return fourModuleRouting(c);}
inline double moduleHeight(int stage){constexpr double h[]={340,270,350,280,330};return h[stage];}
inline Rect moduleRect(int stage,const Value& value,const RackState& state){double y=430;for(int s:routeChain(value)){if(s==stage)return state.position({54,y,974,moduleHeight(s)});y+=moduleHeight(s)+14;}return {};}
inline double contentBottom(int page){return page==0?430+340+350+280+330+56:page==1?1544:1042;}
inline double maxScroll(const RackState& s){return std::max(0.,contentBottom(s.page)-height+18);}
inline void scrollBy(RackState& s,double delta){s.scroll[s.page]=std::clamp(s.offset()+delta,0.,maxScroll(s));}
inline Rect scrollbar(){return {1037,bodyTop+8,12,height-bodyTop-20};}
inline Rect scrollThumb(const RackState& s){auto r=scrollbar();double h=std::max(50.,r.h*(height-bodyTop)/(contentBottom(s.page)-bodyTop));return {r.x,r.y+(r.h-h)*(maxScroll(s)>0?s.offset()/maxScroll(s):0),r.w,h};}
inline Rect headerRect(int stage,const Value& value,const RackState& s){auto r=moduleRect(stage,value,s);return {r.x+17,r.y+10,655,41};}
inline int hitRoute(double x,double y,const Value& value,const RackState& s){if(s.page!=0||y<bodyTop||y>=height)return -1;auto chain=routeChain(value);for(int i=0;i<4;++i)if(headerRect(chain[i],value,s).contains(x,y))return i;return -1;}
inline int routeInsertion(double x,double y,const Value& value,const RackState& s){if(s.page!=0||x<54||x>1028||y<bodyTop||y>=height)return -1;auto chain=routeChain(value);for(int i=0;i<4;++i){auto r=moduleRect(chain[i],value,s);if(y<r.y+r.h/2)return i;}return 4;}
inline Rect randomRect(int stage,const Value& v,const RackState& s){auto r=moduleRect(stage,v,s);return {r.x+805,r.y+(stage==3?235:13),stage==3?143.:119.,30};}
inline Rect enableRect(int stage,const Value& v,const RackState& s){auto r=moduleRect(stage,v,s);return {r.x+712,r.y+13,82,30};}
inline Rect stepRect(int i,int stage,const Value& v,const RackState& s){auto r=moduleRect(stage,v,s);double y=stage==3?130:stage==4?208:226;return {r.x+30+i*57.2,r.y+y,32,stage==3?57.:stage==4?55.:46.};}
inline int hitStep(double x,double y,const Value& v,const RackState& s,int& stage){if(s.page!=0||y<bodyTop)return -1;for(int st:{2,3,4})for(int i=0;i<16;++i)if(stepRect(i,st,v,s).contains(x,y)){stage=st;return i;}return -1;}
inline Rect waveRect(const Value& v,const RackState& s){auto r=moduleRect(0,v,s);return {r.x+24,r.y+229,r.w-48,85};}
inline Rect lfoRect(int i){return {688.+i*77,223,64,34};}
inline int hitLfo(double x,double y,const RackState& s){if(s.page!=1||y<bodyTop)return -1;for(int i=0;i<4;++i)if(s.position(lfoRect(i)).contains(x,y))return i;return -1;}
inline bool visible(Rect r){return r.y+r.h>bodyTop&&r.y<height;}
using skin::Rgb;
struct Painter {std::function<void(int,Rect,Rect)> image;std::function<void(Rect,Rgb,Rgb,double)> box;std::function<void(const std::string&,Rect,double,Rgb,bool)> text;std::function<void(double,double,double,double,Rgb,double)> line;std::function<void(Rect,double)> knob;std::function<void(const std::vector<std::pair<double,double>>&,Rgb)> polygon;std::function<void(const std::vector<std::pair<double,double>>&,Rgb,double)> polyline;std::function<void(Rect)> clip;std::function<void()> unclip;};
enum Kind {Knob,Slider,Toggle,Select,Pad,Pan,PanMode,VSlider};
struct Control {ParamID id;Rect r;Kind kind;std::string label;};
inline std::vector<Control> controls(const Value& value,int currentTab,int selectedLfo,int selectedRepeat,int selectedGate,int selectedReslice,const RackState& state=RackState{}){
 std::vector<Control> result;(void)currentTab;
 #define ADD(ID,X,Y,W,H,K,L) result.push_back({ParamID(ID),{double(X),double(Y),double(W),double(H)},K,L})
 #include "editor_layout_win.inl"
 #undef ADD
 return result;
}
inline Rect valueRect(Rect r,Kind kind,ParamID id,const std::string& label){
 if(kind==Knob)return {r.x+10,r.y+r.h-22,r.w-20,22};
 if(kind==VSlider)return {r.x,r.y+r.h-16,r.w,16};
 if(kind==Slider||kind==Pan){if(id>=kLfoSlots0&&id<kLfoSpeed0)return {r.x+r.w-33,r.y,33,r.h};if(r.h>=40)return {r.x,r.y+r.h-19,r.w,19};return {r.x+r.w-32,r.y,32,r.h};}
 return {};
}
struct Motion {
 std::array<double,5> y{};std::array<double,16> gate{};std::array<double,2> meters{};bool ready=false,routing=false;std::chrono::steady_clock::time_point last{};
 void update(const Value& value,int drag,int insertion){auto now=std::chrono::steady_clock::now();double dt=ready?std::clamp(std::chrono::duration<double>(now-last).count(),0.,.1):0.;last=now;auto chain=routeChain(value);if(drag>=0&&insertion>=0)chain=fourModuleOrder(int(std::round(moveRoute(chain,drag,insertion)*120))-1);double ease=1-std::exp(-dt/.065),top=430;routing=false;for(int stage:chain){y[stage]=ready?y[stage]+ease*(top-y[stage]):top;if(std::abs(top-y[stage])>.8)routing=true;top+=moduleHeight(stage)+14;}int playing=int(std::round(value(kUiGaterStep)*15));for(int i=0;i<16;++i){double target=i==playing&&value(kGaterEnabled)>.5?std::min(value(kUiGaterPhase),value(kUiGaterLength0+i)):0.;gate[i]+=(1-std::exp(-dt/.025))*(target-gate[i]);}for(int ch=0;ch<2;++ch){double target=value(kUiOutputL+ch);meters[ch]+=(1-std::exp(-dt/(target>meters[ch]?.075:.3)))*(target-meters[ch]);}ready=true;}
};
struct WaveVisual {std::array<double,waveformBins> lo{},hi{};double active=0;void update(const Value& v){for(int i=0;i<waveformBins;++i){lo[i]+=.06*((v(kUiWaveLow0+i)*2-1)-lo[i]);hi[i]+=.06*((v(kUiWaveHigh0+i)*2-1)-hi[i]);}active+=.06*((v(kUiGrainActive)>.5?1.:0.)-active);}};
enum Art {Backplate, Atlas};
enum Sprite {KnobFace, Pill, Tab, ActiveTab, Scope, XY, Meter, Track, Thumb, Led, Amber, Locked, Unlocked};
inline Rect spriteRect(Sprite sprite){
 static constexpr Rect frames[]={
 {354,55,238,238},{632,116,297,139},{955,125,284,122},{33,438,283,120},
 {329,344,285,269},{649,345,267,267},{939,405,303,174},
 {630,792,358,75},{1060,738,87,162},{101,1039,137,139},{400,1033,141,147},
 {702,1010,152,194},{996,981,188,224}};
 return frames[int(sprite)];
}
inline void sprite(Painter& p,Sprite art,Rect r){if(p.image)p.image(Atlas,r,spriteRect(art));}
inline void pill(Painter& p,Rect r){
 p.box({r.x,r.y+1,r.w,r.h},{12,14,18},{10,11,14},5);
 p.box({r.x+1,r.y+1,r.w-2,r.h-3},{35,38,44},{76,79,87},4);
 p.line(r.x+6,r.y+2,r.x+r.w-6,r.y+2,{98,100,108},.7);
}
inline void pageButton(Painter& p,int i,bool selected){
 auto r=pageRect(i);p.box(r,{12,14,18},{12,14,18},6);
 p.box({r.x+1,r.y+1,r.w-2,r.h-3},selected?Rgb{55,54,64}:Rgb{34,36,42},{86,85,94},5);
 p.line(r.x+7,r.y+2,r.x+r.w-7,r.y+2,selected?Rgb{125,119,135}:Rgb{88,89,99},.8);
 p.text("0"+std::to_string(i+1),{r.x+17,r.y+3,28,r.h-6},11,{155,153,166},true);
 p.text(i==0?"MODULES":i==1?"MODULATION":"MORPH",{r.x+48,r.y+1,r.w-89,r.h-3},18,selected?Rgb{232,237,235}:Rgb{174,178,188},true);
 if(selected)p.box({r.x+20,r.y+r.h-5,r.w-40,2},{100,204,182},{100,204,182},1);
}
inline void xyPanel(Painter& p,Rect r,bool axes=true){
 p.box(r,{36,39,46},{99,102,111},12);
 p.box({r.x+4,r.y+4,r.w-8,r.h-8},{8,11,15},{7,9,12},9);
 auto a=xySurface(r);p.box(a,{17,29,32},{49,66,71},4);
 for(int i=1;i<8;++i){double t=i/8.;auto c=i==4?Rgb{65,89,91}:Rgb{28,44,49};p.line(a.x+a.w*t,a.y+1,a.x+a.w*t,a.y+a.h-1,c,i==4?1.:.6);p.line(a.x+1,a.y+a.h*t,a.x+a.w-1,a.y+a.h*t,c,i==4?1.:.6);}
 if(axes)p.text("X",{a.x+a.w-21,a.y+a.h/2+7,14,15},10,{112,136,140},true);
 if(axes)p.text("Y",{a.x+a.w/2+7,a.y+7,14,15},10,{112,136,140},true);
}
inline void rackKnob(Painter& p,Rect r,double v){
 const double cx=r.x+r.w/2,cy=r.y+r.h/2,rad=r.w*.32;
 p.box(r,{16,19,23},{86,89,96},r.w/2);
 p.box({r.x+3,r.y+3,r.w-6,r.h-6},{39,42,48},{17,19,24},r.w/2);
 // Reuse just the machined face; no enlarged gear teeth or glowing rim.
 if(p.image)p.image(Atlas,{r.x+r.w*.16,r.y+r.h*.16,r.w*.68,r.h*.68},{405,106,136,136});
 for(int i=0;i<11;++i){double a=(-225.+27*i)*3.141592653589793/180.;p.line(cx+r.w*.435*std::cos(a),cy+r.h*.435*std::sin(a),cx+r.w*.46*std::cos(a),cy+r.h*.46*std::sin(a),{137,143,150},.7);}
 double a=(-225.+270*v)*3.141592653589793/180.;
 p.line(cx+rad*.48*std::cos(a),cy+rad*.48*std::sin(a),cx+rad*.90*std::cos(a),cy+rad*.90*std::sin(a),{20,22,25},std::max(2.,r.w*.035));
}
inline void renderControl(Painter& p,const Control& c,const Value& value,const Display& display,int lfo){
 const auto r=c.r;const double v=std::clamp(value(c.id),0.,1.);
 const Rgb white{230,233,237},muted{170,173,184},accent{100,204,182},ink{20,22,24};
 if(c.kind==Knob){
  double diameter=std::min(r.w-22,r.h-60);
  p.text(c.label,{r.x,r.y,r.w,21},14,white,true);
  rackKnob(p,{r.x+(r.w-diameter)/2,r.y+29,diameter,diameter},v);
  Rect read{r.x+10,r.y+r.h-22,r.w-20,22};pill(p,read);p.text(display(c.id,v),read,12,white,true);
 }else if(c.kind==Toggle){
  bool on=v>=.5;
  if(c.id>=kMixLock0&&c.id<kUiOutputL){sprite(p,on?Locked:Unlocked,{r.x,r.y,18,r.h});p.text(on?"LOCK":"FREE",{r.x+21,r.y,r.w-21,r.h},10,white,false);return;}
  pill(p,r);double d=std::min(17.,r.h*.6);Rect led{r.x+8,r.y+(r.h-d)/2,d,d};
  p.box({led.x+3,led.y+3,d-6,d-6},{13,19,22},{70,78,83},d/2);p.box({led.x+5,led.y+5,d-10,d-10},on?accent:Rgb{70,75,83},on?accent:Rgb{70,75,83},d/2);
  std::string title=c.label=="ON"?(on?"ON":"OFF"):c.label;if(c.id==kFreeze&&on)title="FROZEN";
  p.text(title,{r.x+22,r.y,r.w-27,r.h},std::min(15.,r.h*.43),white,true);
 }else if(c.kind==Select){
  pill(p,r);double shown=c.id==lfoID(lfo,lWave)&&modWaveSelection(value,lfo)>0?value(kUiModWave0+lfo):v;
  auto label=display(c.id,shown);if(c.id>=kModWaveRate0&&c.id<kCount&&v<.5/7){const char* names[]={"Off","1/1","1/2","1/4","1/8","2/1","4/1"};label=names[modWaveSelection(value,int(c.id-kModWaveRate0))];}if(c.id==kReverbSource)label=v>=.5?"RANDOM IMP.":"STEP SEQ.";
  if(c.id==kFilterSeqMode)label=v>=.5?"S&G":"ARP";
  const auto legacy=label.find(" (legacy)");if(legacy!=std::string::npos)label.erase(legacy);
  if(c.id==kReverbRateV2&&v<.5/6){int old=value(kReverbSource)>=.5?kReverbRandomRate:kReverbGrid;label=display(old,value(old));}
  if(c.label.empty())p.text(label,{r.x+4,r.y,r.w-8,r.h},std::min(13.,r.h*.48),white,true);
  else{double split=c.label=="DEST"?34:std::min(r.w*.44,std::max(71.,c.label.size()*5.5+8.));p.line(r.x+split,r.y+5,r.x+split,r.y+r.h-5,{91,83,94},.7);
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
  double track=r.w-(compact?32:0);p.box({r.x,y-3,track,6},{10,13,18},{79,81,91},3);double tx=r.x+3+v*(track-12);p.box({tx,y-8,9,16},{142,149,155},{31,35,42},2);p.line(tx+4,y-5,tx+4,y+5,{236,239,241},1);
  if(compact)p.text(display(c.id,v),{r.x+track+3,y-9,29,18},10,white,false);
  else p.text(display(c.id,v),{r.x,r.y+r.h-19,r.w,18},12,white,false);
 }else if(c.kind==Pad){bool on=v>=.5;p.box(r,on?accent:ink,on?Rgb{110,255,224}:Rgb{10,12,14},r.w/2);if(on)p.line(r.x+3,r.y+5,r.x+3,r.y+r.h-5,{110,255,223},1);
  if(c.id>=kReverbStep0&&c.id<kReverbStep0+16&&int(std::round(value(kUiReverb)*15))==int(c.id-kReverbStep0))p.line(r.x+3,r.y+r.h-5,r.x+r.w-3,r.y+r.h-5,white,1.5);
 }
}

// Stretch only the blank purple panel interior; metal corners and screws retain their proportions.
inline void rackPanel(Painter& p,Rect r){if(!visible(r))return;const Rect src{52,122,798,389};constexpr double cap=27;auto img=[&](Rect d,Rect s){if(p.image)p.image(Backplate,d,s);};img({r.x+cap,r.y+cap,r.w-2*cap,r.h-2*cap},{src.x+cap,src.y+cap,src.w-2*cap,src.h-2*cap});for(int side=0;side<2;++side){double x=side?r.x+r.w-cap:r.x,sx=side?src.x+src.w-cap:src.x;img({x,r.y,cap,cap},{sx,src.y,cap,cap});img({x,r.y+r.h-cap,cap,cap},{sx,src.y+src.h-cap,cap,cap});img({x,r.y+cap,cap,r.h-2*cap},{sx,src.y+cap,cap,src.h-2*cap});}img({r.x+cap,r.y,r.w-2*cap,cap},{src.x+cap,src.y,src.w-2*cap,cap});img({r.x+cap,r.y+r.h-cap,r.w-2*cap,cap},{src.x+cap,src.y+src.h-cap,src.w-2*cap,cap});}
inline bool structural(ParamID id){return id==kPanMode||id==kReverbSource||id==kReverbModel||id==kFilterModel||id==kFilterSeqMode||id==kRoutingOrder||id==kModuleOrder;}
inline bool monitorVisible(ParamID id,const Value& v,const RackState& s,int lfo){
 if(s.page==2)return false;
 if(s.page==1){if(id>=kUiLfoPhase0&&id<kModWaveRnd0)return int(id-kUiLfoPhase0)%4==lfo&&visible(s.position(scope));if(id>=kUiModWave0&&id<kReverbSource)return int(id-kUiModWave0)==lfo&&visible(s.position(scope));if(id==kUiFilterSeqStep)return visible(s.position({573,836,428,104}));if(id==kUiReverb||id==kUiReverbGate)return visible(s.position({300,1118,700,138}));if(id==kUiOutputL||id==kUiOutputR)return visible(s.position({92,1376,354,96}));return false;}
 if((id>=kUiWave0&&id<=kUiWaveSeconds)||(id>=kUiWaveLow0&&id<kReverbRateV2))return visible(waveRect(v,s));if(id==kUiGlitch)return visible(moduleRect(1,v,s));if(id==kUiStep||id==kUiRepeat)return visible(moduleRect(2,v,s));if((id>=kUiGaterStep&&id<kInputDeclick)||id==kUiGaterPhase)return visible(moduleRect(4,v,s));if(id==kUiResliceStep||id==kUiResliceActive||(id>=kUiResliceSource0&&id<kUiFilterSeqStep))return visible(moduleRect(3,v,s));return false;
}
inline void restoreControlBackground(Painter& p,const Control& c,const Value& v,const RackState& s){
 if(c.id==kInputDeclick||c.id==kDeclickSensitivity){if(p.image)p.image(Backplate,c.r,c.r);return;}
 if(s.page==0){if(c.id==kTranspose||c.id==kStretchSpeed||c.id==kStretchOn){rackPanel(p,s.position({54,210,974,188}));return;}for(int stage=0;stage<5;++stage){auto r=moduleRect(stage,v,s);if(c.r.y>=r.y&&c.r.y<r.y+r.h){rackPanel(p,r);return;}}}
 if(s.page==1)for(auto r:{Rect{54,210,974,475},Rect{54,699,480,294},Rect{548,699,480,294},Rect{54,1007,974,305},Rect{54,1326,974,210}}){r=s.position(r);if(r.contains(c.r.x,c.r.y)){rackPanel(p,r);return;}}
 if(s.page==2)rackPanel(p,s.position({54,210,974,832}));
}
inline void render(Painter& p,const Value& value,const Display& display,const std::string& presetName,int tab,int lfo,int repeat,int gate,int reslice,WaveVisual* visual=nullptr,int drag=-1,int insertion=-1,Motion* motion=nullptr,RenderPass pass=RenderPass::All,const RackState& state=RackState{}){
 const bool fixed=pass!=RenderPass::Dynamic,live=pass!=RenderPass::Static;const Rgb white{230,233,237},muted{170,173,184},ink{20,22,24},screen{15,34,34},accent{100,204,182};
 if(live&&motion)motion->update(value,drag,insertion);
 auto button=[&](Rect r,const std::string& s){pill(p,r);p.text(s,r,14,white,true);};
 if(fixed){p.box({0,0,width,height},{25,18,33},{25,18,33},0);if(p.image){p.image(Backplate,{0,0,width,120},{0,0,1083,120});p.image(Backplate,{0,120,50,height-120},{0,120,50,1300});p.image(Backplate,{1033,120,50,height-120},{1033,120,50,1300});}p.box({51,122,981,75},{24,25,31},{11,12,16},5);for(int i=0;i<3;++i)pageButton(p,i,i==state.page);button(preset,presetName);button(previous,"<");button(next,">");button(load,"LOAD");button(save,"SAVE");p.text("INPUT DE-CLICKER",{835,40,172,20},13,white,true);}
 if(p.clip)p.clip({51,bodyTop,982,height-bodyTop});
 auto pos=[&](Rect r){return state.position(r);};auto title=[&](Rect r,const std::string& s){if(fixed)p.text(s,{r.x+32,r.y+17,r.w-230,25},20,white,false);};
 Value drawnValue=value;std::array<double,5> tops{};for(int i=0;i<5;++i)tops[i]=moduleRect(i,value,state).y;
 auto list=controls(value,tab,lfo,repeat,gate,reslice,state);
 if(state.page==0){
  Rect master=pos({54,210,974,188});if(fixed){rackPanel(p,master);title(master,"MASTER OPTIONS");p.text("SIGNAL CHAIN ↓  •  FIRST MODULE AT THE TOP",pos({78,404,520,20}),11,muted,false);p.text("DRAG A MODULE UP OR DOWN TO REORDER",pos({619,404,385,20}),11,muted,true);}
  auto chain=routeChain(value);for(int stage:chain){auto r=moduleRect(stage,value,state);double dy=(motion&&motion->ready&&(drag>=0||motion->routing))?motion->y[stage]-state.offset()-r.y:0;r.y+=dy;if(!visible(r))continue;
   if(fixed){rackPanel(p,r);p.box({r.x+36,r.y+13,32,30},{25,28,35},{91,96,106},5);p.text(std::to_string(1+int(std::find(chain.begin(),chain.end(),stage)-chain.begin())),{r.x+36,r.y+13,32,30},14,accent,true);p.text(moduleNames[stage],{r.x+86,r.y+16,480,26},20,white,false);for(int j=0;j<3;++j)for(int k=0;k<2;++k)p.box({r.x+635+k*6,r.y+20+j*6,2,2},muted,muted,1);auto rr=randomRect(stage,value,state);rr.y+=dy;button(rr,stage==3?"RANDOM ONCE":"RANDOM");}

   if(live&&stage==0){auto w=waveRect(value,state);w.y+=dy;p.box(w,screen,{76,75,82},10);WaveVisual direct;if(!visual){for(int i=0;i<waveformBins;++i){direct.lo[i]=value(kUiWaveLow0+i)*2-1;direct.hi[i]=value(kUiWaveHigh0+i)*2-1;}direct.active=value(kUiGrainActive);visual=&direct;}double scale=w.h/2-6,mid=w.y+w.h/2;auto env=[&](int first,int last){std::vector<std::pair<double,double>> pts;for(int i=first;i<=last;++i)pts.push_back({w.x+4+i*(w.w-8)/(waveformBins-1),mid-visual->hi[i]*scale});for(int i=last;i>=first;--i)pts.push_back({w.x+4+i*(w.w-8)/(waveformBins-1),mid-visual->lo[i]*scale});return pts;};auto base=skin::mix(screen,muted,.35);p.polygon(env(0,waveformBins-1),base);p.line(w.x+4,mid,w.x+w.w-4,mid,{63,80,76},1);if(visual->active>.01){double start=std::min(value(kUiGrainStart),value(kUiGrainEnd)),end=std::max(value(kUiGrainStart),value(kUiGrainEnd));int first=std::clamp(int(start*(waveformBins-1)),0,waveformBins-1),last=std::clamp(int(std::ceil(end*(waveformBins-1))),first,waveformBins-1);auto lit=skin::mix(base,accent,.45*visual->active);p.polygon(env(first,last),lit);double x=w.x+4+value(kUiGrainHead)*(w.w-8);p.line(x,w.y+6,x,w.y+w.h-6,lit,1.5);}}
   if(live&&stage==1)p.text(value(kUiGlitch)>.5?"PROCESSING SLICE":"WAITING FOR TRIGGER",{r.x+29,r.y+r.h-37,r.w-58,22},12,accent,true);
   if(live&&stage>=2)for(int i=0;i<16;++i){Rect sr=stepRect(i,stage,value,state);sr.y+=dy;bool on=stage==2?value(kRepeatStep0+i)>.5:stage==3?value(kResliceStep0+i)>.5:value(value(kGaterStepRnd)>.5?kUiGaterState0+i:kGaterState0+i)>.25;bool selected=i==(stage==2?repeat:stage==3?reslice:gate);if(stage==4){p.box(sr,ink,selected?white:muted,12);double len=value(kGaterLengthRnd)>.5?std::clamp(value(kUiGaterLength0+i),.05,1.):.05+.95*value(kGaterLength0+i),h=(sr.h-8)*len;if(on)p.box({sr.x+4,sr.y+sr.h-4-h,sr.w-8,h},skin::mix(ink,accent,.65),ink,8);double progress=motion?motion->gate[i]:0;if(on&&progress>.001)p.box({sr.x+4,sr.y+sr.h-4-(sr.h-8)*progress,sr.w-8,(sr.h-8)*progress},accent,accent,8);if(value(kGaterRelease0+i)>.5)p.box({sr.x+10,sr.y+9,12,12},{180,104,171},white,6);}else p.box(sr,on?accent:ink,selected?white:muted,10);p.text(std::to_string(i+1),{sr.x-5,sr.y+sr.h+5,sr.w+10,18},12,white,true);if(stage==3)p.text(std::to_string(1+int(std::round(value(value(kResliceRndOn)>.5?kUiResliceSource0+i:kResliceIndex0+i)*15))),{sr.x-5,sr.y+sr.h+24,sr.w+10,16},10,muted,true);int play=int(std::round(value(stage==2?kUiStep:stage==3?kUiResliceStep:kUiGaterStep)*15));if(i==play)p.line(sr.x+5,sr.y+sr.h-6,sr.x+sr.w-5,sr.y+sr.h-6,white,2);}
  }
  if(drag>=0&&insertion>=0){auto c=routeChain(value);double y=insertion==4?moduleRect(c[3],value,state).y+moduleHeight(c[3])+5:moduleRect(c[insertion],value,state).y-6;p.line(68,y,1014,y,accent,3);}
 }else if(state.page==1){
  for(auto r:{Rect{54,210,974,475},Rect{54,699,480,294},Rect{548,699,480,294},Rect{54,1007,974,305},Rect{54,1326,974,210}})if(fixed)rackPanel(p,pos(r));title(pos({54,210,974,475}),"MODULATION");title(pos({54,699,480,294}),"FILTER");title(pos({548,699,480,294}),"FILTER SEQUENCER");title(pos({54,1007,974,305}),"REVERB");title(pos({54,1326,974,210}),"OUTPUT");
  if(fixed){for(int i=0;i<4;++i){auto r=pos(lfoRect(i));pill(p,r);p.text(std::to_string(i+1),r,17,i==lfo?accent:white,true);}xyPanel(p,pos(scope),false);}
  if(live&&visible(pos(scope))){auto sc=pos(scope);qg::Lfo preview;double cycle=std::round(value(kUiLfoCycle0+lfo)*4294967295.-2147483648.);int64_t epoch=int64_t(std::round(value(kUiLfoEpoch0+lfo)*4294967295.-2147483648.));preview.prepare(0x13579BDFULL+uint64_t(lfo)*104729+(value(lfoID(lfo,lReset))>=.5?uint64_t(epoch)*0x9e3779b97f4a7c15ULL:0));qg::LfoSettings shape;shape.enabled=true;shape.beats=1;shape.wave=int(std::round(value(modWaveSelection(value,lfo)>0?kUiModWave0+lfo:lfoID(lfo,lWave))*129));shape.randomSteps=1+int(std::round(value(kRandomSteps0+lfo)*63));shape.depth=value(lfoID(lfo,lDepth));shape.phase=0;shape.glide=.01+.99*value(lfoID(lfo,lGlide));auto glass=xySurface(sc);std::vector<std::pair<double,double>> pts;for(int i=0;i<=210;++i)pts.push_back({glass.x+3+i*(glass.w-6)/210,glass.y+glass.h/2-std::clamp(preview.process(shape,cycle+i/210.,48000,false,0),-1.,1.)*(glass.h/2-5)});if(p.polyline)p.polyline(pts,accent,2);else for(size_t i=1;i<pts.size();++i)p.line(pts[i-1].first,pts[i-1].second,pts[i].first,pts[i].second,accent,2);double x=glass.x+3+xyNormalized(value(kUiLfoPhase0+lfo))*(glass.w-6);p.line(x,glass.y+3,x,glass.y+glass.h-3,white,1);}
  if(live&&visible(pos({573,836,428,104})))for(int i=0;i<32;++i){auto r=pos({573+i*13.4,836,9,104});p.box(r,ink,ink,4);double n=(qg::filterPattern(int(std::round(value(kFilterSeqPattern)*63)),i)+1)*.5;bool active=value(kFilterSeqOn)>.5&&i==int(std::round(value(kUiFilterSeqStep)*31));if(value(kFilterSeqOn)>.5)p.box({r.x+1,r.y+101-n*97,7,3+n*97},active?white:accent,accent,3);if(i%4==0)p.text(std::to_string(i+1),{r.x-4,r.y+108,18,15},9,muted,true);}
  if(fixed&&value(kReverbSource)>=.5)p.text("RANDOM IMPULSE",pos({305,1140,640,75}),18,muted,true);
  if(live&&value(kReverbSource)<.5)for(int i=0;i<16;++i){auto r=pos({300.+i*43,1118,26,138});if(visible(r))renderControl(p,{ParamID(kReverbStep0+i),r,Pad,""},value,display,lfo);p.text(std::to_string(i+1),{r.x-3,r.y+r.h+5,r.w+6,18},12,white,true);}
  for(int ch=0;ch<2;++ch){auto r=pos({92.+ch*185,1376,169,96});if(fixed&&visible(r))sprite(p,Meter,r);if(live&&visible(r)){double level=motion?motion->meters[ch]:value(kUiOutputL+ch),db=20*std::log10(std::max(.00001,level)),a=(-144.+108*std::clamp((db+36)/39.,0.,1.))*3.141592653589793/180;p.line(r.x+84,r.y+83,r.x+84+60*std::cos(a),r.y+83+60*std::sin(a),{153,38,29},1.3);}}
  if(fixed){button(pos(skinMenu),"SKIN");button(pos(zoomMenu),"SIZE");}
 }else{auto r=pos({54,210,974,832});if(fixed){rackPanel(p,r);title(r,"MORPH XY");xyPanel(p,pos(xy));}if(live){auto r=pos(xy);auto pt=xyPoint(r,value(kXYX),value(kXYY));if(p.clip)p.clip(xySurface(r));p.box({pt.first-9,pt.second-9,18,18},{24,46,48},{158,225,208},9);p.box({pt.first-5,pt.second-5,10,10},accent,accent,5);if(p.unclip)p.unclip();}}
 if(state.page==0&&motion&&motion->ready&&(drag>=0||motion->routing))for(auto& c:list)if(c.id!=kInputDeclick&&c.id!=kDeclickSensitivity){for(int stage=0;stage<5;++stage)if(c.r.y>=tops[stage]&&c.r.y<tops[stage]+moduleHeight(stage)){c.r.y+=motion->y[stage]-state.offset()-tops[stage];break;}}
 if(fixed)for(const auto& c:list)if(c.r.y!=65&&c.id!=kDeclickSensitivity&&c.kind!=Pad&&visible(c.r))renderControl(p,c,value,display,lfo);
 if(p.unclip)p.unclip();
 if(fixed)for(const auto& c:list)if(c.id==kInputDeclick||c.id==kDeclickSensitivity)renderControl(p,c,value,display,lfo);
 if(fixed&&maxScroll(state)>0){auto bar=scrollbar();p.box(bar,{31,24,39},{31,24,39},6);p.box(scrollThumb(state),{124,110,139},{148,133,162},6);}
}
}} // aztec::mockup
