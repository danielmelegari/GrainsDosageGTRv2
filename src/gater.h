#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <climits>
namespace qg {
struct GaterSettings {
 bool enabled=false,lengthRandom=false,stepRandom=false,latch=false,tie=false;
 uint16_t release=0;
 double grid=.25,chance=.5,minimumLength=.05;
 std::array<int,16> state{{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}}; // Off, Wet. Legacy Dry values are read as Wet.
 std::array<double,16> length{{.75,.75,.75,.75,.75,.75,.75,.75,.75,.75,.75,.75,.75,.75,.75,.75}};
 std::array<double,16> sustain{{.05,.05,.05,.05,.05,.05,.05,.05,.05,.05,.05,.05,.05,.05,.05,.05}}; // seconds
};
class Gater {
 GaterSettings settings_;
 double sr_=48000.,wet_=1.,releaseStep_=0.;
 int step_=0,latched_=0,lastWetStep_=0;
 int64_t latchTick_=INT64_MIN,cycle_=INT64_MIN;
 bool latchPlaying_=true,gateWasOpen_=true,dirty_=true,haveBeat_=false;
 std::array<int,16> states_{};
 std::array<double,16> lengths_{},sustains_{};
 double phase_=0.;double lastBeat_=0.;uint64_t epoch_=0;
 static double noise(int64_t cycle,int step,uint64_t seed){uint64_t h=uint64_t(cycle)*16+uint64_t(step)+seed;h=(h^(h>>30))*0xbf58476d1ce4e5b9ULL;h=(h^(h>>27))*0x94d049bb133111ebULL;h^=h>>31;return double(h>>11)/9007199254740992.;}
 void pattern(int64_t cycle){
  const double minimum=std::clamp(settings_.minimumLength,.05,.95);
  for(int i=0;i<16;++i){
   int base=std::clamp(settings_.state[i],0,1);
   states_[i]=settings_.stepRandom&&base==1?(noise(cycle,i,0x913ac5+epoch_*0x9e3779b97f4a7c15ULL)<settings_.chance?1:0):base;
   double maximum=std::clamp(settings_.length[i],minimum,1.);
   lengths_[i]=settings_.lengthRandom?minimum+(maximum-minimum)*noise(cycle,i,0xb7ac59+epoch_*0x9e3779b97f4a7c15ULL):maximum;
   sustains_[i]=std::clamp(settings_.sustain[i],0.,.5);
  }
  cycle_=cycle;dirty_=false;
 }
public:
 void prepare(double sr){phase_=0.;sr_=std::max(8000.,sr);wet_=1.;releaseStep_=0.;cycle_=INT64_MIN;dirty_=true;haveBeat_=false;epoch_=0;latched_=0;lastWetStep_=0;latchTick_=INT64_MIN;latchPlaying_=true;gateWasOpen_=true;}
 void resetLatch(){latched_=0;latchTick_=INT64_MIN;}
 void set(const GaterSettings& s){if(s.latch!=settings_.latch||s.enabled!=settings_.enabled)resetLatch();if(s.enabled!=settings_.enabled||s.lengthRandom!=settings_.lengthRandom||s.stepRandom!=settings_.stepRandom||s.grid!=settings_.grid||s.chance!=settings_.chance||s.minimumLength!=settings_.minimumLength||s.state!=settings_.state||s.length!=settings_.length||s.sustain!=settings_.sustain)dirty_=true;settings_=s;}
 void process(float& l,float& r,double beat,double tempo,bool playing=true){
  if(haveBeat_&&beat<lastBeat_-1e-6){++epoch_;dirty_=true;}lastBeat_=beat;haveBeat_=true;
  double grid=std::max(.03125,settings_.grid),position=beat/grid;
  int64_t tick=int64_t(std::floor(position+1e-10));step_=int((tick%16+16)%16);
  int64_t cycle=int64_t(std::floor(double(tick)/16.));if(dirty_||cycle!=cycle_)pattern(cycle);
  double phase=std::clamp(position-double(tick),0.,1.),length=lengths_[step_];phase_=settings_.enabled&&playing?phase:0.;
  int state=states_[step_];bool targetOpen;
  if(settings_.latch){
   if(!playing){resetLatch();latchPlaying_=false;}
   else {if(!latchPlaying_){resetLatch();latchPlaying_=true;}if(tick!=latchTick_){if(settings_.release&(uint16_t(1)<<step_))latched_=0;else if(state!=0)latched_=1;latchTick_=tick;}}
   targetOpen=!settings_.enabled||latched_!=0;
  }else{
   // TIE removed: adjacent wet steps no longer merge into one continuous note.
   targetOpen=!settings_.enabled||(state==1&&phase<length);
  }

  if(targetOpen){
   if(settings_.enabled&&state==1)lastWetStep_=step_;
   releaseStep_=0.;wet_=std::min(1.,wet_+1./std::max(1.,.003*sr_));
  }else{
   if(gateWasOpen_){double seconds=sustains_[lastWetStep_];seconds=std::max(seconds,.002);releaseStep_=wet_/std::max(1.,seconds*sr_);}
   wet_=std::max(0.,wet_-releaseStep_);
  }
  gateWasOpen_=targetOpen;
  l=float(l*wet_);r=float(r*wet_);
 }
 int step()const{return step_;}int state(int i)const{return states_[i];}
 double length(int i)const{return lengths_[i];}double sustain(int i)const{return sustains_[i];}
 double envelope()const{return wet_;}
 double phase()const{return phase_;}
};
}

