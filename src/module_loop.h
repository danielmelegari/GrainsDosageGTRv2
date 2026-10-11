#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
namespace qg {
// Independent post-module capture. One prepared ring, frozen in place: no copies
// or allocations on capture. Eight quarter notes at minimum supported 20 BPM.
class ModuleLoop {
 std::array<std::vector<float>,2> ring_;
 size_t write_=0,valid_=0,start_=0,length_=0,needed_=1;
 double sr_=48000,tempo_=0,selected_=0,heldBeats_=0,position_=0,advance_=1,blend_=0,step_=1./144,lastBeat_=0;
 bool held_=false,wasPlaying_=false;
 int fade_=0,fadeSamples_=144;
 float previous_[2]{},from_[2]{};
 void fade(){fade_=fadeSamples_;from_[0]=previous_[0];from_[1]=previous_[1];}
public:
 void prepare(double rate){sr_=std::max(8000.,rate);for(auto& r:ring_)r.assign(size_t(std::ceil(sr_*24))+2,0);fadeSamples_=std::max(8,int(sr_*.003));step_=1./fadeSamples_;tempo_=selected_=0;reset();}
 void reset(){write_=valid_=start_=length_=0;position_=blend_=0;held_=wasPlaying_=false;fade_=0;previous_[0]=previous_[1]=0;}
 void transportReset(){if(!held_)reset();}
 bool active()const{return held_&&blend_>0;}
 size_t allocatedBytes()const{return (ring_[0].capacity()+ring_[1].capacity())*sizeof(float);}
 void process(float& l,float& r,bool request,double beats,bool enabled,double beat,double tempo,bool playing){
  if(ring_[0].empty())return;
  if(!std::isfinite(beat)){beat=0;playing=false;}
  tempo=std::isfinite(tempo)?std::clamp(tempo,20.,400.):120.;beats=std::clamp(beats,1.,8.);
  if((playing!=wasPlaying_)||(playing&&(beat<lastBeat_-1e-6||beat-lastBeat_>4*tempo/(60*sr_))))transportReset();
  wasPlaying_=playing;lastBeat_=beat;
  if(tempo!=tempo_||beats!=selected_){tempo_=tempo;selected_=beats;needed_=std::clamp(size_t(std::round(sr_*60./tempo*beats)),size_t(2),ring_[0].size()-2);if(held_)advance_=double(length_)*tempo/(60*sr_*heldBeats_);}
  const float input[2]={l,r};
  if(!held_){
   if(request&&enabled&&playing&&valid_>=needed_){length_=needed_;start_=(write_+ring_[0].size()-length_)%ring_[0].size();heldBeats_=beats;position_=0;advance_=double(length_)*tempo/(60*sr_*beats);held_=true;fade();}
   if(!held_&&enabled&&playing){ring_[0][write_]=l;ring_[1][write_]=r;if(++write_==ring_[0].size())write_=0;valid_=std::min(valid_+1,ring_[0].size()-2);}
   else if(!enabled||!playing)valid_=0;
  }
  const double target=held_&&request&&enabled&&heldBeats_==beats?1.:0.;
  blend_+=std::clamp(target-blend_,-step_,step_);
  if(held_){
   size_t offset=size_t(position_);double fraction=position_-offset;
   size_t a=(start_+offset)%ring_[0].size(),b=(start_+(offset+1)%length_)%ring_[0].size();
   double t=fade_?1.-double(fade_)/fadeSamples_:1.;t=t*t*(3-2*t);
   for(int ch=0;ch<2;++ch){double wet=ring_[ch][a]+(ring_[ch][b]-ring_[ch][a])*fraction;if(fade_)wet=from_[ch]+(wet-from_[ch])*t;previous_[ch]=float(wet);float out=float(input[ch]+blend_*(wet-input[ch]));if(ch==0)l=out;else r=out;}
   if(fade_)--fade_;position_+=advance_;if(position_>=length_){position_=std::fmod(position_,double(length_));fade();}
   if(target==0&&blend_==0){held_=false;valid_=0;}
  }else{previous_[0]=l;previous_[1]=r;}
 }
};
}
