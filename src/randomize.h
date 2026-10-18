#pragma once
#include "parameters.h"
#include "reslice_controls.h"
#include "filter_bank.h"
#include <cstdint>
#include <utility>
namespace aztec {
template<class Getter,class Setter> void reseedReslice(uint32_t& seed,Getter get,Setter set){
 seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;
 auto old=uint32_t(std::round(get(kResliceSeed)*65534.));set(kResliceSeed,double((old+1+seed%65534)%65535)/65534.);

}
// Randomize all current sound controls, including hidden mode sections. Keep
// module power and lock state intact; retired manual-step parameters are inert.
template<class Getter,class Setter> void randomizeReslice(uint32_t& seed,Getter get,Setter set){
 if(!seed)seed=0x9e3779b9u;
 auto rnd=[&](){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return double(seed&0xffffff)/16777215.;};
 auto discrete=[&](int steps){return std::min(steps,int(rnd()*(steps+1)))/double(steps);};
 set(kResliceAlgorithm,discrete(2));set(kResliceLength,discrete(3));
 double phrase=discrete(3);set(kReslicePhrase,phrase);
 for(ParamID id:{kResliceRepeat,kResliceVariation,kResliceFill,kResliceReverse})set(id,rnd());
 for(const auto& spec:resliceControlSpecs)set(spec.id,spec.steps?discrete(spec.steps):rnd());
 for(auto pair: {std::pair<ParamID,ParamID>{kResliceMinAmp,kResliceMaxAmp},
                 {kResliceMinPan,kResliceMaxPan},{kResliceMinPitch,kResliceMaxPitch},
                 {kResliceMinRepeats,kResliceMaxRepeats},{kResliceMinBits,kResliceMaxBits},
                 {kResliceMinFreq,kResliceMaxFreq},{kResliceMinDelay,kResliceMaxDelay}}){
  double a=get(pair.first),b=get(pair.second);set(pair.first,std::min(a,b));set(pair.second,std::max(a,b));
 }
 int maxBars=1<<int(std::round(phrase*3));
 set(kResliceMinPhrase,std::min(maxBars-1,int(rnd()*maxBars))/7.);
 set(kModuleLoopOn0+2,discrete(1));set(kModuleLoopWindow0+2,discrete(3));
 if(!mixIsLocked(kResliceMix,get))set(kResliceMix,rnd());
 reseedReslice(seed,get,set);
}
template<class Setter> void randomizeModule(int module,int selectedStep,uint32_t& seed,Setter set){
  auto rnd=[&](){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return double(seed&0xffffff)/16777215.;};
  auto range=[&](ParamID id,double lo,double hi){set(id,lo+(hi-lo)*rnd());};
  if(module==0){set(kGrainPan,.5);set(kPanMode,0.);set(kDivision,int(rnd()*15.999)/15.);set(kDensityFlow,rnd()>.5?1.:0.);set(kGrainBuffer,int(rnd()*4.999)/4.);set(kFreeze,rnd()>.5?1.:0.);range(kSize,.064,.70);range(kDensity,1./31.,15./31.);set(kPitch,(24.+int(rnd()*49.))/96.);range(kPosition,0.,.25);range(kChaos,0.,.7);range(kGrainMix,.35,1.);range(kAttack,.01,.5);range(kRelease,.02,.6);range(kFeedback,0.,.5);}
  if(module==1){set(kGlitchDivision,(4.+int(rnd()*4.))/15.);range(kGlitchChance,.5,1.);range(kGlitchMix,.5,1.);range(kGlitchMove,.1,1.);range(kGlitchVariation,0.,1.);set(kGlitchTriggerRate,std::min(3,int(rnd()*4.))/3.);}
  if(module==2){set(kRepeatDivision,(3.+int(rnd()*5.))/15.);range(kRepeatMix,.5,1.);set(kRepeatInterval,(1.+int(rnd()*5.))/7.);set(kRepeatDuration,(1.+int(rnd()*7.))/31.);range(kRepeatChance,.5,1.);set(kRepeatPitch0+selectedStep,(36.+int(rnd()*25.))/96.);for(int i=0;i<16;++i)set(kRepeatRate0+i,int(rnd()*15.999)/15.);}
}
template<class Getter,class Setter> void randomizeModule(int module,int selectedStep,uint32_t& seed,Getter get,Setter set){
 randomizeModule(module,selectedStep,seed,[&](ParamID id,double v){if(!mixIsLocked(id,get))set(id,v);});
}

}


namespace aztec {
// One sound generator shared by native UI and the host's legacy trigger.
// Work from a snapshot so paired ranges are ordered even with asynchronous edits.
template<class Getter,class Setter> void randomizeSound(uint32_t& seed,Getter get,Setter apply){
 std::array<double,kCount> values{};std::array<bool,kCount> touched{};
 for(int i=0;i<kCount;++i)values[i]=get(ParamID(i));
 auto read=[&](ParamID id){return values[id];};
 auto set=[&](ParamID id,double v){if(!mixIsLocked(id,read)){values[id]=std::clamp(v,0.,1.);touched[id]=true;}};
 if(!seed)seed=0x9e3779b9u;
 auto rnd=[&](){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return double(seed&0xffffff)/16777216.;};
 auto choice=[&](int steps){return int(rnd()*(steps+1))/double(steps);};
 randomizeModule(0,0,seed,read,set);randomizeModule(2,0,seed,read,set);
 randomizeReslice(seed,read,set);
 for(ParamID id:{kStretchSpeed,kTranspose,kGrainPan,kRepeatChance,kGaterChance,kGaterMinLength,
   kFilterCutoff,kFilterResonance,kFilterDrive,kFilterSeqDepth,kFilterSeqGlide,kReverbLength,
   kReverbMix,kXYX,kXYY,kXAmount,kYAmount})set(id,rnd());
 for(ParamID id:{kStretchOn,kRepeatAuto,kRepeatSeq,kGaterLengthRnd,kGaterStepRnd,kGaterLatch,
   kMasterFilter,kFilterSeqOn,kResliceRndOn,kReverbOn,kXYEnable})set(id,choice(1));
 set(kGrainEnabled,1.);set(kRepeatEnabled,choice(1));set(kResliceEnabled,choice(1));set(kGaterEnabled,choice(1));
 set(kReverbKill,0.);set(kFreeze,0.); // A new patch must accept incoming audio, including while stopped.
 set(kMix,.6+.4*rnd());set(kGaterGrid,choice(3));set(kPanMode,choice(2));
 set(kFilterType,choice(2));set(kFilterModel,choice(qg::filterModelCount-1));set(kFilterSlope,choice(1));
 set(kFilterSeqMode,choice(1));set(kFilterSeqPattern,choice(63));set(kFilterSeqRate,choice(5));
 set(kCombRoot,choice(11));set(kCombOctave,choice(6));set(kCombScale,choice(1));
 set(kReverbType,choice(4));set(kReverbModel,choice(4));set(kReverbSource,choice(1));
 set(kReverbGrid,choice(3));set(kReverbRateV2,choice(6));set(kReverbRandomRate,choice(2));set(kResliceRndRate,choice(2));
 set(kRoutingOrder,(1+int(rnd()*120))/120.);
 for(int i=0;i<16;++i){
  set(kStep0+i,choice(1));set(kRepeatStep0+i,choice(1));set(kRepeatRate0+i,choice(15));
  set(kRepeatPitch0+i,(36+int(rnd()*25))/96.);set(kReverbStep0+i,choice(1));
  set(kGaterState0+i,i==0?1.:choice(1));set(kGaterLength0+i,rnd());set(kGaterSustain0+i,rnd());set(kGaterRelease0+i,0.);
 }
 for(int i=0;i<qg::lfoCount;++i){
  if(i<4){set(kModuleLoopOn0+i,0.);set(kModuleLoopWindow0+i,choice(3));}
  for(int j=0;j<lRoute0;++j)set(lfoID(i,j),rnd());
  set(lfoID(i,lEnabled),i==0?1.:choice(1));set(lfoID(i,lSync),1.);set(lfoID(i,lDepth),.15+.5*rnd());
  set(modParam(kLfoWaveChoice0,i),(1+int(rnd()*qg::waveChoiceCount))/double(qg::waveChoiceCount));
  set(lfoID(i,lGrid),choice(20));set(lfoID(i,lReset),choice(1));set(modParam(kRandomSteps0,i),choice(63));
  set(modParam(kLfoSpeed0,i),choice(3));set(modParam(kModWaveRate0,i),choice(7));
  for(int j=0;j<8;++j)set(routeID(i,j),.5);
  for(int j=0;j<6;++j){int target;
   do{target=1+int(rnd()*qg::modTargetCount);}while(!availableMenuOption(slotTarget(i,j),target)||(target-1>=101&&extendedResliceIds[target-1-101]==kModuleLoopOn0+2));
   set(slotTarget(i,j),target/double(qg::modTargetCount));set(slotAmount(i,j),.35+.3*rnd());set(slotPolarity(i,j),choice(2));
  }
 }
 set(kRndSource,choice(1));set(kRndRoot,choice(11));set(kRndMinor,choice(1));set(kRndGateEvery,choice(15));
 double noteA=choice(127),noteB=choice(127);set(kRndNoteMin,std::min(noteA,noteB));set(kRndNoteMax,std::max(noteA,noteB));
 double lengthA=rnd(),lengthB=rnd();set(kRndLengthMin,std::min(lengthA,lengthB));set(kRndLengthMax,std::max(lengthA,lengthB));
 for(auto id:{kXTarget,kYTarget}){int option;do{option=1+int(rnd()*xyDestCount);}while(!availableMenuOption(id,option)||(xyDestinationId(option-1)>=kModuleLoopOn0&&xyDestinationId(option-1)<kModuleLoopWindow0));set(id,option/double(xyDestCount));}
 // Global Random uses full wet modules or bypass, while explicit Mix locks win.
 for(auto module:{std::pair<ParamID,ParamID>{kGrainEnabled,kGrainMix},
                  {kRepeatEnabled,kRepeatMix},{kResliceEnabled,kResliceMix},{kReverbOn,kReverbMix}})
  set(module.second,read(module.first)>=.5?1.:0.);
 // Quality, output protection, de-click settings, locks and UI monitors stay put.
 for(int i=0;i<kCount;++i)if(touched[i])apply(ParamID(i),values[i]);
}
}
