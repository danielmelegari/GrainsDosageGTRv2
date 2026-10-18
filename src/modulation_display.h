#pragma once
#include "parameters.h"
namespace aztec {
// Persistent DSP destination order; all editors use the same normalized values.
inline int modulationParameter(int target){
 constexpr ParamID ids[]={kSize,kDensity,kPitch,kPosition,kChaos,kGrainMix,kStretchSpeed,kTranspose,kFilterCutoff,kFilterResonance,kFilterDrive,kGrainPan,kGlitchMix,kGlitchChance,kGlitchMove,kGlitchVariation,kRepeatMix,kRepeatChance,kRepeatDuration,kReverbMix,kReverbLength,kResliceMix,kGaterChance,kGaterMinLength,kMix,kFilterSeqDepth,kFilterSeqGlide,kGlitchDivision,kRepeatDivision,kGlitchTriggerRate,kRepeatInterval,kReverbGrid,kReverbRandomRate,kGaterGrid,kResliceLength,kResliceRndRate,kFilterSeqRate};
 if(target<0)return -1;
 if(target<37)return ids[target];
 if(target<53)return kRepeatPitch0+target-37;
 if(target<69)return kGaterLength0+target-53;
 if(target<85)return kGaterSustain0+target-69;
 if(target<101)return kResliceIndex0+target-85;
 return target<qg::modTargetCount?int(extendedResliceIds[target-101]):-1;
}
template<class Getter> std::array<double,kCount> modulationDisplayValues(Getter get){
 std::array<double,kCount> p{};for(int id=0;id<kCount;++id)p[id]=std::clamp(get(ParamID(id)),0.,1.);
 const auto saved=p;
 // Match settings(): apply X then Y, clamping each axis before LFO modulation.
 if(saved[kXYEnable]>=.5)for(int axis=0;axis<2;++axis){
  int id=xyDestinationId(int(std::round(saved[axis?kYTarget:kXTarget]*xyDestCount))-1);
  if(id>=0)p[id]=std::clamp(p[id]+(saved[axis?kXYY:kXYX]*2.-1.)*(saved[axis?kYAmount:kXAmount]*2.-1.),0.,1.);
 }
 auto result=p;
 if(saved[kUiModulationActive]>=.5)for(int l=0;l<4;++l){
  if(p[lfoID(l,lEnabled)]<.5)continue;
  double sample=saved[kUiLfoValue0+l]*2.-1.;
  double unipolar=(sample+p[lfoID(l,lDepth)])*.5;
  auto add=[&](int target,double amount){int id=modulationParameter(target);if(id>=0&&!retiredPreslicerParam(id))result[id]+=amount;};
  for(int t=0;t<8;++t)add(t,sample*(p[routeID(l,t)]*2.-1.));
  for(int slot=0;slot<6;++slot){int target=int(std::round(p[slotTarget(l,slot)]*qg::modTargetCount))-1;double amount=p[slotAmount(l,slot)]*2.-1.;int polarity=int(std::round(p[slotPolarity(l,slot)]*2.));
   add(target,polarity==1?unipolar*std::abs(amount):polarity==2?-unipolar*std::abs(amount):sample*amount);
  }
 }
 for(auto& v:result)v=std::clamp(v,0.,1.);
 return result;
}
}
