#define NOMINMAX
#include <windows.h>
#include <gdiplus.h>
#include "plugin.cpp"
#include "mockup_ui.h"
#include "gui_host_guard.h"
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"
#include "gui_scenarios.h"
#include <iostream>
#include <chrono>
static void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static void screenshot(HWND window,int page){
 RECT rect{};GetClientRect(window,&rect);HDC screen=GetDC(window),memory=CreateCompatibleDC(screen);
 HBITMAP bitmap=CreateCompatibleBitmap(screen,rect.right,rect.bottom);auto previous=SelectObject(memory,bitmap);
 check(BitBlt(memory,0,0,rect.right,rect.bottom,screen,0,0,SRCCOPY)!=0,"Screenshot capture");
 SelectObject(memory,previous);DeleteDC(memory);ReleaseDC(window,screen);
 const CLSID png={0x557cf406,0x1a04,0x11d3,{0x9a,0x73,0x00,0x00,0xf8,0x1e,0xf3,0x2e}};
 Gdiplus::Status result;{Gdiplus::Bitmap image(bitmap,nullptr);const auto path=L"windows-page-"+std::to_wstring(page+1)+L".png";result=image.Save(path.c_str(),&png,nullptr);}
 DeleteObject(bitmap);check(result==Gdiplus::Ok,"Screenshot PNG save");
}
static bool typedValueApplied=false;
static void CALLBACK typeValue(HWND,UINT,UINT_PTR timer,DWORD){KillTimer(nullptr,timer);EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM)->BOOL{if(GetDlgItem(w,101)){SetDlgItemTextW(w,101,L"123 ms");typedValueApplied=true;SendMessageW(w,WM_COMMAND,IDOK,0);return FALSE;}return TRUE;},0);}
int main(){try{auto* c=new Controller;check(c->initialize(nullptr)==kResultOk,"Controller init");aztec::GuiHostGuard guard;c->setComponentHandler(&guard);auto* view=c->createView(ViewType::kEditor);IPlugViewContentScaleSupport* dpi=nullptr;check(view->queryInterface(IPlugViewContentScaleSupport::iid,(void**)&dpi)==kResultOk,"DPI interface");
 ViewRect natural;view->getSize(&natural);check(dpi->setContentScaleFactor(1.5f)==kResultOk,"DPI before attach");ViewRect scaled;view->getSize(&scaled);check(std::abs(scaled.getWidth()-natural.getWidth()*1.5)<=1,"Initial DPI size");check(dpi->setContentScaleFactor(1.f)==kResultOk,"DPI reset");
 HWND parent=CreateWindowExW(0,L"STATIC",L"GrainsDosage smoke",WS_OVERLAPPEDWINDOW,0,0,1100,1020,nullptr,nullptr,GetModuleHandle(nullptr),nullptr);check(view->attached(parent,kPlatformTypeHWND)==kResultOk,"Attach");HWND child=GetWindow(parent,GW_CHILD);RECT parentSize{},childSize{};GetClientRect(parent,&parentSize);GetClientRect(child,&childSize);check(parentSize.right==childSize.right&&parentSize.bottom==childSize.bottom,"Initial parent/client alignment before any resize");
 SetWindowPos(parent,nullptr,0,0,1000,800,SWP_NOMOVE|SWP_NOZORDER);SendMessageW(child,WM_APP+74,0,0);GetClientRect(parent,&parentSize);GetClientRect(child,&childSize);check(parentSize.right==childSize.right&&parentSize.bottom==childSize.bottom,"Deferred host opening layout");
 check(dpi->setContentScaleFactor(1.25f)==kResultOk,"DPI after attach");view->getSize(&scaled);GetClientRect(child,&childSize);check(scaled.getWidth()==childSize.right&&scaled.getHeight()==childSize.bottom,"DPI client alignment");check(dpi->setContentScaleFactor(1.f)==kResultOk,"Return to 100% DPI");dpi->release();
 RECT desired{0,0,natural.getWidth(),natural.getHeight()};AdjustWindowRect(&desired,WS_OVERLAPPEDWINDOW,FALSE);SetWindowPos(parent,nullptr,0,0,desired.right-desired.left,desired.bottom-desired.top,SWP_NOMOVE|SWP_NOZORDER);view->onSize(&natural);
 check(SendMessageW(child,WM_APP+71,0,0)==1,"Current skin loaded");ShowWindow(parent,SW_SHOW);UpdateWindow(parent);UpdateWindow(child);
 // Repeated paints/resizes must not accumulate GDI handles. Warm the font
 // cache first so the steady-state count excludes intentional retained fonts.
 auto repaint=[&](){InvalidateRect(child,nullptr,FALSE);UpdateWindow(child);GdiFlush();};
 for(int i=0;i<2;++i){ViewRect size(0,0,natural.getWidth()+i*32,natural.getHeight()+i*24);view->onSize(&size);repaint();}
 view->onSize(&natural);repaint();const DWORD warmHandles=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS);
 check(warmHandles>0,"GDI resource count");
 for(int i=0;i<40;++i)repaint();
 check(GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS)<=warmHandles,"Paint GDI resources stay bounded");
 for(int i=0;i<12;++i){ViewRect size(0,0,natural.getWidth()+i%2*32,natural.getHeight()+i%2*24);view->onSize(&size);repaint();}
 view->onSize(&natural);repaint();
 check(GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS)<=warmHandles,"Resize releases previous render buffer");
 // Repeated knob updates must patch the cached scene, not redraw all artwork.
 {
  aztec::mockup::RackState rack;auto value=[&](aztec::ParamID id){return c->getParamNormalized(id);};
  auto module=aztec::mockup::moduleRect(0,value,rack);
  rack.scroll[0]=std::clamp(module.y-220.,0.,aztec::mockup::maxScroll(rack));
  SendMessageW(child,WM_APP+73,0,LPARAM(std::round(rack.offset())));repaint();
  auto builds=SendMessageW(child,WM_APP+75,0,0),patches=SendMessageW(child,WM_APP+76,0,0);
  double original=value(aztec::kSize),totalMs=0.,maxMs=0.;
  for(int i=0;i<20;++i){auto start=std::chrono::steady_clock::now();c->setParamNormalized(aztec::kSize,(i+1)/21.);repaint();
    double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();totalMs+=ms;maxMs=std::max(maxMs,ms);}
  std::cout<<"Cached knob repaint mean_ms="<<totalMs/20<<" max_ms="<<maxMs<<'\n';
  check(SendMessageW(child,WM_APP+75,0,0)==builds,"Knob updates reuse static artwork");
  check(SendMessageW(child,WM_APP+76,0,0)>=patches+19,"Knob updates patch cached controls");
  c->setParamNormalized(aztec::kSize,original);SendMessageW(child,WM_APP+73,0,0);repaint();
 }
 auto send=[&](int action,double x,double y,int count=1){RECT r;GetClientRect(child,&r);aztec::mockup::Viewport fit(r.right,r.bottom);UINT msg=action==0?(count>1?WM_LBUTTONDBLCLK:WM_LBUTTONDOWN):action==1?WM_MOUSEMOVE:WM_LBUTTONUP;SendMessageW(child,msg,action==2?0:MK_LBUTTON,MAKELPARAM(int(fit.x+x*fit.scale),int(fit.y+y*fit.scale)));};auto click=[&](double x,double y,int n){BYTE old[256]{},keys[256]{};GetKeyboardState(old);std::copy(old,old+256,keys);if(n<0){keys[VK_SHIFT]|=0x80;SetKeyboardState(keys);}send(0,x,y,n);send(2,x,y,n);SetKeyboardState(old);};auto drag=[&](int a,double x,double y){send(a,x,y);};aztec::runGuiScenarios(c,click,drag,[&](double v){SendMessageW(child,WM_APP+73,0,LPARAM(std::round(v)));},[&](){return int(SendMessageW(child,WM_APP+72,0,0));});
 {aztec::mockup::RackState rack;auto value=[&](aztec::ParamID id){return c->getParamNormalized(id);};auto module=aztec::mockup::moduleRect(0,value,rack);rack.scroll[0]=std::clamp(module.y-220.,0.,aztec::mockup::maxScroll(rack));SendMessageW(child,WM_APP+73,0,LPARAM(std::round(rack.offset())));aztec::mockup::Rect readout{};for(auto control:aztec::mockup::controls(value,0,0,0,0,0,rack))if(control.id==aztec::kSize)readout=aztec::mockup::valueRect(control.r,control.kind,control.id,control.label);UINT_PTR timer=SetTimer(nullptr,0,100,typeValue);check(timer!=0,"Value-entry timer");click(readout.x+readout.w/2,readout.y+readout.h/2,2);KillTimer(nullptr,timer);check(typedValueApplied&&std::abs(c->getParamNormalized(aztec::kSize)-(123.-15.)/235.)<1e-9,"Keyboard value dialog Apply");SendMessageW(child,WM_APP+73,0,0);}
 for(int page=0;page<3;++page){auto r=aztec::mockup::pageRect(page);click(r.x+80,r.y+20,1);SendMessageW(child,WM_APP+73,0,0);UpdateWindow(child);screenshot(child,page);}
 ViewRect stale(0,0,900,600);view->onSize(&stale);for(int page=0;page<3;++page){auto r=aztec::mockup::pageRect(page);click(r.x+80,r.y+20,1);check(SendMessageW(child,WM_APP+72,0,0)==page,"Stale host hit alignment");}ViewRect resized(0,0,816,759);check(view->checkSizeConstraint(&resized)==kResultOk,"Resize constraints");view->onSize(&resized);UpdateWindow(child);check(guard.rejectedTabEdits==0,"UI navigation stays local");view->removed();view->release();c->setComponentHandler(nullptr);c->terminate();c->release();DestroyWindow(parent);std::cout<<"PASS: native skin, three pages, vertical routing, scroll, Mix locks, sequencers, XY, keyboard entry and resize\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
