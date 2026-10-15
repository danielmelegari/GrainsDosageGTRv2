#pragma once
#include "preset_io.h"
namespace aztec {
// Stable stage IDs; module files never contain global routing, modulation or licensing state.
inline bool modulePresetParameter(int stage,int id){
 const int loop=moduleLoopIndex(stage);
 if(loop<0||!presetParameter(id))return false;
 if(id==int(moduleLoopOn(loop))||id==int(moduleLoopWindow(loop)))return true;
 switch(stage){
 case 0:return (id>=kDivision&&id<=kGrainMix&&id!=kMix)||id==kDensityFlow||id==kGrainEnabled||id==kGrainPan||id==kPanMode||id==kGrainBuffer||id==kFreeze||id==kMixLock0;
 case 2:return (id>=kRepeatOn&&id<=kRepeatDivision)||id==kRepeatSeq||(id>=kRepeatStep0&&id<kStretchOn)||(id>=kRepeatAuto&&id<=kRepeatChance)||id==kRepeatEnabled||id==kMixLock0+2;
 case 3:return (id>=kResliceEnabled&&id<kGaterLatch)||id==kResliceRndOn||id==kResliceRndRate||(id>=kResliceAlgorithm&&id<kModuleLoopOn0)||id==kMixLock0+3;
 case 4:return (id>=kGaterEnabled&&id<kUiGaterStep)||(id>=kGaterLatch&&id<kUiResliceStep)||id==kGaterMinLength||id==kGaterTie||(id>=kGaterSustain0&&id<kGlitchTriggerRate);
 default:return false;
 }
}
template<class Getter> std::string encodeModulePreset(int stage,Getter get){
 int count=0;for(int id=0;id<kCount;++id)if(modulePresetParameter(stage,id))++count;
 std::ostringstream out;out.imbue(std::locale::classic());out<<"GrainsDosageModule 1 "<<stage<<' '<<count<<'\n'<<std::setprecision(17);
 for(int id=0;id<kCount;++id)if(modulePresetParameter(stage,id))out<<id<<' '<<get(id)<<'\n';return out.str();
}
inline bool decodeModulePreset(const std::string& text,int stage,std::array<double,kCount>& values){
 if(text.size()>65536||!(stage==0||stage==2||stage==3||stage==4))return false;
 std::istringstream in(text);in.imbue(std::locale::classic());std::string magic;int version=0,module=-1,count=0,expected=0;
 for(int id=0;id<kCount;++id)if(modulePresetParameter(stage,id))++expected;
 if(!(in>>magic>>version>>module>>count)||magic!="GrainsDosageModule"||version!=1||module!=stage||count!=expected)return false;
 auto temp=values;for(int id=0;id<kCount;++id)if(modulePresetParameter(stage,id)){int read;double v;if(!(in>>read>>v)||read!=id||!std::isfinite(v)||v<0||v>1)return false;temp[id]=v;}
 in>>std::ws;if(!in.eof())return false;values=temp;return true;
}
}
