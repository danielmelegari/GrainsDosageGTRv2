#pragma once
#include "parameters.h"
#include "reslice.h"
namespace aztec {
inline void applyResliceDestination(qg::ResliceSettings& s,int index,double v){
 v=std::clamp(v,0.,1.);
 switch(index){
 case 0: s.enabled=v>=.5;break;
 case 1: s.algorithm=int(std::round(v*2));break;
 case 2: s.phraseBeats=s.barBeats*std::pow(2.,std::round(v*3));break;
 case 3: s.repeat=v;break;
 case 4: s.variation=v;break;
 case 5: s.fill=v;break;
 case 6: s.reverse=v;break;
 case 7: s.seed=1+uint32_t(std::round(v*65534));break;
 case 8: v=std::round(v*31)/31.;s.subdivision=int((1)+((32)-(1))*v);break;
 case 9: s.fadeMs=(0)+((50)-(0))*v;break;
 case 10: s.minAmp=(0)+((2)-(0))*v;break;
 case 11: s.maxAmp=(0)+((2)-(0))*v;break;
 case 12: s.minPan=((-100)+((100)-(-100))*v)/100.;break;
 case 13: s.maxPan=((-100)+((100)-(-100))*v)/100.;break;
 case 14: s.minPitch=(-2400)+((2400)-(-2400))*v;break;
 case 15: s.maxPitch=(-2400)+((2400)-(-2400))*v;break;
 case 16: s.duty=((0)+((100)-(0))*v)/100.;break;
 case 17: s.fillDuty=((0)+((100)-(0))*v)/100.;break;
 case 18: v=std::round(v*7)/7.;s.minPhraseBeats=s.barBeats*((1)+((8)-(1))*v);break;
 case 19: v=std::round(v*16)/16.;s.minRepeats=int((0)+((16)-(0))*v);break;
 case 20: v=std::round(v*16)/16.;s.maxRepeats=int((0)+((16)-(0))*v);break;
 case 21: s.stutter=((0)+((100)-(0))*v)/100.;break;
 case 22: s.area=((0)+((100)-(0))*v)/100.;break;
 case 23: s.straight=((0)+((100)-(0))*v)/100.;break;
 case 24: s.regular=((0)+((100)-(0))*v)/100.;break;
 case 25: s.ritard=((0)+((100)-(0))*v)/100.;break;
 case 26: s.warpSpeed=(0)+((1)-(0))*v;break;
 case 27: s.activity=((0)+((100)-(0))*v)/100.;break;
 case 28: v=std::round(v*1)/1.;s.crusher=v>=.5;break;
 case 29: v=std::round(v*31)/31.;s.minBits=int((1)+((32)-(1))*v);break;
 case 30: v=std::round(v*31)/31.;s.maxBits=int((1)+((32)-(1))*v);break;
 case 31: s.minFreq=(100)+((48000)-(100))*v;break;
 case 32: s.maxFreq=(100)+((48000)-(100))*v;break;
 case 33: v=std::round(v*1)/1.;s.comb=v>=.5;break;
 case 34: v=std::round(v*1)/1.;s.combType=int((0)+((1)-(0))*v);break;
 case 35: s.combFeedback=((0)+((95)-(0))*v)/100.;break;
 case 36: s.minDelay=(1)+((100)-(1))*v;break;
 case 37: s.maxDelay=(1)+((100)-(1))*v;break;
 }
}
}
