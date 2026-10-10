#pragma once
#include "parameters.h"
namespace aztec {
struct ResliceControlSpec {ParamID id;const char* name;const char* unit;double lo,hi,initial;int steps;};
inline constexpr ResliceControlSpec resliceControlSpecs[]={
 {kResliceSubdivision,"SUBDIV","",1,32,8,31},
 {kResliceFade,"FADE","ms",0,50,3,0},
 {kResliceMinAmp,"MIN AMP","",0,2,1,0},
 {kResliceMaxAmp,"MAX AMP","",0,2,1,0},
 {kResliceMinPan,"MIN PAN","%",-100,100,0,0},
 {kResliceMaxPan,"MAX PAN","%",-100,100,0,0},
 {kResliceMinPitch,"MIN PITCH","cent",-2400,2400,0,0},
 {kResliceMaxPitch,"MAX PITCH","cent",-2400,2400,0,0},
 {kResliceDuty,"DUTY","%",0,100,100,0},
 {kResliceFillDuty,"FILL DUTY","%",0,100,100,0},
 {kResliceMinPhrase,"MIN PHRASE","bars",1,8,4,7},
 {kResliceMinRepeats,"MIN REPEATS","",0,16,0,16},
 {kResliceMaxRepeats,"MAX REPEATS","",0,16,0,16},
 {kResliceStutter,"STUTTER","%",0,100,0,0},
 {kResliceArea,"AREA","%",0,100,100,0},
 {kResliceStraight,"STRAIGHT","%",0,100,50,0},
 {kResliceRegular,"REGULAR","%",0,100,0,0},
 {kResliceRitard,"RITARD","%",0,100,0,0},
 {kResliceWarpSpeed,"SPEED","",0,1,0.5,0},
 {kResliceActivity,"ACTIVITY","%",0,100,100,0},
 {kResliceCrushOn,"CRUSHER","",0,1,0,1},
 {kResliceMinBits,"MIN BITS","bits",1,32,32,31},
 {kResliceMaxBits,"MAX BITS","bits",1,32,32,31},
 {kResliceMinFreq,"MIN FREQ","Hz",100,48000,44100,0},
 {kResliceMaxFreq,"MAX FREQ","Hz",100,48000,44100,0},
 {kResliceCombOn,"COMB","",0,1,0,1},
 {kResliceCombType,"COMB TYPE","",0,1,0,1},
 {kResliceCombFeedback,"FEEDBACK","%",0,95,50,0},
 {kResliceMinDelay,"MIN DELAY","ms",1,100,10,0},
 {kResliceMaxDelay,"MAX DELAY","ms",1,100,10,0},
};
inline double reslicePlain(const std::array<double,kCount>& p,ParamID id){const auto& s=resliceControlSpecs[id-kResliceSubdivision];double v=std::clamp(p[id],0.,1.);if(s.steps)v=std::round(v*s.steps)/s.steps;return s.lo+(s.hi-s.lo)*v;}
inline void resliceDefaults(std::array<double,kCount>& p){for(const auto& s:resliceControlSpecs)p[s.id]=(s.initial-s.lo)/(s.hi-s.lo);}
}
