#pragma once
#include "modulation.h"
namespace qg {
struct FilterSequenceSettings {bool enabled=false;int mode=0,pattern=0,root=0,octave=2,scale=0;double rate=.25,depth=.5,glide=.2;};
// 64 original 32-step patterns: eight motion families, eight variations.
// These are not extracted Virus TI preset data.
inline double filterPattern(int pattern,int step){
 int family=std::clamp(pattern,0,63)/8,v=pattern%8,j=(step*(1+2*(v%3))+v*3)%32;
 double t=(j%16)/15.;
 switch(family){case 0:return 2*t-1;case 1:return 1-2*t;case 2:return 1-4*std::abs(t-.5);case 3:return (j%2?1.:-1.)*(.35+.65*t);case 4:return 2.*((j/2+v)%7)/6.-1.;case 5:return std::sin(tau*(j+v)/double(5+v));case 6:return cycleNoise(j/2,347+v*971);default:return ((j*(v+3))%13<5)?1.-t:-1.+t*.4;}
}
inline int minorNote(double midi,int root,bool scale,int maxMidi=119){
 constexpr int minor[]={0,2,3,5,7,8,10};constexpr int chord[]={0,3,7};int best=0;double distance=1e9;
 for(int n=24;n<=maxMidi;++n){int pc=(n-root+120)%12;bool allowed=false;for(int i=0;i<(scale?7:3);++i)if(pc==(scale?minor[i]:chord[i]))allowed=true;
  if(allowed&&std::abs(n-midi)<distance){distance=std::abs(n-midi);best=n;}}
 return best;
}
class FilterSequencer {
 double current_=0.;int step_=0;
public:
 void reset(){current_=0.;step_=0;}
 int step()const{return step_;}
 double process(const FilterSequenceSettings& s,double cutoff,double beat,double sr,bool playing,bool comb){
  double value=0.;double pos=beat/std::max(.03125,s.rate);int64_t tick=int64_t(std::floor(pos));step_=int((tick%32+32)%32);
  if(s.enabled&&playing){
   if(s.mode==0)value=filterPattern(s.pattern,step_);
   else{double a=cycleNoise(tick-1,0x83932),b=cycleNoise(tick,0x83932);double t=std::clamp((pos-std::floor(pos))/std::max(.01,s.glide),0.,1.);value=a+(b-a)*smooth(t);}
  }
  double smoothing=std::exp(-1./(sr*(.002+s.glide*.08)));current_=value+smoothing*(current_-value);
  if(comb){double midi=12.*(s.octave+1)+s.root+(cutoff-.5)*48.+current_*s.depth*24.;int maximum=int(std::floor(69.+12.*std::log2(std::min(20000.,sr*.2)/440.)));int note=minorNote(midi,s.root,s.scale!=0,maximum);return 440.*std::pow(2.,(note-69)/12.);}
  return 20.*std::pow(1000.,std::clamp(cutoff+current_*s.depth*.4,0.,1.));
 }
};
}
