#pragma once
#include "parameters.h"
#include <cmath>
namespace aztec {
inline std::array<double,kCount> initialParameters() {
  std::array<double,kCount> p{};p[kInputDeclick]=1.;p[kDeclickSensitivity]=.5;p[kReverbRandomRate]=1.;p[kGaterGrid]=2./3.;p[kGaterChance]=.5;for(int i=0;i<16;++i){p[kGaterState0+i]=.5;p[kGaterLength0+i]=(.75-.05)/.95;}
  for(int l=0;l<4;++l){p[kUiLfoCycle0+l]=p[kUiLfoEpoch0+l]=2147483648./4294967295.;p[kLfoSpeed0+l]=2./3.;for(int slot=0;slot<6;++slot)p[slotAmount(l,slot)]=.5;}
  p[kReverbLength]=(2.8-.2)/19.8;
  p[kGrainEnabled]=p[kGlitchEnabled]=p[kRepeatEnabled]=1.;p[kGrainPan]=.5;p[kReverbGrid]=2./3.;p[kReverbMix]=.25;
  for(int i=0;i<16;i+=4)p[kReverbStep0+i]=1.;
  p[kMasterLimiter]=1.;p[kFilterCutoff]=std::log(50.)/std::log(1000.);
  p[kGlitchMove]=.5;p[kGlitchVariation]=.3;p[kGlitchRefresh]=2./7.;p[kRepeatInterval]=4./7.;p[kRepeatDuration]=3./31.;p[kRepeatChance]=1.;
  p[kDensityFlow]=1.; p[kTranspose]=p[kXYX]=p[kXYY]=.5; p[kXAmount]=p[kYAmount]=.75;
  p[kXTarget]=double(kSize+1)/kXYX; p[kYTarget]=double(kPitch+1)/kXYX;
  p[kDensity]=3./31.; p[kDivision]=.25; p[kSize]=.25; p[kPosition]=.05; p[kPitch]=.5;
  p[kMix]=1.; p[kAttack]=.1; p[kRelease]=.15; p[kGrainMix]=1.;
  p[kGlitchDivision]=5./15.; p[kGlitchChance]=1.; p[kRepeatMix]=1.; p[kRepeatDivision]=4./15.;
  p[kGlitchSeq]=1.; p[kRepeatSeq]=0.; p[kStretchSpeed]=.2;
  for(int i=0;i<16;++i) { p[kGlitchStep0+i]=p[kRepeatStep0+i]=1.; p[kRepeatPitch0+i]=.5; }
  for(int i=0;i<16;++i) p[kStep0+i] = 1.;
  for(int i=0;i<qg::lfoCount;++i) {
    const int base=lfoID(i,0); p[base+lSync]=1.; p[base+lHz]=.99/39.99;
    p[base+lGrid]=3./20.; p[kRandomSteps0+i]=15./63.; p[base+lDepth]=1.; p[base+lGlide]=1.;
    for(int t=0;t<8;++t) p[routeID(i,t)]=.5;
  }
  return p;
}
}
namespace aztec {
constexpr const char* factoryNames[]={"01 Clean Grains","02 Forest Drift","03 Psy Scatter","04 Octave Dust","05 Reverse Glitch","06 Micro Stutter","07 Triplet Chops","08 Half Time Mist","09 Space Bloom","10 Acid Motion"};
inline std::array<double,kCount> factoryPreset(int index){
 auto p=initialParameters();p[kNormalize]=1.;p[kGrainMix]=.75;p[kMix]=1.;
 auto pattern=[&](int base,unsigned bits){for(int i=0;i<16;++i)p[base+i]=(bits&(1u<<i))?1.:0.;};
 auto lfo=[&](int wave,int target,double amount,double rate){p[lfoID(0,lEnabled)]=1.;p[lfoID(0,lWave)]=wave/129.;p[slotTarget(0,0)]=(target+1)/11.;p[slotAmount(0,0)]=.5+amount*.5;p[lfoID(0,lGrid)]=rate/20.;};
 switch(index){
 case 0:p[kNormalize]=0.;p[kGrainMix]=1.;p[kSize]=.25;p[kDensity]=7./31.;break;
 case 1:p[kSize]=.6;p[kPosition]=.2;p[kChaos]=.3;p[kPanMode]=.5;lfo(0,2,.16,3);p[kReverbOn]=1.;p[kReverbMix]=.18;break;
 case 2:p[kSize]=.12;p[kDensity]=15./31.;p[kChaos]=.6;p[kPanMode]=1.;lfo(128,2,.32,6);break;
 case 3:p[kPitch]=.625;p[kDensity]=9./31.;p[kGrainPan]=.65;p[kReverbOn]=1.;p[kReverbType]=.75;p[kReverbMix]=.25;p[kReverbLength]=(5.-.2)/19.8;break;
 case 4:p[kModuleOrder]=2./6.;p[kGlitchMix]=.85;p[kGlitchReverse]=1.;p[kGlitchVariation]=.7;p[kGlitchDivision]=4./15.;pattern(kGlitchStep0,0x3333);break;
 case 5:p[kRepeatAuto]=1.;p[kRepeatMix]=.9;p[kRepeatDivision]=6./15.;p[kRepeatInterval]=2./7.;p[kRepeatDuration]=2./31.;p[kGrainMix]=.35;break;
 case 6:p[kRepeatSeq]=1.;p[kRepeatDivision]=10./15.;p[kRepeatMix]=.8;pattern(kRepeatStep0,0x4924);p[kGlitchMix]=.3;pattern(kGlitchStep0,0x8080);break;
 case 7:p[kStretchOn]=1.;p[kStretchSpeed]=(.5-.25)/3.75;p[kSize]=.65;p[kPanMode]=.5;p[kReverbOn]=1.;p[kReverbMix]=.3;p[kReverbLength]=(4.-.2)/19.8;break;
 case 8:p[kSize]=.8;p[kDensity]=7./31.;p[kReverbOn]=1.;p[kReverbType]=1.;p[kReverbMix]=.65;p[kReverbLength]=(12.-.2)/19.8;pattern(kReverbStep0,0x0101);p[kReverbGrid]=1./3.;break;
 case 9:p[kMasterFilter]=1.;p[kFilterType]=.5;p[kFilterCutoff]=std::log(80.)/std::log(1000.);p[kFilterResonance]=.4;p[kFilterDrive]=.25;p[kChaos]=.4;p[kGlitchMix]=.4;pattern(kGlitchStep0,0x5555);lfo(16,8,.6,5);break;
 }
 return p;
}
}
