#pragma once
#include "factory_presets.h"
#include <sstream>
#include <iomanip>
#include <locale>
#include <cmath>
#include <string>
#include <algorithm>
#include <vector>
#include <filesystem>
#include <cwctype>
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
inline std::wstring nextPresetFile(const std::filesystem::path& directory,int direction,const std::wstring& current){
 auto lower=[](std::wstring s){for(auto& c:s)c=wchar_t(towlower(c));return s;};
 std::vector<std::wstring> names;std::error_code ec;
 for(auto& e:std::filesystem::directory_iterator(directory,ec))if(!e.is_directory()){auto p=e.path().filename().wstring();if(p.size()>11&&lower(p.substr(p.size()-11))==L".gdspreset")names.push_back(std::move(p));}
 std::sort(names.begin(),names.end(),[&](const std::wstring&a,const std::wstring&b){return lower(a)<lower(b);});
 if(names.empty())return {};
 const wchar_t* ext=L".gdspreset";auto stem=[&](std::wstring n){if(n.size()>11&&lower(n.substr(n.size()-11))==ext)n.resize(n.size()-11);return lower(n);};
 auto pos=std::find_if(names.begin(),names.end(),[&](const std::wstring&n){return stem(n)==stem(current);});
 size_t index=pos==names.end()?size_t(direction>0?int(names.size())-1:0):size_t(pos-names.begin());
 index=(index+(direction>0?names.size()+1:names.size()-1))%names.size();
 return names[index];
}
}
