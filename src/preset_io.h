#pragma once
#include "factory_presets.h"
#include <sstream>
#include <iomanip>
#include <locale>
#include <cmath>
#include <string>
#include <algorithm>
#include <vector>
#include <cwctype>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dirent.h>
#endif
namespace aztec {
inline bool presetParameter(int id){return id>=0&&id<presetCount&&!isMonitor(id);}
template<class Getter> std::string encodePreset(Getter get){
 std::ostringstream out;out.imbue(std::locale::classic());out<<"GrainsDosagePreset 1 "<<presetCount<<'\n'<<std::setprecision(17);
 for(int i=0;i<presetCount;++i)out<<i<<' '<<(presetParameter(i)?get(i):0.)<<'\n';return out.str();
}
// Parse and validate completely before touching any live parameter.
// Any count between 1 and kCount is accepted: presets written by older or newer
// builds (including the current kCount, which grew when the PRESET < / > buttons
// were added) load with their saved values; missing trailing parameters keep
// their documented defaults.
inline bool decodePreset(const std::string& text,std::array<double,kCount>& values){
 if(text.size()>65536)return false;std::istringstream in(text);in.imbue(std::locale::classic());
 std::string magic;int version=0,count=0;if(!(in>>magic>>version>>count)||magic!="GrainsDosagePreset"||version!=1||count<1||count>kCount)return false;
 auto temp=initialParameters();for(int i=0;i<count;++i){int id;double v;if(!(in>>id>>v)||id!=i||!std::isfinite(v)||v<0.||v>1.)return false;temp[i]=v;}
 if(count<=kResliceRndOn&&count>kResliceLength)temp[kResliceLength]=temp[kResliceLength]<1./6.?2./3.:1.;if(count<=kInputDeclick)temp[kInputDeclick]=0.;in>>std::ws;if(!in.eof())return false;if(count<kLfoSlots0+1)migrateRoutes(temp);else if(count<=kResliceRndOn)migrateModSlots(temp);values=temp;return true;
}
// PRESET < / > buttons: cycle through every .gdspreset file in the user's preset
// folder (same sorted order both editors show in their menus). direction is -1 or +1.
// Directory scanning uses native APIs (Win32 FindFileW / POSIX dirent) instead of
// std::filesystem, which is unavailable when targeting macOS x86_64 with a
// pre-10.15 deployment target. Names are returned as std::wstring.
// Extension ".gdspreset" is 10 wide units on every platform (Windows: UTF-16,
// macOS/Linux: wchar_t==char32_t). Length checks use >=presetExtLen before a
// substr of presetExtLen; the old code used >11/size()-11 (an off-by-one that
// also dropped names whose stem is shorter than one character).
constexpr size_t presetExtLen=10;
inline std::vector<std::wstring> presetFilesIn(const std::wstring& utf16directory){
 auto lower=[](std::wstring s){for(auto& c:s)c=wchar_t(towlower(c));return s;};
 std::vector<std::wstring> names;
#ifdef _WIN32
 std::wstring pattern=utf16directory+L"\\*.gdspreset";WIN32_FIND_DATAW fd{};
 HANDLE h=FindFirstFileW(pattern.c_str(),&fd);
 if(h!=INVALID_HANDLE_VALUE){do{if(!(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))names.push_back(fd.cFileName);}while(FindNextFileW(h,&fd));FindClose(h);}
#else
 auto fromUtf8=[](const std::string& s){std::wstring w;// minimal UTF-8 -> wchar_t decoder (BMP + astral)
   for(size_t i=0;i<s.size();){unsigned char c=static_cast<unsigned char>(s[i]);unsigned cp=c;size_t extra=0;
     if(c>=0xF0&&i+3<s.size()){cp=(c&7)<<18;extra=3;}else if(c>=0xE0&&i+2<s.size()){cp=(c&15)<<12;extra=2;}else if(c>=0xC0&&i+1<s.size()){cp=(c&31)<<6;extra=1;}
     if(extra){for(size_t k=1;k<=extra;++k)cp|=(static_cast<unsigned char>(s[i+k])&63)<<(6*(extra-k));if(cp>0xFFFF){cp-=0x10000;w+=wchar_t(0xD800+(cp>>10));w+=wchar_t(0xDC00+(cp&1023));}else w+=wchar_t(cp);i+=extra+1;}
     else{w+=wchar_t(c);++i;}}
   return w;};
 // On POSIX platforms the directory arrives as UTF-8 encoded into wchar_t units; decode it back.
 std::string native;for(wchar_t c:utf16directory)native+=static_cast<char>(c);
 if(!native.empty())if(DIR* d=opendir(native.c_str())){while(dirent* e=readdir(d)){const std::string n=e->d_name;if(n=="."||n=="..")continue;auto w=fromUtf8(n);if(w.size()>=presetExtLen&&lower(w.substr(w.size()-presetExtLen))==L".gdspreset")names.push_back(std::move(w));}closedir(d);}
#endif
 return names;
}
inline std::wstring nextPresetFile(const std::wstring& utf16directory,int direction,const std::wstring& current){
 auto lower=[](std::wstring s){for(auto& c:s)c=wchar_t(towlower(c));return s;};
 auto names=presetFilesIn(utf16directory);
 std::sort(names.begin(),names.end(),[&](const std::wstring&a,const std::wstring&b){return lower(a)<lower(b);});
 if(names.empty())return {};
 auto hasExt=[&](const std::wstring&n){return n.size()>=presetExtLen&&lower(n.substr(n.size()-presetExtLen))==L".gdspreset";};
 auto stem=[&](std::wstring n){if(hasExt(n))n.resize(n.size()-presetExtLen);return lower(n);};
 auto pos=std::find_if(names.begin(),names.end(),[&](const std::wstring&n){return stem(n)==stem(current);});
 size_t index=pos==names.end()?size_t(direction>0?int(names.size())-1:0):size_t(pos-names.begin());
 index=(index+(direction>0?names.size()+1:names.size()-1))%names.size();
 return names[index];
}
}
