#define NOMINMAX
#include <windows.h>
#include "plugin.cpp"
#include "mockup_ui.h"
#include "gui_host_guard.h"
#include "gui_scenarios.h"
#include <iostream>
static void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main(){try{auto* c=new Controller;check(c->initialize(nullptr)==kResultOk,"Controller init");aztec::GuiHostGuard guard;c->setComponentHandler(&guard);auto* view=c->createView(ViewType::kEditor);HWND parent=CreateWindowExW(0,L"STATIC",L"GrainsDosage smoke",WS_OVERLAPPEDWINDOW,0,0,1100,1020,nullptr,nullptr,GetModuleHandle(nullptr),nullptr);check(view->attached(parent,kPlatformTypeHWND)==kResultOk,"Attach");HWND child=GetWindow(parent,GW_CHILD);check(SendMessageW(child,WM_APP+71,0,0)==1,"Current skin loaded");ShowWindow(parent,SW_SHOW);UpdateWindow(parent);UpdateWindow(child);
 auto send=[&](int action,double x,double y,int count=1){RECT r;GetClientRect(child,&r);aztec::mockup::Viewport fit(r.right,r.bottom);UINT msg=action==0?(count>1?WM_LBUTTONDBLCLK:WM_LBUTTONDOWN):action==1?WM_MOUSEMOVE:WM_LBUTTONUP;SendMessageW(child,msg,action==2?0:MK_LBUTTON,MAKELPARAM(int(fit.x+x*fit.scale),int(fit.y+y*fit.scale)));};auto click=[&](double x,double y,int n){send(0,x,y,n);send(2,x,y,n);};auto drag=[&](int a,double x,double y){send(a,x,y);};aztec::runGuiScenarios(c,click,drag,[&](double v){SendMessageW(child,WM_APP+73,0,LPARAM(std::round(v)));},[&](){return int(SendMessageW(child,WM_APP+72,0,0));});
 ViewRect stale(0,0,900,600);view->onSize(&stale);for(int page=0;page<3;++page){auto r=aztec::mockup::pageRect(page);click(r.x+80,r.y+20,1);check(SendMessageW(child,WM_APP+72,0,0)==page,"Stale host hit alignment");}ViewRect resized(0,0,816,759);check(view->checkSizeConstraint(&resized)==kResultOk,"Resize constraints");view->onSize(&resized);UpdateWindow(child);check(guard.rejectedTabEdits==0,"UI navigation stays local");view->removed();view->release();c->setComponentHandler(nullptr);c->terminate();c->release();DestroyWindow(parent);std::cout<<"PASS: native skin, three pages, vertical routing, scroll, Mix locks, sequencers, XY and resize\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
