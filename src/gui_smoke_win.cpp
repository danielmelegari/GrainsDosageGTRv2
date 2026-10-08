#define NOMINMAX
#include <windows.h>
#include "plugin.cpp"
#include "mockup_ui.h"
#include "gui_host_guard.h"
#include <stdexcept>
#include <iostream>
static void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main(){try{
 auto* c=new Controller;check(c->initialize(nullptr)==kResultOk,"Controller init");aztec::GuiHostGuard hostGuard;c->setComponentHandler(&hostGuard);auto* view=c->createView(ViewType::kEditor);check(view,"Editor factory");
 HWND parent=CreateWindowExW(0,L"STATIC",L"GrainsDosage smoke",WS_OVERLAPPEDWINDOW,0,0,1100,1020,nullptr,nullptr,GetModuleHandle(nullptr),nullptr);check(parent,"Window");
 check(view->attached(parent,kPlatformTypeHWND)==kResultOk,"Attach");HWND child=GetWindow(parent,GW_CHILD);check(child,"Native child");
 check(SendMessageW(child,WM_APP+71,0,0)==1,"Approved rack assets decoded");ShowWindow(parent,SW_SHOW);UpdateWindow(parent);UpdateWindow(child);
 auto send=[&](UINT msg,double x,double y){RECT r;GetClientRect(child,&r);SendMessageW(child,msg,msg==WM_LBUTTONUP?0:MK_LBUTTON,MAKELPARAM(int(aztec::mockup::Viewport(r.right,r.bottom).x+x*aztec::mockup::Viewport(r.right,r.bottom).scale),int(aztec::mockup::Viewport(r.right,r.bottom).y+y*aztec::mockup::Viewport(r.right,r.bottom).scale)));};
 auto click=[&](double x,double y){send(WM_LBUTTONDOWN,x,y);send(WM_LBUTTONUP,x,y);};
 auto toggle=[&](int id,double x,double y){double old=c->getParamNormalized(id);click(x,y);check(c->getParamNormalized(id)==(old>=.5?0.:1.),"Toggle binding");c->setParamNormalized(id,old);};
 using namespace aztec;
 auto controlRect=[&](ParamID id){auto list=mockup::controls([&](ParamID p){return c->getParamNormalized(p);},tabFromValue(c->getParamNormalized(kUiTab)),id==lfoID(2,lEnabled)?2:0,0,0,0);for(const auto& item:list)if(item.id==id)return item.r;throw std::runtime_error("Missing control");};
 auto toggleControl=[&](ParamID id){auto r=controlRect(id);toggle(id,r.x+r.w/2,r.y+r.h/2);};
 auto clickRect=[&](mockup::Rect r){click(r.x+r.w/2,r.y+r.h/2);};
 auto chooseTab=[&](int t){auto r=mockup::tabRect(t);click(r.x+r.w*.4,r.y+r.h/2);};
 toggleControl(kFreeze);toggleControl(kInputDeclick);toggleControl(kXYEnable);toggleControl(kMasterFilter);toggleControl(kMasterLimiter);
 auto sizeRect=controlRect(kSize);double knobX=sizeRect.x+sizeRect.w/2,knobY=sizeRect.y+sizeRect.h/2;
 c->setParamNormalized(kSize,.25);send(WM_LBUTTONDOWN,knobX,knobY);send(WM_MOUSEMOVE,knobX,knobY-45);send(WM_LBUTTONUP,knobX,knobY-45);check(std::abs(c->getParamNormalized(kSize)-.5)<.015,"Knob drag");
 send(WM_LBUTTONDBLCLK,knobX,knobY);send(WM_LBUTTONUP,knobX,knobY);check(std::abs(c->getParamNormalized(kSize)-.25)<1e-6,"Double click reset");
 const ParamID enabled[]={kGrainEnabled,kGlitchEnabled,kRepeatEnabled,kResliceEnabled,kGaterEnabled};
 for(int t=0;t<5;++t){chooseTab(t);check(tabFromValue(c->getParamNormalized(kUiTab))==t,"Tab hit target");toggleControl(enabled[t]);auto led=mockup::ledRect(t);toggle(enabled[t],led.x+led.w/2,led.y+led.h/2);UpdateWindow(child);}
 toggle(kGaterState0,mockup::stepRect(0,4).x+12,mockup::stepRect(0,4).y+20);chooseTab(3);toggleControl(kResliceRndOn);
 chooseTab(0);clickRect(mockup::lfoRect(2));toggleControl(lfoID(2,lEnabled));
 click(mockup::xy.x+13,mockup::xy.y+13);check(c->getParamNormalized(kXYX)<.015&&c->getParamNormalized(kXYY)>.985,"XY corner");
 auto routeValue=[&](ParamID id){return c->getParamNormalized(id);};
 auto original=mockup::routeChain(routeValue);
 send(WM_LBUTTONDOWN,mockup::tabRect(4).x+40,175);send(WM_MOUSEMOVE,mockup::tabRect(0).x+20,175);send(WM_LBUTTONUP,mockup::tabRect(0).x+20,175);
 auto moved=mockup::routeChain(routeValue);check(moved[0]==original[4]&&moved[1]==original[0],"Routing inserts before first");
 click(mockup::tabRect(0).x+20,175);check(tabFromValue(c->getParamNormalized(kUiTab))==original[4],"Moved top button selects its module");
 auto led0=mockup::ledRect(0);toggle(enabled[original[4]],led0.x+led0.w/2,led0.y+led0.h/2);
 send(WM_LBUTTONDOWN,mockup::tabRect(0).x+20,175);send(WM_MOUSEMOVE,mockup::tabRect(4).x+mockup::tabRect(4).w-3,175);send(WM_LBUTTONUP,mockup::tabRect(4).x+mockup::tabRect(4).w-3,175);
 check(mockup::routeChain(routeValue)==original,"Routing inserts after last");
 double beforeCancel=c->getParamNormalized(kRoutingOrder);
 send(WM_LBUTTONDOWN,mockup::tabRect(0).x+20,175);send(WM_MOUSEMOVE,100,100);send(WM_LBUTTONUP,100,100);
 check(c->getParamNormalized(kRoutingOrder)==beforeCancel,"Routing outside drop cancels");
 chooseTab(0);c->setParamNormalized(kGrainMix,.271);c->setParamNormalized(kMixLock0,0.);clickRect(controlRect(kMixLock0));check(c->getParamNormalized(kMixLock0)==1.,"Mix lock click");
 clickRect(mockup::randomRect(0));check(c->getParamNormalized(kGrainMix)==.271,"Locked mix survives Random");clickRect(controlRect(kMixLock0));clickRect(mockup::randomRect(0));check(c->getParamNormalized(kGrainMix)!=.271,"Unlocked mix randomises");
 c->setParamNormalized(kReverbMix,.25);send(WM_LBUTTONDOWN,113,1310);send(WM_MOUSEMOVE,113,1280);send(WM_LBUTTONUP,113,1280);check(c->getParamNormalized(kReverbMix)>.4,"Vertical reverb fader");
 clickRect(mockup::next);check(c->getParamNormalized(kRoutingOrder)==factoryPreset(0)[kRoutingOrder],"First categorised preset");clickRect(mockup::next);check(c->getParamNormalized(kRoutingOrder)==factoryPreset(1)[kRoutingOrder],"Next categorised preset");
 c->setParamNormalized(kReverbSource,1.);
 ViewRect staleHostSize(0,0,900,600);view->onSize(&staleHostSize);for(int slot=0;slot<5;++slot)chooseTab(slot);
 ViewRect resized(0,0,816,759);check(view->checkSizeConstraint(&resized)==kResultOk,"Resize constraint");check(view->onSize(&resized)==kResultOk,"Resize");UpdateWindow(child);
 check(hostGuard.rejectedTabEdits==0,"Module selection never edits a host read-only parameter");
 view->removed();view->release();c->setComponentHandler(nullptr);c->terminate();c->release();DestroyWindow(parent);std::cout<<"PASS: native skin, tabs, controls, XY and resize\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
