#pragma once
#include "factory_presets.h"
#include <sstream>
#include <iomanip>
#include <locale>
#include <cmath>
#include <string>
namespace aztec {
inline bool presetParameter(int id){return id>=0&&id<presetCount&&!isMonitor(id);}
template<class Getter> std::string encodePreset(Getter get){
 std::ostringstream out;out.imbue(std::locale::classic());out<<"GrainsDosagePreset 1 "<<presetCount<<'\n'<<std::setprecision(17);
 for(int i=0;i<presetCount;++i)out<<i<<' '<<(presetParameter(i)?get(i):0.)<<'\n';return out.str();
}
// Parse and validate completely before touching any live parameter.
inline bool decodePreset(const std::string& text,std::array<double,kCount>& values){
 if(text.size()>65536)return false;std::istringstream in(text);in.imbue(std::locale::classic());
 std::string magic;int version=0,count=0;if(!(in>>magic>>version>>count)||magic!="GrainsDosagePreset"||version!=1||(count!=kUiWave0&&count!=kLfoSlots0&&count!=kModWaveRnd0&&count!=kReverbSource&&count!=kLimiterCeiling&&count!=kGaterEnabled&&count!=kInputDeclick&&count!=kCount))return false;
 auto temp=initialParameters();for(int i=0;i<count;++i){int id;double v;if(!(in>>id>>v)||id!=i||!std::isfinite(v)||v<0.||v>1.)return false;temp[i]=v;}
 if(count<=kInputDeclick)temp[kInputDeclick]=0.;in>>std::ws;if(!in.eof())return false;if(count<kLfoSlots0+1)migrateRoutes(temp);values=temp;return true;
}
}
