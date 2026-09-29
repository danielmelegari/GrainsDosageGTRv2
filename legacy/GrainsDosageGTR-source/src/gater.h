#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace qg {
struct GaterSettings {
 bool enabled=false,lengthRandom=false,stepRandom=false;
 double grid=.25,chance=.5;
 std::array<int,16> state{{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}}; // Off, Wet, Dry
 std::array<double,16> length{{.75,.75,.75,.75,.75,.75,.75,.75,.75,.75,.75,.75,.75,.75,.75,.75}};
};
class Gater {
 GaterSettings settings_;double sr_=48000.,wet_=1.,dry_=0.;int step_=0;
 std::array<int,16> states_{};std::array<double,16> lengths_{};int64_t cycle_=INT64_MIN;bool dirty_=true;double lastBeat_=0.;bool haveBeat_=false;uint64_t epoch_=0;
 static double noise(int64_t cycle,int step,uint64_t seed){uint64_t h=uint64_t(cycle)*16+uint64_t(step)+seed;h=(h^(h>>30))*0xbf58476d1ce4e5b9ULL;h=(h^(h>>27))*0x94d049bb133111ebULL;h^=h>>31;return double(h>>11)/9007199254740992.;}
 void pattern(int64_t cycle){for(int i=0;i<16;++i){int base=std::clamp(settings_.state[i],0,2);states_[i]=settings_.stepRandom&&base!=2?(noise(cycle,i,0x913ac5+epoch_*0x9e3779b97f4a7c15ULL)<settings_.chance?1:0):base;double maximum=std::clamp(settings_.length[i],.05,1.);lengths_[i]=settings_.lengthRandom?.05+(maximum-.05)*noise(cycle,i,0xb7ac59+epoch_*0x9e3779b97f4a7c15ULL):maximum;}cycle_=cycle;dirty_=false;}
public:
 void prepare(double sr){sr_=std::max(8000.,sr);wet_=1.;dry_=0.;cycle_=INT64_MIN;dirty_=true;haveBeat_=false;epoch_=0;}
 void set(const GaterSettings& s){if(s.enabled!=settings_.enabled||s.lengthRandom!=settings_.lengthRandom||s.stepRandom!=settings_.stepRandom||s.grid!=settings_.grid||s.chance!=settings_.chance||s.state!=settings_.state||s.length!=settings_.length)dirty_=true;settings_=s;}
 void process(float& l,float& r,double beat,double tempo){if(haveBeat_&&beat<lastBeat_-1e-6){++epoch_;dirty_=true;}lastBeat_=beat;haveBeat_=true;double grid=std::max(.03125,settings_.grid),position=beat/grid;int64_t tick=int64_t(std::floor(position+1e-10));step_=int((tick%16+16)%16);int64_t cycle=int64_t(std::floor(double(tick)/16.));if(dirty_||cycle!=cycle_)pattern(cycle);
  double phase=std::clamp(position-double(tick),0.,1.),stepSamples=grid*60./std::max(20.,tempo)*sr_,length=lengths_[step_];double edge=std::min(sr_*.003,stepSamples*length*.25);
  double envelope=std::clamp(std::min(phase*stepSamples,(length-phase)*stepSamples)/std::max(1.,edge),0.,1.);envelope=envelope*envelope*(3.-2.*envelope);
  double w=settings_.enabled?(states_[step_]==1?envelope:0.):1.,d=settings_.enabled&&states_[step_]==2?envelope:0.;
  // Short dezipper also protects mode toggles and edits mid-step.
  double slew=1./(.001*sr_);wet_+=std::clamp(w-wet_,-slew,slew);dry_+=std::clamp(d-dry_,-slew,slew);
  l=float(l*wet_);r=float(r*wet_);
 }
 double dryGain()const{return dry_;}int step()const{return step_;}int state(int i)const{return states_[i];}double length(int i)const{return lengths_[i];}
};
}
