#define NOMINMAX
#include <windows.h>
#include "plugin.cpp"
#include <stdexcept>
#include <iostream>
static void check(bool v,const char* why){if(!v)throw std::runtime_error(why);}
int main(){try{
  auto* c=new Controller;check(c->initialize(nullptr)==kResultOk,"controller");auto* view=c->createView(ViewType::kEditor);check(view,"factory");
  HWND parent=CreateWindowExW(0,L"STATIC",L"GrainsDosage smoke",WS_OVERLAPPEDWINDOW,0,0,1100,760,nullptr,nullptr,GetModuleHandle(nullptr),nullptr);check(parent,"parent");
  check(view->attached(parent,kPlatformTypeHWND)==kResultOk,"attach");HWND child=GetWindow(parent,GW_CHILD);check(child,"native child");check(SendMessageW(child,WM_APP+71,0,0)==1,"embedded skin decoded");
  ShowWindow(parent,SW_SHOW);UpdateWindow(parent);UpdateWindow(child);
  auto send=[&](UINT msg,int x,int y){SendMessageW(child,msg,msg==WM_LBUTTONUP?0:MK_LBUTTON,MAKELPARAM(int(x*.6),int(y*.6)));};
  send(WM_LBUTTONDOWN,96,194);send(WM_MOUSEMOVE,96,149);send(WM_LBUTTONUP,96,149);check(std::abs(c->getParamNormalized(aztec::kSize)-(.25+27./.6/180.))<.001,"knob");
  double before=c->getParamNormalized(aztec::kPitch),mix=c->getParamNormalized(aztec::kGlitchMix);
  send(WM_LBUTTONDOWN,370,120);send(WM_LBUTTONUP,370,120);check(c->getParamNormalized(aztec::kSize)!=.5||c->getParamNormalized(aztec::kPitch)!=before,"random");check(c->getParamNormalized(aztec::kGlitchMix)==mix,"random isolation");
  send(WM_LBUTTONDOWN,150,120);send(WM_MOUSEMOVE,600,120);send(WM_LBUTTONUP,600,120);check(std::abs(c->getParamNormalized(aztec::kModuleOrder)-2./6.)<.001,"reorder");
  send(WM_LBUTTONDOWN,489,1065);send(WM_LBUTTONUP,489,1065);check(c->getParamNormalized(aztec::kMasterLimiter)==0.,"limiter toggle");
  send(WM_LBUTTONDOWN,197,908);send(WM_LBUTTONUP,197,908);check(c->getParamNormalized(aztec::kMasterFilter)==1.,"filter toggle");
  send(WM_LBUTTONDOWN,364,1065);send(WM_LBUTTONUP,364,1065);check(c->getParamNormalized(aztec::kNormalize)==1.,"normalize");
  send(WM_LBUTTONDOWN,745,908);send(WM_LBUTTONUP,745,908);check(c->getParamNormalized(aztec::kReverbOn)==1.,"reverb");
  send(WM_LBUTTONDOWN,603,995);send(WM_LBUTTONUP,603,995);check(c->getParamNormalized(aztec::kReverbStep0)==0.,"reverb step");
  send(WM_LBUTTONDOWN,724,120);send(WM_LBUTTONUP,724,120);check(c->getParamNormalized(aztec::kGrainEnabled)==0.,"moved module bypass");
  c->setParamNormalized(aztec::kPanMode,.5);check(c->getParamNormalized(aztec::kPanMode)==.5,"alternate pan");
  c->setParamNormalized(aztec::kPanMode,0.);check(c->getParamNormalized(aztec::kPanMode)==0.,"manual pan");
  c->setParamNormalized(aztec::kGrainPan,.5);send(WM_LBUTTONDOWN,690,414);send(WM_MOUSEMOVE,727,414);send(WM_LBUTTONUP,727,414);check(c->getParamNormalized(aztec::kGrainPan)>.59,"pan slider");
  send(WM_LBUTTONDOWN,876,908);send(WM_LBUTTONUP,876,908);check(c->getParamNormalized(aztec::kReverbKill)==1.,"kill dry");
  ViewRect r(0,0,792,660);check(view->onSize(&r)==kResultOk,"resize");
  send(WM_LBUTTONDOWN,182,792);send(WM_LBUTTONUP,182,792);check(c->getParamNormalized(aztec::kGaterEnabled)==1.,"gater on");
  send(WM_LBUTTONDOWN,842,792);send(WM_LBUTTONUP,842,792);send(WM_LBUTTONDOWN,67,843);send(WM_LBUTTONUP,67,843);check(c->getParamNormalized(aztec::kGaterState0)==1.,"paint dry step");
  send(WM_LBUTTONDOWN,500,67);send(WM_LBUTTONUP,500,67);check(c->getParamNormalized(aztec::kInputDeclick)==0.,"input de-click toggle");
  view->removed();check(GetWindow(parent,GW_CHILD)==nullptr,"detach");view->release();c->terminate();c->release();DestroyWindow(parent);
  std::cout<<"PASS: Windows native GUI attach, knob, random isolation, module drag, resize, detach\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
