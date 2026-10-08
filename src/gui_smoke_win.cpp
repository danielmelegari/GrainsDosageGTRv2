#define NOMINMAX
#include <windows.h>
#include "plugin.cpp"
#include "mockup_ui.h"
#include <stdexcept>
#include <iostream>
static void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main(){try{
 auto* c=new Controller;check(c->initialize(nullptr)==kResultOk,"Controller init");auto* view=c->createView(ViewType::kEditor);check(view,"Editor factory");
 HWND parent=CreateWindowExW(0,L"STATIC",L"GrainsDosage smoke",WS_OVERLAPPEDWINDOW,0,0,1100,1020,nullptr,nullptr,GetModuleHandle(nullptr),nullptr);check(parent,"Window");
 check(view->attached(parent,kPlatformTypeHWND)==kResultOk,"Attach");HWND child=GetWindow(parent,GW_CHILD);check(child,"Native child");
 check(SendMessageW(child,WM_APP+71,0,0)==1,"Supplied knob decoded");ShowWindow(parent,SW_SHOW);UpdateWindow(parent);UpdateWindow(child);
 auto send=[&](UINT msg,double x,double y){RECT r;GetClientRect(child,&r);SendMessageW(child,msg,msg==WM_LBUTTONUP?0:MK_LBUTTON,MAKELPARAM(int((x+aztec::mockup::rackInset)*r.right/aztec::mockup::width),int(y*r.bottom/aztec::mockup::height)));};
 auto click=[&](double x,double y){send(WM_LBUTTONDOWN,x,y);send(WM_LBUTTONUP,x,y);};
 auto toggle=[&](int id,double x,double y){double old=c->getParamNormalized(id);click(x,y);check(c->getParamNormalized(id)==(old>=.5?0.:1.),"Toggle binding");c->setParamNormalized(id,old);};
 using namespace aztec;
 toggle(kFreeze,440,442);toggle(kInputDeclick,1350,66);toggle(kXYEnable,1545,703);toggle(kMasterFilter,209,1158);toggle(kMasterLimiter,1505,1717);
 c->setParamNormalized(kSize,.25);send(WM_LBUTTONDOWN,158,290);send(WM_MOUSEMOVE,158,245);send(WM_LBUTTONUP,158,245);check(std::abs(c->getParamNormalized(kSize)-.5)<.015,"Knob drag");
 send(WM_LBUTTONDBLCLK,158,290);send(WM_LBUTTONUP,158,290);check(std::abs(c->getParamNormalized(kSize)-.25)<1e-6,"Double click reset");
 const ParamID enabled[]={kGrainEnabled,kGlitchEnabled,kRepeatEnabled,kResliceEnabled,kGaterEnabled};
 for(int t=0;t<5;++t){click(140+252*t,135);check(tabFromValue(c->getParamNormalized(kUiTab))==t,"Tab hit target");toggle(enabled[t],1300,(t==3?597:t==2?410:442));auto led=mockup::ledRect(t);toggle(enabled[t],led.x+led.w/2,led.y+led.h/2);UpdateWindow(child);}
 toggle(kGaterState0,77,536);click(140+3*252,135);toggle(kResliceRndOn,750,248);
 click(140,135);click(688+105*2,703);toggle(lfoID(2,lEnabled),540,703);
 click(mockup::xy.x+13,mockup::xy.y+13);check(c->getParamNormalized(kXYX)<.015&&c->getParamNormalized(kXYY)>.985,"XY corner");
 auto routeValue=[&](ParamID id){return c->getParamNormalized(id);};
 auto original=mockup::routeChain(routeValue);
 send(WM_LBUTTONDOWN,1140,138);send(WM_MOUSEMOVE,138,138);send(WM_LBUTTONUP,138,138);
 auto moved=mockup::routeChain(routeValue);check(moved[0]==original[4]&&moved[1]==original[0],"Routing inserts before first");
 click(138,138);check(tabFromValue(c->getParamNormalized(kUiTab))==original[4],"Moved top button selects its module");
 auto led0=mockup::ledRect(0);toggle(enabled[original[4]],led0.x+led0.w/2,led0.y+led0.h/2);
 send(WM_LBUTTONDOWN,138,138);send(WM_MOUSEMOVE,1270,138);send(WM_LBUTTONUP,1270,138);
 check(mockup::routeChain(routeValue)==original,"Routing inserts after last");
 double beforeCancel=c->getParamNormalized(kRoutingOrder);
 send(WM_LBUTTONDOWN,138,138);send(WM_MOUSEMOVE,100,100);send(WM_LBUTTONUP,100,100);
 check(c->getParamNormalized(kRoutingOrder)==beforeCancel,"Routing outside drop cancels");
 click(140,135);c->setParamNormalized(kGrainMix,.271);c->setParamNormalized(kMixLock0,0.);click(1220,404);check(c->getParamNormalized(kMixLock0)==1.,"Mix lock click");
 click(1000,442);check(c->getParamNormalized(kGrainMix)==.271,"Locked mix survives Random");click(1220,404);click(1000,442);check(c->getParamNormalized(kGrainMix)!=.271,"Unlocked mix randomises");
 c->setParamNormalized(kReverbMix,.25);send(WM_LBUTTONDOWN,116,1650);send(WM_MOUSEMOVE,116,1620);send(WM_LBUTTONUP,116,1620);check(c->getParamNormalized(kReverbMix)>.4,"Vertical reverb fader");
 click(987,52);check(c->getParamNormalized(kRoutingOrder)==factoryPreset(0)[kRoutingOrder],"First categorised preset");click(987,52);check(c->getParamNormalized(kRoutingOrder)==factoryPreset(1)[kRoutingOrder],"Next categorised preset");
 c->setParamNormalized(kReverbSource,1.);
 ViewRect resized(0,0,816,759);check(view->checkSizeConstraint(&resized)==kResultOk,"Resize constraint");check(view->onSize(&resized)==kResultOk,"Resize");UpdateWindow(child);
 view->removed();view->release();c->terminate();c->release();DestroyWindow(parent);std::cout<<"PASS: native skin, tabs, controls, XY and resize\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
