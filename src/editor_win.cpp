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
#include <shlwapi.h>
#include <filesystem>
#include "editor.h"
#include "parameters.h"
#include "skin_spec.h"
#include "skin_theme.h"
#include "mockup_ui.h"
#include <memory>
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
enum Kind{Knob,Slider,Toggle,Select,Pad,Pan,PanMode,VSlider};
struct Control{ParamID id;double x,y,w,h;Kind kind;std::string label;};
class WinEditor final:public CPluginView{
  EditController* controller;HWND window=nullptr;std::vector<Control> controls;
  mockup::RackState rackState;bool dragScroll=false;double scrollOrigin=0,routePointerX=0,routePointerY=0;
  int selectedGate=0,selectedReslice=0;
  int selectedLfo=0,selectedRepeat=0,dragID=-1,dragSlot=-1,dropSlot=-1;
  mockup::WaveVisual waveVisual;
  std::array<double,kCount> lastDisplayValues{};
  int animationFrames=0;
  aztec::mockup::Motion motion;
  std::array<int,5> routingAtDrag;
  double originX=0,originY=0,dragValue=0,dragWidth=1,dragHeight=180;Kind dragKind=Knob;bool dragXY=false,routeMoved=false;
  uint32_t seed=0;HDC dc=nullptr;
  std::unique_ptr<Gdiplus::Bitmap> mockupKnob, rackBackplate, rackAtlas;
  ULONG_PTR imaging=0;HBITMAP skin=nullptr;HDC skinDC=nullptr;HGDIOBJ oldSkin=nullptr;std::array<HBITMAP,6> sprites{};std::array<HDC,6> spriteDC{};std::array<HGDIOBJ,6> oldSprite{};
  void loadSprite(int id,int index){
    // Runtime skin folder first: <plugin dir>\GrainsDosage-spriteNN.png lets
    // users reskin by swapping PNGs only, no rebuild. Bundled RC resource is
    // the factory fallback (and seeds the folder on first run).
    std::wstring file=skinPath(id);
    if(!file.empty()&&loadSpriteFile(file,index))return;
    HMODULE module=nullptr;GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&proc),&module);
    HRSRC resource=FindResourceW(module,MAKEINTRESOURCEW(id),MAKEINTRESOURCEW(10));if(!resource)return;DWORD bytes=SizeofResource(module,resource);auto loaded=LoadResource(module,resource);const void* data=LockResource(loaded);if(!data||!bytes)return;
    seedSkinFile(id,data,bytes);
    HGLOBAL memory=GlobalAlloc(GMEM_MOVEABLE,bytes);if(!memory)return;void* dest=GlobalLock(memory);if(!dest){GlobalFree(memory);return;}std::memcpy(dest,data,bytes);GlobalUnlock(memory);IStream* stream=nullptr;if(FAILED(CreateStreamOnHGlobal(memory,TRUE,&stream))){GlobalFree(memory);return;}
    {Gdiplus::Bitmap bitmap(stream);if(bitmap.GetLastStatus()==Gdiplus::Ok)bitmap.GetHBITMAP(Gdiplus::Color(255,12,9,20),&sprites[size_t(index)]);}stream->Release();if(sprites[size_t(index)]){spriteDC[size_t(index)]=CreateCompatibleDC(nullptr);if(spriteDC[size_t(index)])oldSprite[size_t(index)]=SelectObject(spriteDC[size_t(index)],sprites[size_t(index)]);}
  }
  // ---- Replaceable skin folder ------------------------------------------------
  // All graphics resolve from one folder next to the plugin binary:
  //   <dir of GrainsDosage.vst3>\GrainsDosage-skin\background.png
  //   ...\GrainsDosage-skin\knob.png slider.png step.png header-left.png
  //   header-right.png xy-nebula.png — same names as the macOS loader and the
  //   repo's assets/sprites/, so one skin works on both platforms.
  // Drop your own PNGs there to change the look. Missing files fall back to the
  // factory images baked into the binary, which are also seeded into the folder
  // on first run so there is always something to edit.
  static const wchar_t* skinName(int resourceId){
    switch(resourceId){
      case 201:return L"background";
      case 202:return L"knob";
      case 203:return L"slider";
      case 204:return L"step";
      case 205:return L"header-left";
      case 206:return L"header-right";
      default: return L"xy-nebula";}}
  static std::wstring skinDir(){
    wchar_t path[4096]{};DWORD n=GetModuleFileNameW(nullptr,path,4096);
    if(!n||n>=4096)return L"";
    std::filesystem::path p(path);p.remove_filename();
    return (p/"GrainsDosage-skin").wstring();}
  static std::wstring skinPath(int resourceId){
    std::wstring dir=skinDir();if(dir.empty())return L"";
    std::wstring f=dir+L"\\"+skinName(resourceId)+L".png";
    DWORD a=GetFileAttributesW(f.c_str());
    return(a!=INVALID_FILE_ATTRIBUTES&&!(a&FILE_ATTRIBUTE_DIRECTORY))?f:std::wstring();}
  static void seedSkinFile(int resourceId,const void* data,DWORD bytes){
    std::wstring dir=skinDir();if(dir.empty())return;
    std::error_code ec;std::filesystem::create_directories(dir,ec);
    std::wstring f=dir+L"\\"+skinName(resourceId)+L".png";
    if(GetFileAttributesW(f.c_str())!=INVALID_FILE_ATTRIBUTES)return;
    HANDLE h=CreateFileW(f.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(h==INVALID_HANDLE_VALUE)return;DWORD written=0;WriteFile(h,data,bytes,&written,nullptr);CloseHandle(h);}
  bool loadSpriteFile(const std::wstring& file,int index){
    Gdiplus::Bitmap* bitmap=Gdiplus::Bitmap::FromFile(file.c_str());if(!bitmap)return false;
    bool ok=bitmap->GetLastStatus()==Gdiplus::Ok;
    if(ok)bitmap->GetHBITMAP(Gdiplus::Color(255,12,9,20),&sprites[size_t(index)]);
    delete bitmap;
    if(ok&&sprites[size_t(index)]){spriteDC[size_t(index)]=CreateCompatibleDC(nullptr);if(spriteDC[size_t(index)])oldSprite[size_t(index)]=SelectObject(spriteDC[size_t(index)],sprites[size_t(index)]);}
    return ok&&sprites[size_t(index)];}
  void loadSkin(){
    Gdiplus::GdiplusStartupInput startup;
    if(Gdiplus::GdiplusStartup(&imaging,&startup,nullptr)!=Gdiplus::Ok){imaging=0;return;}
    HMODULE module=nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&proc),&module);
    { HRSRC res=FindResourceW(module,MAKEINTRESOURCEW(208),MAKEINTRESOURCEW(10));
      if(res){DWORD n=SizeofResource(module,res);auto loaded=LoadResource(module,res);const void* bytes=LockResource(loaded);
        HGLOBAL mem=GlobalAlloc(GMEM_MOVEABLE,n);if(mem){void* ptr=GlobalLock(mem);if(ptr){std::memcpy(ptr,bytes,n);GlobalUnlock(mem);IStream* stream=nullptr;
          if(SUCCEEDED(CreateStreamOnHGlobal(mem,TRUE,&stream))){Gdiplus::Bitmap image(stream);if(image.GetLastStatus()==Gdiplus::Ok)mockupKnob.reset(image.Clone(Gdiplus::Rect(0,0,int(image.GetWidth()),int(image.GetHeight())),PixelFormat32bppPARGB));stream->Release();}else GlobalFree(mem);
        }else GlobalFree(mem);}}
      if(!mockupKnob)mockupKnob.reset(Gdiplus::Bitmap::FromFile(L"assets/mockup/knob.png"));
    }
    auto loadRack=[&](int id,const wchar_t* fallback,std::unique_ptr<Gdiplus::Bitmap>& target){
      HRSRC res=FindResourceW(module,MAKEINTRESOURCEW(id),MAKEINTRESOURCEW(10));
      if(res){DWORD n=SizeofResource(module,res);const void* bytes=LockResource(LoadResource(module,res));
        HGLOBAL mem=GlobalAlloc(GMEM_MOVEABLE,n);if(mem){void* ptr=GlobalLock(mem);if(ptr){std::memcpy(ptr,bytes,n);GlobalUnlock(mem);IStream* stream=nullptr;
          if(SUCCEEDED(CreateStreamOnHGlobal(mem,TRUE,&stream))){Gdiplus::Bitmap image(stream);if(image.GetLastStatus()==Gdiplus::Ok)target.reset(image.Clone(Gdiplus::Rect(0,0,int(image.GetWidth()),int(image.GetHeight())),PixelFormat32bppPARGB));stream->Release();}else GlobalFree(mem);
        }else GlobalFree(mem);}}
      if(!target)target.reset(Gdiplus::Bitmap::FromFile(fallback));
    };
    loadRack(209,L"assets/approved-rack/backplate.png",rackBackplate);
    loadRack(210,L"assets/approved-rack/controls.png",rackAtlas);
    // Skin-folder background.png wins over the bundled artwork.
    if(std::wstring bg=skinPath(201);!bg.empty()){
      Gdiplus::Bitmap* bitmap=Gdiplus::Bitmap::FromFile(bg.c_str());
      if(bitmap){if(bitmap->GetLastStatus()==Gdiplus::Ok)bitmap->GetHBITMAP(Gdiplus::Color(255,12,9,20),&skin);delete bitmap;}
      if(skin){skinDC=CreateCompatibleDC(nullptr);if(skinDC)oldSkin=SelectObject(skinDC,skin);}}
    if(skinDC){for(int i=0;i<6;++i)loadSprite(202+i,i);return;}
    HRSRC resource=FindResourceW(module,MAKEINTRESOURCEW(201),MAKEINTRESOURCEW(10));
    if(!resource)return;DWORD bytes=SizeofResource(module,resource);auto loaded=LoadResource(module,resource);const void* data=LockResource(loaded);if(!data||!bytes)return;
    seedSkinFile(201,data,bytes);
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
    std::sort(names.begin(),names.end(),presetFileLess);HMENU menu=CreatePopupMenu();
    for(int category=0;category<factoryCategoryCount+2;++category){HMENU group=CreatePopupMenu();int count=0;
      for(size_t i=0;i<names.size();++i){auto label=names[i].substr(0,names[i].find_last_of(L'.'));if(presetCategory(label)!=category)continue;
        AppendMenuW(group,MF_STRING|(label==presetName?MF_CHECKED:0),UINT_PTR(i+1),label.c_str());++count;}
      if(count){std::string title=factoryCategories[category];std::wstring wide;for(char ch:title){wide+=wchar_t(ch);if(ch=='&')wide+=L'&';}AppendMenuW(menu,MF_POPUP,UINT_PTR(group),wide.c_str());}else DestroyMenu(group);
    }
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
  int tab()const{return aztec::tabFromValue(value(kUiTab));}
  void chooseTab(int t){t=std::clamp(t,0,tabCount-1);controller->setParamNormalized(kUiTab,tabToValue(t));InvalidateRect(window,nullptr,FALSE);}
  void layout(){controls.clear();for(const auto& c:mockup::controls([&](ParamID id){return value(id);},tab(),selectedLfo,selectedRepeat,selectedGate,selectedReslice,rackState))controls.push_back({c.id,c.r.x,c.r.y,c.r.w,c.r.h,Kind(c.kind),c.label});}
  static bool inside(double x,double y,double a,double b,double w,double h){return x>=a&&x<a+w&&y>=b&&y<b+h;}
  void point(LPARAM lp,double& x,double& y){RECT r;GetClientRect(window,&r);mockup::Viewport viewport(r.right,r.bottom);auto logical=viewport.logical(GET_X_LPARAM(lp),GET_Y_LPARAM(lp));x=logical.first;y=logical.second;}
  static COLORREF green(){return AZSKIN(kAccent);}static COLORREF cream(){return AZSKIN(kCream);}static COLORREF dark(){return AZSKIN(kBgDeep);}
  void box(double x,double y,double w,double h,COLORREF fill,COLORREF stroke){HBRUSH b=CreateSolidBrush(fill);HPEN p=CreatePen(PS_SOLID,1,stroke);auto ob=SelectObject(dc,b),op=SelectObject(dc,p);RoundRect(dc,int(x),int(y),int(x+w),int(y+h),8,8);SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(b);DeleteObject(p);}
  void text(std::wstring s,double x,double y,double w,double h,int size,COLORREF color,bool center=false,int minimumSize=14){HFONT f=CreateFontW(-std::max(minimumSize,size),0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Arial");auto old=SelectObject(dc,f);SetTextColor(dc,color);SetBkMode(dc,TRANSPARENT);int extra=std::max(0,minimumSize-size);RECT r{int(x),int(y)-extra/2,int(x+w),int(y+h)+extra/2};DrawTextW(dc,s.c_str(),-1,&r,DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS|(center?DT_CENTER:DT_LEFT));SelectObject(dc,old);DeleteObject(f);}
  void line(double x,double y,double xx,double yy,COLORREF color,int width=1){auto p=CreatePen(PS_SOLID,width,color);auto old=SelectObject(dc,p);MoveToEx(dc,int(x),int(y),nullptr);LineTo(dc,int(xx),int(yy));SelectObject(dc,old);DeleteObject(p);}
  static std::wstring wide(const std::string& s){int n=MultiByteToWideChar(CP_UTF8,0,s.data(),int(s.size()),nullptr,0);std::wstring out(size_t(n),L' ');if(n)MultiByteToWideChar(CP_UTF8,0,s.data(),int(s.size()),out.data(),n);return out;}
  static std::string utf8(const std::wstring& s){int n=WideCharToMultiByte(CP_UTF8,0,s.data(),int(s.size()),nullptr,0,nullptr,nullptr);std::string out(size_t(n),' ');if(n)WideCharToMultiByte(CP_UTF8,0,s.data(),int(s.size()),out.data(),n,nullptr,nullptr);return out;}
  void draw(){
    layout();mockup::Painter painter;
    painter.image=[&](int asset,mockup::Rect r,mockup::Rect source){
      auto* image=asset==mockup::Backplate?rackBackplate.get():rackAtlas.get();if(!image||image->GetLastStatus()!=Gdiplus::Ok)return;
      Gdiplus::Graphics g(dc);SIZE viewport{},logical{};GetViewportExtEx(dc,&viewport);GetWindowExtEx(dc,&logical);
      POINT offset{};GetViewportOrgEx(dc,&offset);g.TranslateTransform(float(offset.x),float(offset.y));g.ScaleTransform(float(viewport.cx)/logical.cx,float(viewport.cy)/logical.cy);g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
      g.DrawImage(image,Gdiplus::RectF(float(r.x),float(r.y),float(r.w),float(r.h)),float(source.x),float(source.y),float(source.w),float(source.h),Gdiplus::UnitPixel);
    };
    painter.box=[&](mockup::Rect r,skin::Rgb fill,skin::Rgb edge,double radius){
      // GDI+ gives rounded plates and transparent image edges at every zoom.
      Gdiplus::Graphics g(dc);SIZE viewport{},logical{};GetViewportExtEx(dc,&viewport);GetWindowExtEx(dc,&logical);
      POINT offset{};GetViewportOrgEx(dc,&offset);g.TranslateTransform(float(offset.x),float(offset.y));g.ScaleTransform(float(viewport.cx)/logical.cx,float(viewport.cy)/logical.cy);g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
      Gdiplus::GraphicsPath path;float d=float(std::min({radius*2,r.w,r.h}));float x=float(r.x),y=float(r.y),w=float(r.w),h=float(r.h);
      if(d<1)path.AddRectangle(Gdiplus::RectF(x,y,w,h));else{path.AddArc(x,y,d,d,180,90);path.AddArc(x+w-d,y,d,d,270,90);path.AddArc(x+w-d,y+h-d,d,d,0,90);path.AddArc(x,y+h-d,d,d,90,90);path.CloseFigure();}
      auto top=skin::mix(fill,skin::C(skin::kFiligree),r.h>=20?.10:0.);
      Gdiplus::LinearGradientBrush brush(Gdiplus::PointF(x,y),Gdiplus::PointF(x,y+h),Gdiplus::Color(255,top.r,top.g,top.b),Gdiplus::Color(255,fill.r,fill.g,fill.b));
      Gdiplus::Pen pen(Gdiplus::Color(255,edge.r,edge.g,edge.b),1.3f);g.FillPath(&brush,&path);g.DrawPath(&pen,&path);
    };
    painter.text=[&](const std::string& s,mockup::Rect r,double size,skin::Rgb col,bool center){auto str=wide(s);HFONT f=CreateFontW(-int(size),0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Arial");auto old=SelectObject(dc,f);SIZE extent{};GetTextExtentPoint32W(dc,str.c_str(),int(str.size()),&extent);SelectObject(dc,old);DeleteObject(f);if(extent.cx>r.w)size*=r.w/extent.cx;text(str,r.x,r.y,r.w,r.h,int(size),C(col),center,0);};
    painter.line=[&](double x,double y,double xx,double yy,skin::Rgb col,double width){line(x,y,xx,yy,C(col),int(std::max(1.,width)));};
    painter.polygon=[&](const std::vector<std::pair<double,double>>& pts,skin::Rgb col){if(pts.size()<3)return;Gdiplus::Graphics g(dc);SIZE viewport{},logical{};GetViewportExtEx(dc,&viewport);GetWindowExtEx(dc,&logical);POINT offset{};GetViewportOrgEx(dc,&offset);g.TranslateTransform(float(offset.x),float(offset.y));g.ScaleTransform(float(viewport.cx)/logical.cx,float(viewport.cy)/logical.cy);g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);std::vector<Gdiplus::PointF> points;for(auto pt:pts)points.emplace_back(float(pt.first),float(pt.second));Gdiplus::SolidBrush brush(Gdiplus::Color(255,col.r,col.g,col.b));g.FillPolygon(&brush,points.data(),int(points.size()));};
    painter.knob=[&](mockup::Rect r,double value){
      if(!mockupKnob||mockupKnob->GetLastStatus()!=Gdiplus::Ok){disc(r.x+r.w/2,r.y+r.h/2,r.w/2,RGB(180,180,180),cream());return;}
      Gdiplus::Graphics g(dc);SIZE viewport{},logical{};GetViewportExtEx(dc,&viewport);GetWindowExtEx(dc,&logical);
      POINT offset{};GetViewportOrgEx(dc,&offset);g.TranslateTransform(float(offset.x),float(offset.y));g.ScaleTransform(float(viewport.cx)/logical.cx,float(viewport.cy)/logical.cy);
      g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);g.TranslateTransform(float(r.x+r.w/2),float(r.y+r.h/2));g.RotateTransform(float(270*value-135));
      g.DrawImage(mockupKnob.get(),Gdiplus::RectF(float(-r.w/2),float(-r.h/2),float(r.w),float(r.h)));
    };
    painter.clip=[&](mockup::Rect r){SaveDC(dc);IntersectClipRect(dc,int(r.x),int(r.y),int(r.x+r.w),int(r.y+r.h));};painter.unclip=[&](){RestoreDC(dc,-1);};
    mockup::render(painter,[&](ParamID id){return value(id);},[&](ParamID id,double v){return utf8(display(id,v));},utf8(presetName),tab(),selectedLfo,selectedRepeat,selectedGate,selectedReslice,&waveVisual,dragSlot,dropSlot,&motion,mockup::RenderPass::All,rackState);
  }
  void end(){if(dragID>=0)controller->endEdit(ParamID(dragID));if(dragXY){controller->endEdit(kXYX);controller->endEdit(kXYY);}dragID=dragSlot=dropSlot=-1;dragXY=routeMoved=dragScroll=false;animationFrames=12;}
  void down(double x,double y,bool dbl){layout();SetFocus(window);
    auto hit=[&](mockup::Rect r){return r.contains(x,y);};
    if(hit(mockup::previous)){stepPreset(-1);return;}
    if(hit(mockup::next)){stepPreset(1);return;}
    if(hit(mockup::preset)){presetMenu();return;}
    if(hit(mockup::load)){preset(false);return;}
    if(hit(mockup::save)){preset(true);return;}
    if(rackState.page==1&&y>=mockup::bodyTop&&hit(rackState.position(mockup::skinMenu))){skinMenu();return;}
    if(rackState.page==1&&y>=mockup::bodyTop&&hit(rackState.position(mockup::zoomMenu))){HMENU m=CreatePopupMenu();int n=1;for(int size:{50,60,70,75,80,90,100})AppendMenuW(m,MF_STRING,n++,(std::to_wstring(size)+L"%").c_str());POINT p;GetCursorPos(&p);int pick=TrackPopupMenu(m,TPM_RETURNCMD,p.x,p.y,0,window,nullptr);DestroyMenu(m);if(pick&&plugFrame){int sizes[]={50,60,70,75,80,90,100};double f=sizes[pick-1]/100.;ViewRect r(0,0,int(mockup::width*f),int(mockup::height*f));plugFrame->resizeView(this,&r);}return;}
    auto val=[&](ParamID id){return value(id);};int page=mockup::hitPage(x,y);if(page>=0){rackState.page=page;motion.ready=false;InvalidateRect(window,nullptr,FALSE);return;}
    if(mockup::maxScroll(rackState)>0&&hit(mockup::scrollbar())){dragScroll=true;originY=y;scrollOrigin=rackState.offset();SetCapture(window);return;}
    int route=mockup::hitRoute(x,y,val,rackState);if(route>=0){dragSlot=route;dropSlot=-1;routeMoved=false;originX=x;originY=y;routingAtDrag=mockup::routeChain(val);chooseTab(routingAtDrag[route]);SetCapture(window);return;}
    int lfo=mockup::hitLfo(x,y,rackState);if(lfo>=0){selectedLfo=lfo;InvalidateRect(window,nullptr,FALSE);return;}
    if(rackState.page==0&&y>=mockup::bodyTop)for(int t=0;t<5;++t)if(hit(mockup::randomRect(t,val,rackState))){if(t<3)randomizeModule(t,selectedRepeat,seed,val,[&](ParamID id,double v){edit(id,v);});else if(t==3)randomizeReslice(seed,val,[&](ParamID id,double v){edit(id,v);});else for(int i=0;i<16;++i){seed=seed*1664525u+1013904223u;edit(kGaterState0+i,(seed>>31)?1.:0.);}return;}
    int st=-1,step=mockup::hitStep(x,y,val,rackState,st);if(step>=0){if(st==2){selectedRepeat=step;if(dbl)edit(kRepeatStep0+step,value(kRepeatStep0+step)>.5?0.:1.);}if(st==3){selectedReslice=step;if(dbl)edit(kResliceStep0+step,value(kResliceStep0+step)>.5?0.:1.);}if(st==4){selectedGate=step;if(GetKeyState(VK_SHIFT)&0x8000){double v=value(kGaterRelease0+step)>.5?0.:1.;edit(kGaterRelease0+step,v);if(v>.5)edit(kGaterState0+step,0.);}else{edit(kGaterRelease0+step,0.);edit(kGaterState0+step,value(kGaterState0+step)>.25?0.:1.);}}InvalidateRect(window,nullptr,FALSE);return;}
    auto xy=rackState.position(mockup::xy);if(rackState.page==2&&y>=mockup::bodyTop&&hit(xy)){dragXY=true;controller->beginEdit(kXYX);controller->beginEdit(kXYY);auto v=mockup::xyValue(xy,x,y);change(kXYX,v.first);change(kXYY,v.second);SetCapture(window);return;}
    for(const auto& c:controls)if((y>=mockup::bodyTop||c.id==kInputDeclick||c.id==kDeclickSensitivity)&&inside(x,y,c.x,c.y,c.w,c.h)){
      if(c.kind==PanMode){edit(c.id,std::clamp(int((x-c.x)/(c.w/3)),0,2)/2.);return;}
      if(c.kind==Toggle||c.kind==Pad){edit(c.id,value(c.id)>.5?0:1);return;}
      if(dbl){edit(c.id,info(c.id).defaultNormalizedValue);return;}
      if(c.kind==Select){HMENU m=CreatePopupMenu();int count=info(c.id).stepCount;for(int i=0;i<=count;++i)AppendMenuW(m,MF_STRING|(int(std::round(value(c.id)*count))==i?MF_CHECKED:0),i+1,display(c.id,i/double(std::max(1,count))).c_str());POINT p;GetCursorPos(&p);int chosen=TrackPopupMenu(m,TPM_RETURNCMD,p.x,p.y,0,window,nullptr);DestroyMenu(m);if(chosen)edit(c.id,(chosen-1)/double(std::max(1,count)));return;}
      dragID=int(c.id);dragKind=c.kind;dragWidth=c.w;dragHeight=c.h;originX=x;originY=y;dragValue=value(c.id);controller->beginEdit(c.id);SetCapture(window);return;
    }
  }
  static LRESULT CALLBACK proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){auto* e=reinterpret_cast<WinEditor*>(GetWindowLongPtrW(hwnd,GWLP_USERDATA));if(msg==WM_NCCREATE){e=static_cast<WinEditor*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(e));e->window=hwnd;}if(!e)return DefWindowProcW(hwnd,msg,wp,lp);
    double x=0,y=0;
    switch(msg){
      case WM_APP+72:return e->rackState.page;
      case WM_APP+73:e->rackState.scroll[e->rackState.page]=std::clamp(double(lp),0.,mockup::maxScroll(e->rackState));InvalidateRect(hwnd,nullptr,FALSE);return 0;
      case WM_APP+71:return e->rackBackplate&&e->rackAtlas&&e->rackBackplate->GetLastStatus()==Gdiplus::Ok&&e->rackAtlas->GetLastStatus()==Gdiplus::Ok;
      case WM_ERASEBKGND:return 1;
      case WM_TIMER:{
        if(!IsWindowVisible(hwnd)||IsIconic(GetAncestor(hwnd,GA_ROOT)))return 0;
        bool changed=false;for(int id=0;id<kCount;++id){double v=e->value(id);if(e->lastDisplayValues[id]!=v){e->lastDisplayValues[id]=v;if(!isMonitor(id)||mockup::monitorVisible(id,[&](ParamID p){return e->value(p);},e->rackState,e->selectedLfo))changed=true;}}
        if(e->dragSlot>=0&&e->routeMoved){double d=e->routePointerY<mockup::bodyTop+45?-18:e->routePointerY>mockup::height-45?18:0;if(d){mockup::scrollBy(e->rackState,d);e->dropSlot=mockup::routeInsertion(e->routePointerX,e->routePointerY,[&](ParamID id){return e->value(id);},e->rackState);}}if(changed)e->animationFrames=12;
        if(e->animationFrames>0||e->dragSlot>=0){--e->animationFrames;if(e->rackState.page==0&&mockup::visible(mockup::waveRect([&](ParamID id){return e->value(id);},e->rackState)))e->waveVisual.update([&](ParamID id){return e->lastDisplayValues[id];});InvalidateRect(hwnd,nullptr,FALSE);}return 0;}
      case WM_PAINT:{PAINTSTRUCT ps;HDC screen=BeginPaint(hwnd,&ps);RECT r;GetClientRect(hwnd,&r);HDC mem=CreateCompatibleDC(screen);auto bitmap=CreateCompatibleBitmap(screen,r.right*2,r.bottom*2);auto old=SelectObject(mem,bitmap);PatBlt(mem,0,0,r.right*2,r.bottom*2,BLACKNESS);mockup::Viewport fit(r.right,r.bottom);SetMapMode(mem,MM_ANISOTROPIC);SetWindowExtEx(mem,int(mockup::width),int(mockup::height),nullptr);SetViewportExtEx(mem,int(std::round(mockup::width*fit.scale*2)),int(std::round(mockup::height*fit.scale*2)),nullptr);SetViewportOrgEx(mem,int(std::round(fit.x*2)),int(std::round(fit.y*2)),nullptr);e->dc=mem;e->draw();SetMapMode(mem,MM_TEXT);SetViewportOrgEx(mem,0,0,nullptr);SetStretchBltMode(screen,HALFTONE);SetBrushOrgEx(screen,0,0,nullptr);StretchBlt(screen,0,0,r.right,r.bottom,mem,0,0,r.right*2,r.bottom*2,SRCCOPY);SelectObject(mem,old);DeleteObject(bitmap);DeleteDC(mem);EndPaint(hwnd,&ps);return 0;}
      case WM_LBUTTONDOWN:case WM_LBUTTONDBLCLK:e->point(lp,x,y);e->down(x,y,msg==WM_LBUTTONDBLCLK);return 0;
      case WM_MOUSEMOVE:e->point(lp,x,y);if(e->dragScroll){auto b=mockup::scrollbar(),t=mockup::scrollThumb(e->rackState);e->rackState.scroll[e->rackState.page]=std::clamp(e->scrollOrigin+(y-e->originY)*mockup::maxScroll(e->rackState)/std::max(1.,b.h-t.h),0.,mockup::maxScroll(e->rackState));InvalidateRect(hwnd,nullptr,FALSE);}else if(e->dragSlot>=0){e->routePointerX=x;e->routePointerY=y;if(std::hypot(x-e->originX,y-e->originY)>8)e->routeMoved=true;if(e->routeMoved)e->dropSlot=mockup::routeInsertion(x,y,[&](ParamID id){return e->value(id);},e->rackState);InvalidateRect(hwnd,nullptr,FALSE);}else if(e->dragXY){auto xy=e->rackState.position(mockup::xy);auto v=mockup::xyValue(xy,x,y);e->change(kXYX,v.first);e->change(kXYY,v.second);}else if(e->dragID>=0){double delta=(e->dragKind==Slider||e->dragKind==Pan)?(x-e->originX)/e->dragWidth:(e->originY-y)/(e->dragKind==VSlider?std::max(1.,e->dragHeight-42):180.);if(GetKeyState(VK_SHIFT)&0x8000)delta*=.1;e->dragValue=std::clamp(e->dragValue+delta,0.,1.);e->originX=x;e->originY=y;e->change(ParamID(e->dragID),e->dragValue);}return 0;
      case WM_LBUTTONUP:if(e->dragSlot>=0&&e->dropSlot>=0&&e->dropSlot!=e->dragSlot&&e->dropSlot!=e->dragSlot+1)e->edit(kRoutingOrder,mockup::moveRoute(e->routingAtDrag,e->dragSlot,e->dropSlot));e->end();ReleaseCapture();InvalidateRect(hwnd,nullptr,FALSE);return 0;
      case WM_CAPTURECHANGED:e->end();return 0;
      case WM_MOUSEWHEEL:{mockup::scrollBy(e->rackState,-GET_WHEEL_DELTA_WPARAM(wp)/120.*72);InvalidateRect(hwnd,nullptr,FALSE);return 0;}

    }return DefWindowProcW(hwnd,msg,wp,lp);
  }
public:
  explicit WinEditor(EditController* c):controller(c){controller->addRef();rect=ViewRect(0,0,int(mockup::width*.70),int(mockup::height*.70));seed=uint32_t(GetTickCount64())^uint32_t(reinterpret_cast<uintptr_t>(this));seed|=1;loadSkin();}
  ~WinEditor()override{removed();mockupKnob.reset();rackBackplate.reset();rackAtlas.reset();if(skinDC){SelectObject(skinDC,oldSkin);DeleteDC(skinDC);}if(skin)DeleteObject(skin);for(size_t i=0;i<sprites.size();++i){if(spriteDC[i]){SelectObject(spriteDC[i],oldSprite[i]);DeleteDC(spriteDC[i]);}if(sprites[i])DeleteObject(sprites[i]);}if(imaging)Gdiplus::GdiplusShutdown(imaging);controller->release();}
  tresult PLUGIN_API isPlatformTypeSupported(FIDString type)override{return type&&std::strcmp(type,kPlatformTypeHWND)==0?kResultTrue:kResultFalse;}
  tresult PLUGIN_API attached(void* parent,FIDString type)override{if(!parent||window||isPlatformTypeSupported(type)!=kResultTrue)return kResultFalse;HINSTANCE instance=nullptr;GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&proc),&instance);WNDCLASSW wc{};wc.style=CS_DBLCLKS;wc.lpfnWndProc=proc;wc.hInstance=instance;wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.lpszClassName=L"GrainsDosage070";RegisterClassW(&wc);window=CreateWindowExW(0,wc.lpszClassName,L"GrainsDosage",WS_CHILD|WS_VISIBLE,0,0,rect.getWidth(),rect.getHeight(),static_cast<HWND>(parent),nullptr,instance,this);if(!window)return kResultFalse;SetTimer(window,1,33,nullptr);return CPluginView::attached(parent,type);}
  tresult PLUGIN_API removed()override{end();if(window){KillTimer(window,1);DestroyWindow(window);window=nullptr;}return CPluginView::removed();}
  tresult PLUGIN_API onSize(ViewRect* r)override{if(!r)return kInvalidArgument;auto result=CPluginView::onSize(r);if(window)SetWindowPos(window,nullptr,0,0,r->getWidth(),r->getHeight(),SWP_NOZORDER|SWP_NOMOVE);return result;}
  tresult PLUGIN_API canResize()override{return kResultTrue;}
  tresult PLUGIN_API checkSizeConstraint(ViewRect* r)override{if(!r)return kInvalidArgument;double f=std::clamp(r->getWidth()/mockup::width,.5,1.);r->right=r->left+int(std::round(mockup::width*f));r->bottom=r->top+int(std::round(mockup::height*f));return kResultTrue;}
};
IPlugView* createEditor(EditController* controller){return new WinEditor(controller);}
}

