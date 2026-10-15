#pragma once
#include "parameters.h"
#include "reslice_controls.h"
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

