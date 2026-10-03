#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include "preset_io.h"
#include "factory_presets.h"
#include <shlobj.h>
#include <shellapi.h>
#include <gdiplus.h>
#include <objidl.h>
#include "editor.h"
#include "parameters.h"
#include "skin_spec.h"
#include "skin_theme.h"
#include "filter_sequencer.h"
#include "randomize.h"
#include "public.sdk/source/common/pluginview.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>
namespace aztec {
using namespace Steinberg;using namespace Steinberg::Vst;
// Astral/Filigree Green: resolve a shared skin colour to a Win32 COLORREF.
static inline COLORREF C(skin::Rgb k){return RGB(k.r,k.g,k.b);}
// Resolve a colour ROLE against the live (theme-aware) palette.
static inline COLORREF C(int role){return C(skin::active().at(role));}
static inline skin::Rgb C2(int role){return skin::active().at(role);}
#define AZSKIN(k) C(int(aztec::skin::k))  // COLORREF from live palette role
#define ONTEXT() AZSKIN(kOnText)             // dark text for lit green fills
constexpr double slotX[]={16,452,888};
enum Kind{Knob,Slider,Toggle,Select,Pad,Pan,PanMode};
struct Control{ParamID id;double x,y,w,h;Kind kind;const char* label;};
class WinEditor final:public CPluginView{
  EditController* controller;HWND window=nullptr;std::vector<Control> controls;
  int selectedGate=0,selectedReslice=0;
  int selectedLfo=0,selectedRepeat=0,dragID=-1,dragSlot=-1,dropSlot=-1;
  double originX=0,originY=0,dragValue=0,dragWidth=1;Kind dragKind=Knob;bool dragXY=false;
  uint32_t seed=0;HDC dc=nullptr;
  ULONG_PTR imaging=0;HBITMAP skin=nullptr;HDC skinDC=nullptr;HGDIOBJ oldSkin=nullptr;std::array<HBITMAP,6> sprites{};std::array<HDC,6> spriteDC{};std::array<HGDIOBJ,6> oldSprite{};
  void loadSprite(int id,int index){
    HMODULE module=nullptr;GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&proc),&module);
    HRSRC resource=FindResourceW(module,MAKEINTRESOURCEW(id),MAKEINTRESOURCEW(10));if(!resource)return;DWORD bytes=SizeofResource(module,resource);auto loaded=LoadResource(module,resource);const void* data=LockResource(loaded);if(!data||!bytes)return;
    HGLOBAL memory=GlobalAlloc(GMEM_MOVEABLE,bytes);if(!memory)return;void* dest=GlobalLock(memory);if(!dest){GlobalFree(memory);return;}std::memcpy(dest,data,bytes);GlobalUnlock(memory);IStream* stream=nullptr;if(FAILED(CreateStreamOnHGlobal(memory,TRUE,&stream))){GlobalFree(memory);return;}
    {Gdiplus::Bitmap bitmap(stream);if(bitmap.GetLastStatus()==Gdiplus::Ok)bitmap.GetHBITMAP(Gdiplus::Color(255,12,9,20),&sprites[size_t(index)]);}stream->Release();if(sprites[size_t(index)]){spriteDC[size_t(index)]=CreateCompatibleDC(nullptr);if(spriteDC[size_t(index)])oldSprite[size_t(index)]=SelectObject(spriteDC[size_t(index)],sprites[size_t(index)]);}
  }
  void loadSkin(){
    Gdiplus::GdiplusStartupInput startup;
    if(Gdiplus::GdiplusStartup(&imaging,&startup,nullptr)!=Gdiplus::Ok){imaging=0;return;}
    HMODULE module=nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&proc),&module);
    HRSRC resource=FindResourceW(module,MAKEINTRESOURCEW(201),MAKEINTRESOURCEW(10));
    if(!resource)return;DWORD bytes=SizeofResource(module,resource);auto loaded=LoadResource(module,resource);const void* data=LockResource(loaded);if(!data||!bytes)return;
    HGLOBAL memory=GlobalAlloc(GMEM_MOVEABLE,bytes);if(!memory)return;
    void* dest=GlobalLock(memory);if(!dest){GlobalFree(memory);return;}std::memcpy(dest,data,bytes);GlobalUnlock(memory);
    IStream* stream=nullptr;if(FAILED(CreateStreamOnHGlobal(memory,TRUE,&stream))){GlobalFree(memory);return;}
    {Gdiplus::Bitmap bitmap(stream);if(bitmap.GetLastStatus()==Gdiplus::Ok)bitmap.GetHBITMAP(Gdiplus::Color(255,12,9,20),&skin);}
    stream->Release();if(skin){skinDC=CreateCompatibleDC(nullptr);if(skinDC)oldSkin=SelectObject(skinDC,skin);}for(int i=0;i<6;++i)loadSprite(202+i,i);
  }
  void art(int sx,int sy,int sw,int sh,double x,double y,double w,double h){
    const int coordinates[5][4]={{385,260,76,76},{453,636,24,32},{734,456,54,48},{1850,12,136,104},{1726,15,95,104}};for(int i=0;i<5;++i)if(sx==coordinates[i][0]&&sy==coordinates[i][1]&&sw==coordinates[i][2]&&sh==coordinates[i][3]&&spriteDC[size_t(i)]){BITMAP b{};GetObjectW(sprites[size_t(i)],sizeof(b),&b);int old=SetStretchBltMode(dc,HALFTONE);StretchBlt(dc,int(x),int(y),int(w),int(h),spriteDC[size_t(i)],0,0,b.bmWidth,b.bmHeight,SRCCOPY);SetStretchBltMode(dc,old);return;}
    if(!skinDC)return;int old=SetStretchBltMode(dc,HALFTONE);POINT origin;SetBrushOrgEx(dc,0,0,&origin);
    StretchBlt(dc,int(x),int(y),int(w),int(h),skinDC,sx,sy,sw,sh,SRCCOPY);
    SetBrushOrgEx(dc,origin.x,origin.y,nullptr);SetStretchBltMode(dc,old);
  }
  void disc(double cx,double cy,double radius,COLORREF fill,COLORREF edge){
    HBRUSH brush=CreateSolidBrush(fill);HPEN pen=CreatePen(PS_SOLID,1,edge);
    auto oldBrush=SelectObject(dc,brush),oldPen=SelectObject(dc,pen);
    Ellipse(dc,int(cx-radius),int(cy-radius),int(cx+radius),int(cy+radius));
    SelectObject(dc,oldBrush);SelectObject(dc,oldPen);DeleteObject(brush);DeleteObject(pen);
  }
  void knobCap(double cx,double cy,double value){
    (void)value;
    disc(cx,cy+2,25,AZSKIN(kWell),AZSKIN(kWell));
    disc(cx,cy,23,C(skin::mix(skin::active().at(skin::kPanel),skin::active().at(skin::kWell),.5)),AZSKIN(kFiligree));
    disc(cx,cy-1,20,C(skin::mix(skin::active().at(skin::kPanelRaised),skin::active().at(skin::kWell),.35)),AZSKIN(kFiligreeDim));
    disc(cx,cy-2,17,C(skin::active().at(skin::kBorderDark)),C(skin::active().at(skin::kBorderDark)));
  }
  void panel(double x,double y,double w,double h){
    box(x,y,w,h,AZSKIN(kPanel),AZSKIN(kFiligree));
    box(x+3,y+3,w-6,h-6,AZSKIN(kPanel),AZSKIN(kBorderDark));
    line(x+15,y+5,x+w-15,y+5,AZSKIN(kFiligree));
    for(int side=0;side<2;++side){double xx=side?x+w-5:x+5;
      line(xx,y+14,xx+(side?-2:2),y+h*.33,AZSKIN(kFiligreeDim));
      line(xx+(side?-2:2),y+h*.33,xx+(side?2:-2),y+h*.66,AZSKIN(kFiligreeDim));
      line(xx+(side?2:-2),y+h*.66,xx,y+h-14,AZSKIN(kFiligreeDim));
    }
  }
  double value(ParamID id)const{return controller->getParamNormalized(id);}
  ParameterInfo info(ParamID id)const{auto* p=controller->getParameterObject(id);return p?p->getInfo():ParameterInfo{};}
  void change(ParamID id,double v){auto p=info(id);v=std::clamp(v,0.,1.);if(p.stepCount)v=std::round(v*p.stepCount)/p.stepCount;controller->setParamNormalized(id,v);controller->performEdit(id,v);InvalidateRect(window,nullptr,FALSE);}
  void edit(ParamID id,double v){controller->beginEdit(id);change(id,v);controller->endEdit(id);}
  std::wstring display(ParamID id,double v){String128 s{};controller->getParamStringByValue(id,v,s);std::wstring out(reinterpret_cast<wchar_t*>(s));auto p=info(id);
    if(!(p.flags&ParameterInfo::kIsList)&&out.find(L'.')!=std::wstring::npos){wchar_t b[64];swprintf(b,64,L"%.2f",wcstod(out.c_str(),nullptr));out=b;while(out.back()==L'0')out.pop_back();if(out.back()==L'.')out.pop_back();}
    if(p.units[0]){out+=L" ";out+=reinterpret_cast<wchar_t*>(p.units);}return out;}
  std::wstring presetName=L"PRESETS ▾";
  void loadSkinArt(){aztec::theme::loadSkinFile(aztec::theme::defaultPath());}
  void saveSkin(){aztec::theme::saveSkinFile(aztec::theme::defaultPath());}
  void skinMenu(){
    HMENU m=CreatePopupMenu();int n=1;for(size_t i=0;i<aztec::theme::themes().size();++i){auto& t=aztec::theme::themes()[i];std::wstring title;for(const char* c=t.title;*c;++c)title+=wchar_t(*c);AppendMenuW(m,MF_STRING,n++,title.c_str());}
    AppendMenuW(m,MF_SEPARATOR,0,nullptr);
    int origId=n++;AppendMenuW(m,MF_STRING,origId,L"Original (Astral)");
    int reloadId=n++;AppendMenuW(m,MF_STRING,reloadId,L"Reload skin.txt");
    // Check-mark the live theme so the current GUI is obvious at a glance.
    CheckMenuItem(m,UINT(aztec::theme::currentTheme())+1,MF_BYCOMMAND|(aztec::theme::customized()?MF_UNCHECKED:MF_CHECKED));
    if(aztec::theme::currentTheme()==0&&!aztec::theme::customized())CheckMenuItem(m,origId,MF_BYCOMMAND|MF_CHECKED);
    POINT p;GetCursorPos(&p);int pick=TrackPopupMenu(m,TPM_RETURNCMD,p.x,p.y,0,window,nullptr);DestroyMenu(m);
    if(!pick)return;
    if(pick==reloadId){loadSkinArt();}
    else if(pick==origId){aztec::theme::applySkin(0,{});DeleteFileW(L"grainsdosage-skin.txt");}
    else{aztec::theme::applySkin(pick-1,{});saveSkin();}
    InvalidateRect(window,nullptr,FALSE);
  }
  std::wstring presetFolder(){
    wchar_t root[MAX_PATH]{};if(FAILED(SHGetFolderPathW(window,CSIDL_APPDATA|CSIDL_FLAG_CREATE,nullptr,SHGFP_TYPE_CURRENT,root)))return {};
    std::wstring folder=std::wstring(root)+L"\\GrainsDosage";
    if(!CreateDirectoryW(folder.c_str(),nullptr)&&GetLastError()!=ERROR_ALREADY_EXISTS)return {};
    folder+=L"\\Presets";if(!CreateDirectoryW(folder.c_str(),nullptr)&&GetLastError()!=ERROR_ALREADY_EXISTS)return {};
    for(int i=0;i<factoryPresetCount;++i){std::string name=factoryNames[i];std::wstring path=folder+L"\\"+std::wstring(name.begin(),name.end())+L".gdspreset";
      HANDLE f=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
      if(f==INVALID_HANDLE_VALUE){if(GetLastError()==ERROR_FILE_EXISTS||GetLastError()==ERROR_ALREADY_EXISTS)continue;return {};}
      auto p=factoryPreset(i);auto text=encodePreset([&](int id){return p[id];});DWORD n=0;bool ok=WriteFile(f,text.data(),DWORD(text.size()),&n,nullptr)&&n==text.size();CloseHandle(f);if(!ok){DeleteFileW(path.c_str());return {};}
    }return folder;
  }
  void setPresetName(const std::wstring& path){auto pos=path.find_last_of(L"\\/");presetName=path.substr(pos==std::wstring::npos?0:pos+1);auto dot=presetName.find_last_of(L'.');if(dot!=std::wstring::npos)presetName.resize(dot);}
  // PRESET < / > buttons: cycle through the sorted preset folder (factory + user files).
  void stepPreset(int direction){
    auto folder=presetFolder();if(folder.empty()){MessageBoxW(window,L"Cannot create or populate the preset folder.",L"GrainsDosage",MB_OK|MB_ICONERROR);return;}
    auto name=nextPresetFile(folder,direction,presetName+L".gdspreset");if(name.empty())return;
    if(!loadPresetPath(folder+L"\\"+name))MessageBoxW(window,L"Invalid or incompatible preset. No settings changed.",L"GrainsDosage",MB_OK|MB_ICONERROR);
  }
  bool loadPresetPath(const std::wstring& path){
    HANDLE file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);if(file==INVALID_HANDLE_VALUE)return false;
    std::array<double,kCount> p{};LARGE_INTEGER size{};bool ok=false;
    if(GetFileSizeEx(file,&size)&&size.QuadPart>0&&size.QuadPart<=65536){std::string text(size_t(size.QuadPart),'\0');DWORD n=0;ok=ReadFile(file,text.data(),DWORD(text.size()),&n,nullptr)&&n==text.size()&&decodePreset(text,p);}CloseHandle(file);
    if(ok){for(int i=0;i<kCount;++i)if(presetParameter(i))edit(i,p[i]);setPresetName(path);}return ok;
  }
  void presetMenu(){
    auto folder=presetFolder();if(folder.empty()){MessageBoxW(window,L"Cannot create or populate the preset folder.",L"GrainsDosage",MB_OK|MB_ICONERROR);return;}
    std::vector<std::wstring> names;WIN32_FIND_DATAW data{};HANDLE search=FindFirstFileW((folder+L"\\*.gdspreset").c_str(),&data);
    if(search!=INVALID_HANDLE_VALUE){do{if(!(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))names.emplace_back(data.cFileName);}while(FindNextFileW(search,&data));FindClose(search);}
    std::sort(names.begin(),names.end());HMENU menu=CreatePopupMenu();for(size_t i=0;i<names.size();++i){auto label=names[i].substr(0,names[i].find_last_of(L'.'));AppendMenuW(menu,MF_STRING,UINT_PTR(i+1),label.c_str());}
    AppendMenuW(menu,MF_SEPARATOR,0,nullptr);AppendMenuW(menu,MF_STRING,UINT_PTR(names.size()+1),L"Open Preset Folder…");POINT point;GetCursorPos(&point);int choice=TrackPopupMenu(menu,TPM_RETURNCMD,point.x,point.y,0,window,nullptr);DestroyMenu(menu);
    if(choice==int(names.size()+1))ShellExecuteW(window,L"open",folder.c_str(),nullptr,nullptr,SW_SHOWNORMAL);
    else if(choice>0&&choice<=int(names.size())&&!loadPresetPath(folder+L"\\"+names[size_t(choice-1)]))MessageBoxW(window,L"Invalid or incompatible preset. No settings changed.",L"GrainsDosage",MB_OK|MB_ICONERROR);
    InvalidateRect(window,nullptr,FALSE);
  }
  void preset(bool save){
    wchar_t path[MAX_PATH]=L"GrainsDosage.gdspreset";
    OPENFILENAMEW dialog{};dialog.lStructSize=sizeof(dialog);dialog.hwndOwner=window;
    dialog.lpstrFilter=L"GrainsDosage Preset (*.gdspreset)\0*.gdspreset\0\0";dialog.lpstrFile=path;dialog.nMaxFile=MAX_PATH;dialog.lpstrDefExt=L"gdspreset";
    auto folder=presetFolder();dialog.lpstrInitialDir=folder.empty()?nullptr:folder.c_str();
    dialog.Flags=OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST|(save?OFN_OVERWRITEPROMPT:OFN_FILEMUSTEXIST);
    if(!(save?GetSaveFileNameW(&dialog):GetOpenFileNameW(&dialog))){if(CommDlgExtendedError())MessageBoxW(window,L"Could not open the preset dialog.",L"GrainsDosage",MB_OK|MB_ICONERROR);return;}
    bool ok=false;
    if(save){
      auto data=encodePreset([&](int id){return value(id);});std::wstring full(path);auto slash=full.find_last_of(L"\\/");std::wstring directory=slash==std::wstring::npos?L".":full.substr(0,slash);wchar_t temporary[MAX_PATH]{};
      if(GetTempFileNameW(directory.c_str(),L"GDP",0,temporary)){
        HANDLE file=CreateFileW(temporary,GENERIC_WRITE,0,nullptr,TRUNCATE_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(file!=INVALID_HANDLE_VALUE){DWORD written=0;ok=WriteFile(file,data.data(),DWORD(data.size()),&written,nullptr)&&written==data.size();if(ok)ok=FlushFileBuffers(file)!=0;CloseHandle(file);}
        if(ok)ok=MoveFileExW(temporary,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
        if(!ok)DeleteFileW(temporary);
      }
    }else{ok=loadPresetPath(path);}
    if(ok){setPresetName(path);InvalidateRect(window,nullptr,FALSE);}
    if(!ok)MessageBoxW(window,save?L"Could not save the preset. The existing file was preserved.":L"Invalid, unreadable or incompatible preset. No settings changed.",L"GrainsDosage",MB_OK|MB_ICONERROR);
  }
  int order()const{return std::clamp(int(std::round(value(kModuleOrder)*6)),0,6);}
  int stage(int i)const{return moduleOrders[order()==6?0:order()][i];}
  int slot(int st)const{for(int i=0;i<3;++i)if(stage(i)==st)return i;return 0;}
  void layout(){controls.clear();
#define ADD(ID,X,Y,W,H,K,L) controls.push_back({ParamID(ID),double(X),double(Y),double(W),double(H),K,L})
#include "editor_layout_win.inl"
#undef ADD
  }
  static bool inside(double x,double y,double a,double b,double w,double h){return x>=a&&x<a+w&&y>=b&&y<b+h;}
  void point(LPARAM lp,double& x,double& y){RECT r;GetClientRect(window,&r);x=GET_X_LPARAM(lp)*1320./std::max(1L,r.right);y=GET_Y_LPARAM(lp)*1360./std::max(1L,r.bottom);}
  static COLORREF green(){return AZSKIN(kAccent);}static COLORREF cream(){return AZSKIN(kCream);}static COLORREF dark(){return AZSKIN(kBgDeep);}
  void box(double x,double y,double w,double h,COLORREF fill,COLORREF stroke){HBRUSH b=CreateSolidBrush(fill);HPEN p=CreatePen(PS_SOLID,1,stroke);auto ob=SelectObject(dc,b),op=SelectObject(dc,p);RoundRect(dc,int(x),int(y),int(x+w),int(y+h),8,8);SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(b);DeleteObject(p);}
  void text(std::wstring s,double x,double y,double w,double h,int size,COLORREF color,bool center=false,int minimumSize=14){HFONT f=CreateFontW(-std::max(minimumSize,size),0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Segoe UI");auto old=SelectObject(dc,f);SetTextColor(dc,color);SetBkMode(dc,TRANSPARENT);int extra=std::max(0,minimumSize-size);RECT r{int(x),int(y)-extra/2,int(x+w),int(y+h)+extra/2};DrawTextW(dc,s.c_str(),-1,&r,DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS|(center?DT_CENTER:DT_LEFT));SelectObject(dc,old);DeleteObject(f);}
  void line(double x,double y,double xx,double yy,COLORREF color,int width=1){auto p=CreatePen(PS_SOLID,width,color);auto old=SelectObject(dc,p);MoveToEx(dc,int(x),int(y),nullptr);LineTo(dc,int(xx),int(yy));SelectObject(dc,old);DeleteObject(p);}
  void draw(){layout();box(0,0,1320,1360,dark(),dark());panel(8,8,1304,73);text(L"GRAINS",28,20,165,45,31,AZSKIN(kTitle));text(L"DOSAGE",196,20,194,45,31,AZSKIN(kTitle));box(420,22,462,30,dark(),green());text(L"<",420,22,24,30,13,cream(),true);text(L">",858,22,24,30,13,cream(),true);text(presetName,448,22,406,30,12,cream(),true);
    box(420,56,462,28,AZSKIN(kPanelRaised),AZSKIN(kFiligree));text(L"LOAD",420,56,227,28,10,cream(),true);text(L"SAVE",651,56,227,28,10,cream(),true);
    const wchar_t* names[]={L"GRANULIZER",L"PRESLICER",L"BEAT REPEATER"};int play=int(std::round(value(kUiStep)*15));
    for(int i=0;i<3;++i){double x=slotX[i];int st=stage(i);panel(x,98,416,404);if(dropSlot==i){line(x+10,100,x+406,100,green(),2);}text(std::wstring(L"↔  ")+names[st],x+15,105,210,29,15,cream());box(x+316,106,87,28,AZSKIN(kPanelRaised),AZSKIN(kFiligree));text(L"RANDOM",x+319,109,81,23,11,cream(),true);
      bool active=st==0?(value(kGrainEnabled)>.5&&value(kGrainMix)>.001):value(st==1?kUiGlitch:kUiRepeat)>.5;for(int j=0;j<12;++j)box(x+19+j*32,486,23,4,active?green():AZSKIN(kAccentTrack),dark());
      if(st==0){box(x+208,248,196,147,AZSKIN(kPanelInset),AZSKIN(kFiligreeDim));text(L"MASTER OPTIONS",x+220,252,174,17,10,AZSKIN(kMuted),true);
        if(value(kPanMode)>=.25)text(value(kPanMode)<.75?L"PAN: ALTERNATE L / R":L"PAN: RANDOM L / R",x+170,405,228,16,10,green(),true);
        box(x+14,432,388,48,dark(),AZSKIN(kHairline));bool active=value(kUiGrainActive)>.5;double left=value(kUiGrainStart),right=value(kUiGrainEnd);
        double wavePeak=.02;for(int b=0;b<128;++b)wavePeak=std::max(wavePeak,value(kUiWave0+b));
        for(int b=0;b<128;++b){double u=(b+.5)/128.,h=std::max(1.,value(kUiWave0+b)/wavePeak*30.);box(x+18+b*3.,446+(30.-h)*.5,2,h,active&&(b+1.)/128.>=left&&b/128.<=right?green():AZSKIN(kFaint),dark());}
        if(active)line(x+18+value(kUiGrainHead)*384.,446,x+18+value(kUiGrainHead)*384.,476,cream());
        text(L"GRAIN FOLLOW",x+22,433,360,11,9,cream());
      }
      if(st==1){text(L"RANDOM TRIGGER",x+20,252,350,24,12,cream());
        text(L"CHANCE = probability at each interval",x+20,354,375,24,10,cream());
        text(L"SLICE = cut length / burst = up to 2 cuts",x+20,380,375,24,10,cream());}
      if(st==2){text(L"SELECT STEP · DOUBLE CLICK TO ENABLE",x+20,246,375,20,10,cream());
        for(int j=0;j<16;++j){
          double a=x+19+j%8*48,b=271+j/8*39;bool on=value(kRepeatStep0+j)>.5;
          const bool selected=j==selectedRepeat;const double press=selected?1.:0.;
          box(a,b+3,43,29,AZSKIN(kWell),AZSKIN(kBorderDark));
          box(a,b+press,43,28,on?AZSKIN(kAccentGlow):AZSKIN(kPanelRaised),selected?cream():(on?AZSKIN(kFiligreeDim):AZSKIN(kFiligreeDim)));
          line(a+4,b+2+press,a+39,b+2+press,on?AZSKIN(kFiligree):AZSKIN(kFiligreeDim));
          line(a+2,b+5+press,a+2,b+24+press,AZSKIN(kFiligreeDim));
          line(a+3,b+26+press,a+40,b+26+press,AZSKIN(kWell));
          text(std::to_wstring(j+1),a+3,b+2+press,37,13,11,on?ONTEXT():cream(),true,11);
          text(display(kRepeatRate0+j,value(kRepeatRate0+j)),a+3,b+16+press,37,10,9,on?ONTEXT():AZSKIN(kMuted),true,9);
          if(j==play)line(a+5,b+30,a+38,b+30,green(),2);
        }
      }
    }
    panel(16,516,836,236);text(L"MODULATION",32,526,129,28,13,cream());
    for(int i=0;i<4;++i){box(171+i*133,528,120,28,i==selectedLfo?AZSKIN(kPanelRaised):AZSKIN(kPanel),green());text(L"Mod "+std::to_wstring(i+1),171+i*133,529,120,25,12,cream(),true);}
    box(32,613,220,64,dark(),green());double cycle=std::round(value(kUiLfoCycle0+selectedLfo)*4294967295.-2147483648.);int64_t epoch=int64_t(std::round(value(kUiLfoEpoch0+selectedLfo)*4294967295.-2147483648.));qg::Lfo preview;preview.prepare(0x13579BDFULL+uint64_t(selectedLfo)*104729+(value(lfoID(selectedLfo,lReset))>=.5?uint64_t(epoch)*0x9e3779b97f4a7c15ULL:0));qg::LfoSettings shape;shape.enabled=true;shape.beats=1.;shape.wave=int(std::round(value(value(kModWaveRnd0+selectedLfo)>0.?kUiModWave0+selectedLfo:lfoID(selectedLfo,lWave))*129));shape.randomSteps=1+int(std::round(value(kRandomSteps0+selectedLfo)*63));shape.depth=value(lfoID(selectedLfo,lDepth));shape.phase=0.;shape.glide=.01+.99*value(lfoID(selectedLfo,lGlide));double prev=640;for(int j=0;j<=210;++j){double y=640-preview.process(shape,cycle+j/210.,48000.,false,0)*21;if(j)line(36+j,prev,37+j,y,green());prev=y;}
    double cursor=37.+210.*value(kUiLfoPhase0+selectedLfo);line(cursor,617,cursor,661,cream(),2);
    panel(864,516,440,236);text(L"XY MORPH",880,526,240,28,13,cream());box(880,566,168,166,dark(),green());if(spriteDC[5]){BITMAP nb{};GetObjectW(sprites[5],sizeof(nb),&nb);int om=SetStretchBltMode(dc,HALFTONE);StretchBlt(dc,880,566,168,166,spriteDC[5],0,0,nb.bmWidth,nb.bmHeight,SRCCOPY);SetStretchBltMode(dc,om);}double px=880+168*value(kXYX),py=732-166*value(kXYY);line(px,566,px,732,C(skin::scale(C2(skin::kAccent),.35)));line(880,py,1048,py,C(skin::scale(C2(skin::kAccent),.35)));for(int i=0;i<8;++i){double a=i*qg::tau/8.;for(int j=i+1;j<8;++j){double b=j*qg::tau/8.;line(964+72*std::cos(a),649+71*std::sin(a),964+72*std::cos(b),649+71*std::sin(b),AZSKIN(kViolet));}}box(px-5,py-5,10,10,green(),cream());
    panel(16,766,1288,108);text(L"RESLICE",32,781,108,24,14,AZSKIN(kMuted));box(1052,778,216,28,AZSKIN(kAccentGlow),green());text(L"RANDOM ONCE",1052,778,216,28,12,ONTEXT(),true);
    for(int i=0;i<16;++i){double x=32+i*78.;bool on=value(kResliceStep0+i)>.5;box(x,823,70,40,on?AZSKIN(kAccentGlow):dark(),selectedReslice==i?cream():AZSKIN(kMuted));text(std::to_wstring(i+1)+L" → "+std::to_wstring(1+int(std::round(value(value(kResliceRndOn)>.5?kUiResliceSource0+i:kResliceIndex0+i)*15))),x,827,70,18,10,on?ONTEXT():cream(),true);if(i==int(std::round(value(kUiResliceStep)*15)))line(x+5,858,x+65,858,value(kUiResliceActive)>.5?green():cream(),2);}
    panel(16,886,1288,108);text(L"GATER",32,901,104,24,14,AZSKIN(kMuted));text(L"CLICK: WET / OFF     SHIFT-CLICK: LATCH RELEASE",760,876,500,16,9,AZSKIN(kMuted),true);
    for(int i=0;i<16;++i){double x=32+i*78.;int state=value(kGaterState0+i)>=.25?1:0;if(value(kGaterEnabled)>.5&&value(kGaterStepRnd)>.5)state=value(kUiGaterState0+i)>=.5?1:0;bool release=value(kGaterRelease0+i)>=.5;double length=value(kGaterLengthRnd)>.5?value(kUiGaterLength0+i):.05+.95*value(kGaterLength0+i),sustain=value(kGaterSustain0+i);COLORREF color=release?AZSKIN(kRelease):state?green():AZSKIN(kMuted);box(x,943,70,40,dark(),selectedGate==i?cream():color);text(std::to_wstring(i+1)+(release?L" REL":state?L" WET":value(kGaterLatch)>.5?L" HOLD":L" OFF"),x+2,944,66,17,10,color,true);box(x+5,965,60,4,AZSKIN(kBorderDark),dark());box(x+5,965,60*length,4,color,color);box(x+5,973,60,4,AZSKIN(kBorderDark),dark());box(x+5,973,60*sustain,4,AZSKIN(kViolet),AZSKIN(kViolet));/* Tie badge removed */if(value(kGaterEnabled)>.5&&i==int(std::round(value(kUiGaterStep)*15)))line(x+5,981,x+65,981,cream(),2);}
    /* Tie wrap-around badge removed */
    panel(16,1006,540,136);text(L"FILTER",32,1017,100,22,13,cream());
    panel(568,1006,736,136);text(L"REVERB",584,1017,100,22,13,cream());if(value(kReverbSource)>=.5){box(592,1110,8,8,value(kUiReverbGate)>.5?green():AZSKIN(kFiligreeDim),dark());text(L"RANDOM IMPULSE · 50% CHANCE · TAIL CONTINUES",614,1102,650,28,11,cream());}
    panel(16,1154,1288,128);text(L"FILTER SEQUENCER",32,1173,180,22,13,cream());
    if(value(kFilterSeqMode)>.5)text(L"SAMPLE & GLIDE - smooth random cutoff",32,1230,704,24,14,green(),true);
    else for(int i=0;i<32;++i){double x=32+i*22.,v=qg::filterPattern(int(std::round(value(kFilterSeqPattern)*63.)),i);bool active=value(kFilterSeqOn)>.5&&i==int(std::round(value(kUiFilterSeqStep)*31.));box(x,1223,18,37,dark(),active?green():AZSKIN(kAccentTrack));box(x+3,1255-25*(v+1)*.5,12,3+25*(v+1)*.5,active?green():AZSKIN(kAccentTrack),dark());}
    panel(16,1294,1288,56);text(L"MASTER",32,1312,78,22,13,cream());text(L"OUTPUT",994,1304,106,16,10,cream());for(int j=0;j<24;++j)box(994+j*4,1326,2,12,value(kUiLevel)>j/24.?(j>20?AZSKIN(kHot):AZSKIN(kMeter)):AZSKIN(kMeterOff),dark());
    for(const auto& c:controls){double v=value(c.id);std::wstring title(c.label,c.label+std::strlen(c.label));
      if(c.kind==PanMode){const wchar_t* modes[]={L"MANUAL",L"ALTERNATE",L"RANDOM"};int mode=int(std::round(v*2));for(int i=0;i<3;++i){double x=c.x+i*c.w/3;box(x,c.y,c.w/3-3,c.h,mode==i?AZSKIN(kPanelRaised):dark(),mode==i?green():cream());text(modes[i],x,c.y,c.w/3-3,c.h,9,mode==i?ONTEXT():cream(),true);}}
      else if(c.kind==Pan){text(L"L",c.x,c.y,16,16,10,cream());text(L"C",c.x+c.w/2-8,c.y,16,16,10,cream(),true);text(L"R",c.x+c.w-16,c.y,16,16,10,cream());box(c.x+4,c.y+23,c.w-8,3,AZSKIN(kHairline),dark());box(c.x+v*(c.w-8),c.y+18,8,13,green(),green());}
      else if(c.kind==Knob){text(title,c.x,c.y,c.w,17,11,cream(),true);double cx=c.x+c.w/2,cy=c.y+43;box(cx-22,cy-22,44,44,AZSKIN(kViolet),AZSKIN(kFiligreeDim));for(int j=0;j<21;++j){double a=(135+j*13.5)*qg::tau/360.;box(cx+30*std::cos(a)-2,cy+30*std::sin(a)-2,4,4,j/20.<=v?green():AZSKIN(kAccentTrack),dark());}knobCap(cx,cy,v);double a=(135+270*v)*qg::tau/360.;line(cx+6*std::cos(a),cy+6*std::sin(a),cx+22*std::cos(a),cy+22*std::sin(a),cream(),3);box(c.x+4,c.y+74,c.w-8,19,dark(),AZSKIN(kHairline));text(display(c.id,v),c.x+5,c.y+75,c.w-10,18,12,AZSKIN(kReadout),true);}
      else if(c.kind==Slider){text(title,c.x,c.y,c.w,14,9,cream());box(c.x+4,c.y+20,c.w-8,4,AZSKIN(kHairline),dark());box(c.x+4,c.y+20,(c.w-8)*v,4,green(),green());art(453,636,24,32,c.x+v*(c.w-8)-2,c.y+13,12,16);if(c.h>=33)text(display(c.id,v),c.x,c.y+28,c.w,13,10,cream(),true);else text(display(c.id,v),c.x+100,c.y,c.w-100,13,9,cream(),true);}
      else if(c.kind==Select){box(c.x,c.y,c.w,c.h,dark(),AZSKIN(kBorderDark));if(c.h>=38)text(title,c.x+7,c.y+3,c.w-20,12,8,cream());text(display(c.id,c.id==lfoID(selectedLfo,lWave)&&value(kModWaveRnd0+selectedLfo)>0.?value(kUiModWave0+selectedLfo):v)+L" ▾",c.x+7,c.y+(c.h>=38?17:5),c.w-14,21,11,cream());}
      else{bool on=v>=.5;if(c.id>=kGrainEnabled&&c.id<=kRepeatEnabled)title=on?L"ON":L"OFF";box(c.x,c.y,c.w,c.h,on?AZSKIN(kAccentGlow):AZSKIN(kPanelRaised),on?green():AZSKIN(kFiligreeDim));if(c.kind==Pad){int step=int(c.id-(c.id>=kReverbStep0?kReverbStep0:kGlitchStep0));art(734,456,54,48,c.x+c.w/2-10,c.y+1,20,18);text(std::to_wstring(step+1),c.x,c.y+c.h-15,c.w,14,10,on?ONTEXT():cream(),true);if(step==(c.id>=kReverbStep0?int(std::round(value(kUiReverb)*15)):play))line(c.x+4,c.y+c.h-3,c.x+c.w-4,c.y+c.h-3,cream(),2);}else{text(title,c.x+18,c.y,c.w-21,c.h,10,on?ONTEXT():cream(),true);if(on)disc(c.x+10,c.y+c.h/2,6,AZSKIN(kAccent),AZSKIN(kAccent));disc(c.x+10,c.y+c.h/2,3,on?green():AZSKIN(kMuted),on?green():dark());}}
    }
    box(982,1312,144,26,dark(),AZSKIN(kFiligreeDim));{std::wstring label=L"SKIN: ";for(const char* c=aztec::theme::skinLabel().c_str();*c;++c)label+=wchar_t(*c);label+=L" ▾";text(label,982,1312,144,26,10,cream(),true);}
    box(1140,1312,144,26,dark(),green());text(L"UI SIZE ▾",1140,1312,144,26,11,cream(),true);
  }
  void end(){if(dragID>=0)controller->endEdit(ParamID(dragID));if(dragXY){controller->endEdit(kXYX);controller->endEdit(kXYY);}dragID=dragSlot=dropSlot=-1;dragXY=false;}
  void down(double x,double y,bool dbl){layout();SetFocus(window);
    if(inside(x,y,420,22,24,30)){stepPreset(-1);return;}// PRESET <
    if(inside(x,y,858,22,24,30)){stepPreset(1);return;}// PRESET >
    if(inside(x,y,444,22,414,30)){presetMenu();return;}
    if(inside(x,y,420,56,227,28)){preset(false);return;}// LOAD
    if(inside(x,y,651,56,227,28)){preset(true);return;}// SAVE
    if(inside(x,y,1140,1312,144,26)){HMENU m=CreatePopupMenu();int n=1;for(int size:{50,60,70,75,80,90,100})AppendMenuW(m,MF_STRING,n++,(std::to_wstring(size)+L"%").c_str());POINT p;GetCursorPos(&p);int pick=TrackPopupMenu(m,TPM_RETURNCMD,p.x,p.y,0,window,nullptr);DestroyMenu(m);if(pick&&plugFrame){int sizes[]={50,60,70,75,80,90,100};double f=sizes[pick-1]/100.;ViewRect r(0,0,int(1320*f),int(1360*f));plugFrame->resizeView(this,&r);}return;}
    if(inside(x,y,1052,778,216,28)){randomizeReslice(seed,[&](ParamID id){return value(id);},[&](ParamID id,double v){edit(id,v);});return;}
    for(int i=0;i<16;++i)if(inside(x,y,32+i*78,823,70,40)){selectedReslice=i;if(dbl)edit(kResliceStep0+i,value(kResliceStep0+i)>.5?0.:1.);InvalidateRect(window,nullptr,FALSE);return;}
    for(int i=0;i<16;++i)if(inside(x,y,32+i*78,943,70,40)){selectedGate=i;if(GetKeyState(VK_SHIFT)&0x8000){double next=value(kGaterRelease0+i)>.5?0.:1.;edit(kGaterRelease0+i,next);if(next>.5)edit(kGaterState0+i,0.);}else{edit(kGaterRelease0+i,0.);edit(kGaterState0+i,value(kGaterState0+i)>=.25?0.:1.);}InvalidateRect(window,nullptr,FALSE);return;}
    for(int i=0;i<3;++i){if(inside(x,y,slotX[i]+316,106,87,28)){randomizeModule(stage(i),selectedRepeat,seed,[&](ParamID id,double v){edit(id,v);});return;}if(inside(x,y,slotX[i]+1,99,226,42)){dragSlot=dropSlot=i;SetCapture(window);return;}}
    for(int i=0;i<4;++i)if(inside(x,y,171+i*133,528,120,28)){selectedLfo=i;InvalidateRect(window,nullptr,FALSE);return;}
    double rx=slotX[slot(2)];for(int i=0;i<16;++i)if(inside(x,y,rx+19+i%8*48,271+i/8*39,43,32)){selectedRepeat=i;if(dbl)edit(kRepeatStep0+i,value(kRepeatStep0+i)>.5?0:1);InvalidateRect(window,nullptr,FALSE);return;}
    if(inside(x,y,880,566,168,166)){dragXY=true;controller->beginEdit(kXYX);controller->beginEdit(kXYY);change(kXYX,(x-880)/168);change(kXYY,1-(y-566)/166);SetCapture(window);return;}
    for(const auto& c:controls)if(inside(x,y,c.x,c.y,c.w,c.h)){
      if(c.kind==PanMode){edit(c.id,std::clamp(int((x-c.x)/(c.w/3)),0,2)/2.);return;}
      if(c.kind==Toggle||c.kind==Pad){edit(c.id,value(c.id)>.5?0:1);return;}
      if(dbl){edit(c.id,info(c.id).defaultNormalizedValue);return;}
      if(c.kind==Select){HMENU m=CreatePopupMenu();int count=info(c.id).stepCount;for(int i=0;i<=count;++i)AppendMenuW(m,MF_STRING|(int(std::round(value(c.id)*count))==i?MF_CHECKED:0),i+1,display(c.id,i/double(std::max(1,count))).c_str());POINT p;GetCursorPos(&p);int chosen=TrackPopupMenu(m,TPM_RETURNCMD,p.x,p.y,0,window,nullptr);DestroyMenu(m);if(chosen)edit(c.id,(chosen-1)/double(std::max(1,count)));return;}
      dragID=int(c.id);dragKind=c.kind;dragWidth=c.w;originX=x;originY=y;dragValue=value(c.id);controller->beginEdit(c.id);SetCapture(window);return;
    }
  }
  static LRESULT CALLBACK proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){auto* e=reinterpret_cast<WinEditor*>(GetWindowLongPtrW(hwnd,GWLP_USERDATA));if(msg==WM_NCCREATE){e=static_cast<WinEditor*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(e));e->window=hwnd;}if(!e)return DefWindowProcW(hwnd,msg,wp,lp);
    double x=0,y=0;
    switch(msg){
      case WM_APP+71:return e->skinDC!=nullptr;
      case WM_ERASEBKGND:return 1;
      case WM_TIMER:InvalidateRect(hwnd,nullptr,FALSE);return 0;
      case WM_PAINT:{PAINTSTRUCT ps;HDC screen=BeginPaint(hwnd,&ps);RECT r;GetClientRect(hwnd,&r);HDC mem=CreateCompatibleDC(screen);auto bitmap=CreateCompatibleBitmap(screen,r.right*2,r.bottom*2);auto old=SelectObject(mem,bitmap);SetMapMode(mem,MM_ANISOTROPIC);SetWindowExtEx(mem,1320,1360,nullptr);SetViewportExtEx(mem,r.right*2,r.bottom*2,nullptr);e->dc=mem;e->draw();SetMapMode(mem,MM_TEXT);SetStretchBltMode(screen,HALFTONE);SetBrushOrgEx(screen,0,0,nullptr);StretchBlt(screen,0,0,r.right,r.bottom,mem,0,0,r.right*2,r.bottom*2,SRCCOPY);SelectObject(mem,old);DeleteObject(bitmap);DeleteDC(mem);EndPaint(hwnd,&ps);return 0;}
      case WM_LBUTTONDOWN:case WM_LBUTTONDBLCLK:e->point(lp,x,y);e->down(x,y,msg==WM_LBUTTONDBLCLK);return 0;
      case WM_MOUSEMOVE:e->point(lp,x,y);if(e->dragSlot>=0){e->dropSlot=-1;for(int i=0;i<3;++i)if(inside(x,y,slotX[i],98,416,404))e->dropSlot=i;InvalidateRect(hwnd,nullptr,FALSE);}else if(e->dragXY){e->change(kXYX,(x-880)/168);e->change(kXYY,1-(y-566)/166);}else if(e->dragID>=0){double delta=(e->dragKind==Slider||e->dragKind==Pan)?(x-e->originX)/e->dragWidth:(e->originY-y)/180.;if(GetKeyState(VK_SHIFT)&0x8000)delta*=.1;e->dragValue=std::clamp(e->dragValue+delta,0.,1.);e->originX=x;e->originY=y;e->change(ParamID(e->dragID),e->dragValue);}return 0;
      case WM_LBUTTONUP:if(e->dragSlot>=0&&e->dropSlot>=0&&e->dragSlot!=e->dropSlot){std::array<int,3> o={e->stage(0),e->stage(1),e->stage(2)};std::swap(o[e->dragSlot],o[e->dropSlot]);for(int i=0;i<6;++i)if(o==moduleOrders[i])e->edit(kModuleOrder,i/6.);}e->end();ReleaseCapture();InvalidateRect(hwnd,nullptr,FALSE);return 0;
      case WM_CAPTURECHANGED:e->end();return 0;
      case WM_MOUSEWHEEL:{POINT p{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};ScreenToClient(hwnd,&p);e->point(MAKELPARAM(p.x,p.y),x,y);e->layout();for(const auto& c:e->controls)if(inside(x,y,c.x,c.y,c.w,c.h)&&(c.kind==Knob||c.kind==Slider)){int n=e->info(c.id).stepCount;double d=n?1./n:.01;if(GetKeyState(VK_SHIFT)&0x8000)d*=.1;e->edit(c.id,e->value(c.id)+(GET_WHEEL_DELTA_WPARAM(wp)>0?d:-d));break;}return 0;}
    }return DefWindowProcW(hwnd,msg,wp,lp);
  }
public:
  explicit WinEditor(EditController* c):controller(c){controller->addRef();rect=ViewRect(0,0,792,816);seed=uint32_t(GetTickCount64())^uint32_t(reinterpret_cast<uintptr_t>(this));seed|=1;loadSkin();}
  ~WinEditor()override{removed();if(skinDC){SelectObject(skinDC,oldSkin);DeleteDC(skinDC);}if(skin)DeleteObject(skin);for(size_t i=0;i<sprites.size();++i){if(spriteDC[i]){SelectObject(spriteDC[i],oldSprite[i]);DeleteDC(spriteDC[i]);}if(sprites[i])DeleteObject(sprites[i]);}if(imaging)Gdiplus::GdiplusShutdown(imaging);controller->release();}
  tresult PLUGIN_API isPlatformTypeSupported(FIDString type)override{return type&&std::strcmp(type,kPlatformTypeHWND)==0?kResultTrue:kResultFalse;}
  tresult PLUGIN_API attached(void* parent,FIDString type)override{if(!parent||window||isPlatformTypeSupported(type)!=kResultTrue)return kResultFalse;HINSTANCE instance=nullptr;GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&proc),&instance);WNDCLASSW wc{};wc.style=CS_DBLCLKS;wc.lpfnWndProc=proc;wc.hInstance=instance;wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.lpszClassName=L"GrainsDosage070";RegisterClassW(&wc);window=CreateWindowExW(0,wc.lpszClassName,L"GrainsDosage",WS_CHILD|WS_VISIBLE,0,0,rect.getWidth(),rect.getHeight(),static_cast<HWND>(parent),nullptr,instance,this);if(!window)return kResultFalse;SetTimer(window,1,33,nullptr);return CPluginView::attached(parent,type);}
  tresult PLUGIN_API removed()override{end();if(window){KillTimer(window,1);DestroyWindow(window);window=nullptr;}return CPluginView::removed();}
  tresult PLUGIN_API onSize(ViewRect* r)override{if(!r)return kInvalidArgument;auto result=CPluginView::onSize(r);if(window)SetWindowPos(window,nullptr,0,0,r->getWidth(),r->getHeight(),SWP_NOZORDER|SWP_NOMOVE);return result;}
  tresult PLUGIN_API canResize()override{return kResultTrue;}
  tresult PLUGIN_API checkSizeConstraint(ViewRect* r)override{if(!r)return kInvalidArgument;double f=std::clamp(r->getWidth()/1320.,.5,1.);r->right=r->left+int(std::round(1320*f));r->bottom=r->top+int(std::round(1360*f));return kResultTrue;}
};
IPlugView* createEditor(EditController* controller){return new WinEditor(controller);}
}
