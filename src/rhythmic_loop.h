#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
namespace qg {
// Live input history and a separate held slice. All storage is allocated in
// prepare(); captures copy a bounded slice, never allocate in the audio callback.
class RhythmicLoop {
  std::array<std::vector<float>,2> history_,capture_;
  int64_t written_=0,firstSignal_=-1;
  double capturedLength_=16.,length_=16.,position_=0.,blend_=0.,smooth_=0.,pitch_=0.;
  bool frozen_=false,requested_=false,reverse_=false;
  std::array<float,2> previous_{};
  int transition_=0;
  float read(int ch,double p)const{
    p=std::fmod(p,length_);if(p<0)p+=length_;
    int a=int(p);double f=p-a,fade=std::min(32.,length_*.1);
    double env=std::min(1.,std::min(p,length_-p)/fade);
    return float((capture_[ch][a]*(1.-f)+capture_[ch][(a+1)%int(length_)]*f)*env);
  }
  void capture(double samples,double offset){
    capturedLength_=length_=samples;int64_t available=std::min(written_,int64_t(history_[0].size()));
    int64_t start=written_-int64_t(length_)-int64_t(std::clamp(std::round(offset),0.,double(available)-length_));
    size_t index=size_t(start%int64_t(history_[0].size()));size_t n=size_t(length_);
    size_t first=std::min(n,history_[0].size()-index);
    for(int ch=0;ch<2;++ch){std::copy_n(history_[ch].data()+index,first,capture_[ch].data());std::copy_n(history_[ch].data(),n-first,capture_[ch].data()+first);}
    position_=0.;frozen_=true;transition_=64;
  }
public:
  void prepare(double rate){for(auto& b:history_)b.assign(size_t(rate*16.),0.f);for(auto& b:capture_)b.assign(size_t(rate*16.),0.f);smooth_=std::exp(-1./(.002*rate));reset();}
  void reset(){written_=0;firstSignal_=-1;frozen_=requested_=false;position_=blend_=0.;previous_={};transition_=0;}
  bool active()const{return frozen_&&blend_>.01;}
  void process(float& l,float& r,bool request,bool audible,double samples,double pitch,bool reverse,double mix,double offset=0.,bool recapture=false){
    if(history_[0].empty())return;
    size_t index=size_t(written_%int64_t(history_[0].size()));history_[0][index]=l;history_[1][index]=r;++written_;
    if(firstSignal_<0&&(std::abs(l)>1e-7||std::abs(r)>1e-7))firstSignal_=written_;
    samples=std::clamp(std::round(samples),16.,double(history_[0].size()-2));
    if(!request){frozen_=false;requested_=false;}
    if(request){
      bool needs=!frozen_||!requested_||recapture;
      if(needs&&firstSignal_>=0&&written_-firstSignal_>=samples)capture(samples,offset);
      if(frozen_&&!needs){double next=std::min(samples,capturedLength_);if(next!=length_){position_=position_/length_*next;length_=next;transition_=64;}}
      requested_=true;
    }
    if(frozen_&&(reverse!=reverse_||pitch!=pitch_))transition_=64;
    reverse_=reverse;pitch_=pitch;
    double target=frozen_&&audible?std::clamp(mix,0.,1.):0.;blend_=target+smooth_*(blend_-target);
    if(frozen_){
      float wet[2];for(int ch=0;ch<2;++ch){wet[ch]=read(ch,reverse?length_-1.-position_:position_);if(transition_>0){double t=1.-transition_/64.;wet[ch]=float(previous_[ch]*(1.-t)+wet[ch]*t);}}
      if(transition_>0)--transition_;previous_={wet[0],wet[1]};
      l=float(l*(1.-blend_)+wet[0]*blend_);r=float(r*(1.-blend_)+wet[1]*blend_);
      position_=std::fmod(position_+std::pow(2.,std::clamp(pitch,-48.,48.)/12.),length_);
    }else if(blend_>1e-8){l=float(l*(1.-blend_)+previous_[0]*blend_);r=float(r*(1.-blend_)+previous_[1]*blend_);}
  }
};
}
