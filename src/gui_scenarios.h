#include "preset_banks.h"
#pragma once
// Real native input scenarios shared by Cocoa and Win32 smoke runners.
#include "mockup_ui.h"
#include "factory_presets.h"
#include <stdexcept>
#include <cmath>
namespace aztec {
template<class Click,class Drag,class Scroll,class Page>
void runGuiScenarios(Controller* c,Click click,Drag drag,Scroll scroll,Page actualPage){
 auto check=[](bool ok,const char* msg){if(!ok)throw std::runtime_error(msg);};
 mockup::RackState rack;auto val=[&](ParamID id){return c->getParamNormalized(id);};int lfo=0;
 auto rectClick=[&](mockup::Rect r,int count=1){click(r.x+r.w/2,r.y+r.h/2,count);};
 auto page=[&](int p){rectClick(mockup::pageRect(p));rack.page=p;check(actualPage()==p,"Fixed page navigation");};
 auto setScroll=[&](double d){rack.scroll[rack.page]=std::clamp(d,0.,mockup::maxScroll(rack));scroll(rack.offset());};
 auto focus=[&](int stage){page(0);setScroll(0);auto r=mockup::moduleRect(stage,val,rack);setScroll(r.y-220);};
 auto control=[&](ParamID id){for(auto c:mockup::controls(val,0,lfo,0,0,0,rack))if(c.id==id)return c.r;throw std::runtime_error("Control missing from page");};
 auto toggle=[&](ParamID id){auto r=control(id);double before=val(id);rectClick(r);check(val(id)==(before>=.5?0.:1.),"Visible toggle binding");c->setParamNormalized(id,before);};
 for(int stage:{0,2,3,4}){focus(stage);toggle(mockup::stageEnabled[stage]);}
 focus(0);toggle(kFreeze);toggle(kMixLock0);auto r=control(kSize);c->setParamNormalized(kSize,.25);drag(0,r.x+r.w/2,r.y+r.h/2);drag(1,r.x+r.w/2,r.y+r.h/2-45);drag(2,r.x+r.w/2,r.y+r.h/2-45);check(std::abs(val(kSize)-.5)<.015,"Round knob drag");rectClick(r,2);check(std::abs(val(kSize)-.25)<1e-6,"Double click default");
 c->setParamNormalized(kGrainMix,.271);c->setParamNormalized(kMixLock0,0);rectClick(control(kMixLock0));rectClick(mockup::randomRect(0,val,rack));check(val(kGrainMix)==.271,"Mix lock survives Random");rectClick(control(kMixLock0));rectClick(mockup::randomRect(0,val,rack));check(val(kGrainMix)!=.271,"Unlocked mix randomises");
 focus(3);auto rmode=control(kResliceAlgorithm);check(rmode.w>0,"Generative mode selector");double before=val(kResliceSeed);rectClick(mockup::randomRect(3,val,rack));check(val(kResliceSeed)!=before,"New procedural phrase seed");

 focus(4);r=mockup::stepRect(0,4,val,rack);before=val(kGaterState0);rectClick(r);check(val(kGaterState0)==(before>.25?0.:1.),"Gater sequencer after scroll");
 auto original=mockup::routeChain(val);focus(original[3]);r=mockup::headerRect(original[3],val,rack);drag(0,r.x+80,r.y+20);setScroll(0);r=mockup::headerRect(original[0],val,rack);drag(1,r.x+80,r.y+15);drag(2,r.x+80,r.y+15);auto moved=mockup::routeChain(val);check(moved[0]==original[3]&&moved[1]==original[0],"Vertical routing from bottom to first across scrolling");
 focus(moved[0]);r=mockup::headerRect(moved[0],val,rack);drag(0,r.x+80,r.y+20);setScroll(mockup::maxScroll(rack));r=mockup::moduleRect(moved[3],val,rack);drag(1,r.x+80,r.y+r.h-10);drag(2,r.x+80,r.y+r.h-10);check(mockup::routeChain(val)==original,"Vertical routing first to last across scrolling");
 focus(original[0]);r=mockup::headerRect(original[0],val,rack);before=val(kRoutingOrder);drag(0,r.x+80,r.y+20);drag(1,200,100);drag(2,200,100);check(val(kRoutingOrder)==before,"Drop over fixed header cancels");
 setScroll(mockup::maxScroll(rack));toggle(kInputDeclick);
 page(1);setScroll(0);rectClick(rack.position(mockup::lfoRect(2)));lfo=2;toggle(lfoID(2,lEnabled));setScroll(620);toggle(kReverbOn);r=control(kReverbMix);c->setParamNormalized(kReverbMix,.25);drag(0,r.x+r.w/2,r.y+60);drag(1,r.x+r.w/2,r.y+30);drag(2,r.x+r.w/2,r.y+30);check(val(kReverbMix)>.4,"Reverb fader binding");setScroll(mockup::maxScroll(rack));toggle(kMasterLimiter);
 page(2);setScroll(0);toggle(kXYEnable);r=rack.position(mockup::xy);click(r.x+12,r.y+12,1);check(val(kXYX)<.005&&val(kXYY)>.995,"Square Morph XY corner");c->setParamNormalized(kXYX,.5);c->setParamNormalized(kXYY,.5);
 rectClick(mockup::next);check(val(kRoutingOrder)==bankPreset(0,0)[kRoutingOrder],"Categorised preset 1");rectClick(mockup::next);check(val(kRoutingOrder)==bankPreset(0,1)[kRoutingOrder],"Categorised preset 2");
 page(0);setScroll(0);
}
}
