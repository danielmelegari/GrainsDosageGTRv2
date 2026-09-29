#pragma once
#include "parameters.h"
#include <cstdint>
namespace aztec {
template<class Getter,class Setter> void randomizeReslice(uint32_t& seed,Getter get,Setter set){
 set(kResliceRndOn,0.); // One-shot mode stays fixed after this edit.
 for(int i=0;i<16;++i){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;
   int old=int(std::round(get(kResliceIndex0+i)*15.));set(kResliceIndex0+i,((old+1+int(seed%15))%16)/15.);}
}
template<class Setter> void randomizeModule(int module,int selectedStep,uint32_t& seed,Setter set){
  auto rnd=[&](){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return double(seed&0xffffff)/16777215.;};
  auto range=[&](ParamID id,double lo,double hi){set(id,lo+(hi-lo)*rnd());};
  if(module==0){range(kGrainPan,0.,1.);range(kSize,.064,.70);range(kDensity,1./31.,15./31.);set(kPitch,(24.+int(rnd()*49.))/96.);range(kPosition,0.,.25);range(kChaos,0.,.7);range(kGrainMix,.35,1.);range(kStretchSpeed,.25/3.75,1.75/3.75);set(kTranspose,(36.+int(rnd()*25.))/96.);}
  if(module==1){set(kGlitchDivision,(4.+int(rnd()*4.))/15.);range(kGlitchChance,.5,1.);range(kGlitchMix,.5,1.);range(kGlitchMove,.1,1.);range(kGlitchVariation,0.,1.);set(kGlitchTriggerRate,std::min(3,int(rnd()*4.))/3.);}
  if(module==2){set(kRepeatDivision,(3.+int(rnd()*5.))/15.);range(kRepeatMix,.5,1.);set(kRepeatInterval,(1.+int(rnd()*5.))/7.);set(kRepeatDuration,(1.+int(rnd()*7.))/31.);range(kRepeatChance,.5,1.);set(kRepeatPitch0+selectedStep,(36.+int(rnd()*25.))/96.);}
}
}
