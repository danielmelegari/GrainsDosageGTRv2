#pragma once
#include "quantized_trigger.h"
#include <array>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace qg {
struct ResliceSettings {bool enabled=false,random=false;double randomBeats=4.;double beats=4.,mix=1.;std::array<bool,16> on{{true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true}};std::array<int,16> slice{{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}};};
class Reslice {
 std::array<std::vector<float>,2> buffer_;ResliceSettings s_;double sr_=48000.,lastBeat_=0.,tempo_=120.,lastLength_=4.,anchor_=0.,length_=0.,mix_=0.;int64_t write_=0,lastTick_=INT64_MIN,lastCycle_=INT64_MIN;int step_=0,fade_=0,fadeSize_=144;bool wasPlaying_=true,active_=false;double from_[2]{},last_[2]{};
 QuantizedTrigger randomClock_;std::array<int,16> pattern_{};uint32_t seed_=0x6149acb3;bool randomReady_=false;
 int randomIndex(int previous){seed_^=seed_<<13;seed_^=seed_>>17;seed_^=seed_<<5;return (previous+1+int(seed_%15))%16;}
 double read(int ch,double pos)const{int64_t n=int64_t(std::floor(pos));if(n<0||n<write_-int64_t(buffer_[ch].size())||n+1>=write_)return 0;size_t a=size_t(n)%buffer_[ch].size(),b=(a+1)%buffer_[ch].size();return buffer_[ch][a]+(buffer_[ch][b]-buffer_[ch][a])*(pos-n);}
public:
 void prepare(double sr){sr_=std::max(8000.,sr);for(auto& b:buffer_)b.assign(size_t(sr_*96.)+64,0.f);fadeSize_=std::max(1,int(.003*sr_));write_=0;last_[0]=last_[1]=from_[0]=from_[1]=0;mix_=0;reset();}
 void reset(){randomClock_.reset();randomReady_=false;write_=0;lastTick_=lastCycle_=INT64_MIN;active_=false;length_=0;fade_=fadeSize_;from_[0]=last_[0];from_[1]=last_[1];}
 void set(const ResliceSettings& s){if(s.random!=s_.random){randomReady_=false;randomClock_.reset();}s_=s;}
 int source(int i)const{return s_.random&&randomReady_?pattern_[i]:s_.slice[i];}
 void process(float& l,float& r,double beat,double tempo,bool playing=true){
  bool jump=lastTick_!=INT64_MIN&&(beat<lastBeat_-1e-6||beat-lastBeat_>tempo/(60*sr_)*4.);if(jump||playing!=wasPlaying_)reset();wasPlaying_=playing;lastBeat_=beat;
  bool randomized=false;
  if(!randomReady_){pattern_=s_.slice;randomReady_=true;}
  if(randomClock_.tick(beat,s_.randomBeats,s_.random&&s_.enabled&&playing)){
    for(int i=0;i<16;++i)pattern_[i]=randomIndex(pattern_[i]);randomized=true;
  }
  const double source[2]={l,r};for(int c=0;c<2;++c)buffer_[c][size_t(write_)%buffer_[c].size()]=float(source[c]);++write_;
  double beats=std::max(.125,s_.beats),position=beat/(beats/16.);int64_t tick=int64_t(std::floor(position+1e-9)),cycle=int64_t(std::floor(position/16.));step_=int((tick%16+16)%16);
  bool changed=std::abs(tempo-tempo_)>1e-8||beats!=lastLength_;tempo_=tempo;lastLength_=beats;
  if(cycle!=lastCycle_||changed){double wanted=beats*60./std::clamp(tempo,20.,400.)*sr_;if(write_>wanted){length_=wanted;anchor_=write_-1-wanted;}else length_=0;lastCycle_=cycle;}
  bool next=s_.enabled&&playing&&s_.on[step_]&&length_>32.;
  if((tick!=lastTick_&&(next||active_))||next!=active_||((changed||randomized)&&next)){from_[0]=last_[0];from_[1]=last_[1];fade_=fadeSize_;lastTick_=tick;active_=next;}lastTick_=tick;
  double target=next?std::clamp(s_.mix,0.,1.):0.;mix_+=std::clamp(target-mix_,-1./fadeSize_,1./fadeSize_);
  for(int c=0;c<2;++c){double wet=source[c];if(length_>32){double span=length_/16.,phase=std::clamp(position-std::floor(position),0.,1.);double pos=anchor_+std::clamp(this->source(step_),0,15)*span+phase*std::max(1.,span-2.);wet=read(c,pos);}double out=source[c]+mix_*(wet-source[c]);if(fade_&&s_.enabled){double t=1.-double(fade_)/fadeSize_;t=t*t*(3.-2*t);out=from_[c]+t*(out-from_[c]);}last_[c]=out;if(c==0)l=float(out);else r=float(out);}
  if(fade_)--fade_;
 }
 int step()const{return step_;}bool active()const{return active_;}
};
}

