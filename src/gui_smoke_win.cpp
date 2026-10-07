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
 auto send=[&](UINT msg,double x,double y){RECT r;GetClientRect(child,&r);SendMessageW(child,msg,msg==WM_LBUTTONUP?0:MK_LBUTTON,MAKELPARAM(int(x*r.right/aztec::mockup::width),int(y*r.bottom/aztec::mockup::height)));};
 auto click=[&](double x,double y){send(WM_LBUTTONDOWN,x,y);send(WM_LBUTTONUP,x,y);};
 auto toggle=[&](int id,double x,double y){double old=c->getParamNormalized(id);click(x,y);check(c->getParamNormalized(id)==(old>=.5?0.:1.),"Toggle binding");c->setParamNormalized(id,old);};
 using namespace aztec;
 toggle(kFreeze,440,526);toggle(kInputDeclick,1350,66);toggle(kXYEnable,1160,928);toggle(kMasterFilter,209,1158);toggle(kMasterLimiter,700,1481);
 c->setParamNormalized(kSize,.25);send(WM_LBUTTONDOWN,158,338);send(WM_MOUSEMOVE,158,293);send(WM_LBUTTONUP,158,293);check(std::abs(c->getParamNormalized(kSize)-.5)<.015,"Knob drag");
 send(WM_LBUTTONDBLCLK,158,338);send(WM_LBUTTONUP,158,338);check(std::abs(c->getParamNormalized(kSize)-.25)<1e-6,"Double click reset");
 const ParamID enabled[]={kGrainEnabled,kGlitchEnabled,kRepeatEnabled,kResliceEnabled,kGaterEnabled};
 for(int t=0;t<5;++t){click(140+256*t,135);check(tabFromValue(c->getParamNormalized(kUiTab))==t,"Tab hit target");toggle(enabled[t],1190,526);UpdateWindow(child);}
 toggle(kGaterState0,77,650);click(140+3*256,135);toggle(kResliceRndOn,700,273);
 click(140,135);click(688+105*2,885);toggle(lfoID(2,lEnabled),540,882);
 click(mockup::xy.x+13,mockup::xy.y+13);check(c->getParamNormalized(kXYX)<.015&&c->getParamNormalized(kXYY)>.985,"XY corner");
 ViewRect resized(0,0,816,759);check(view->checkSizeConstraint(&resized)==kResultOk,"Resize constraint");check(view->onSize(&resized)==kResultOk,"Resize");UpdateWindow(child);
 view->removed();view->release();c->terminate();c->release();DestroyWindow(parent);std::cout<<"PASS: native skin, tabs, controls, XY and resize\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
