#pragma once
#include <array>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace qg {
// Streaming granular overlap-add. Every grain reads at unity speed, preserving
// local pitch; grain origins advance at the requested time ratio. A finite live
// history must reseed when playback catches the writer or exhausts old samples.
class FreeStretch {
  struct Grain { double start=0.; int age=0; bool active=false; };
  std::array<std::vector<float>,2> history_;
  std::array<Grain,4> grains_{};
  int64_t write_=0;
  int length_=2048,hop_=512,tick_=0;
  double cursor_=0., blend_=0., smoothing_=0.;
  bool running_=false;
  double read(int channel,double position) const {
    if(position<0. || position>=write_ || position<double(write_)-history_[0].size()+2.) return 0.;
    auto i=int64_t(position); double f=position-i;
    const auto& b=history_[channel];
    return b[size_t(i%int64_t(b.size()))]*(1.-f)+b[size_t((i+1)%int64_t(b.size()))]*f;
  }
public:
  void prepare(double rate) {
    for(auto& b:history_) b.assign(size_t(rate*16.),0.f);
    length_=std::max(64,int(rate*.08)); length_-=length_%4; hop_=length_/4;
    smoothing_=std::exp(-1./(.01*rate)); write_=0; reset();
  }
  void reset() { grains_={}; tick_=0; running_=false; blend_=0.; }
  void process(float& left,float& right,bool enabled,double speed) {
    if(history_[0].empty()) return;
    auto index=size_t(write_%int64_t(history_[0].size()));
    history_[0][index]=left; history_[1][index]=right; ++write_;
    speed=std::clamp(speed,.25,4.);
    const bool active=enabled && std::abs(speed-1.)>1e-6;
    if(active && !running_) { cursor_=double(write_)-length_*4.; tick_=0; grains_={}; running_=true; }
    if(running_ && tick_==0) {
      if(cursor_<double(write_)-history_[0].size()+length_ || cursor_+length_>=write_)
        cursor_=double(write_)-length_*4.;
      for(auto& g:grains_) if(!g.active) { g={cursor_,0,true}; break; }
      cursor_+=speed*hop_;
    }
    double sum[2]={},weight=0.;
    for(auto& g:grains_) if(g.active) {
      double window=.5-.5*std::cos(6.283185307179586*g.age/length_);
      for(int ch=0;ch<2;++ch) sum[ch]+=read(ch,g.start+g.age)*window;
      weight+=window; if(++g.age>=length_) g.active=false;
    }
    if(running_) tick_=(tick_+1)%hop_;
    const double target=active && write_>length_*4 ? 1. : 0.;
    blend_=target+smoothing_*(blend_-target);
    if(!active && blend_<1e-8) { blend_=0.; running_=false; }
    if(blend_>0.) {
      left=float(left*(1.-blend_)+sum[0]/std::max(1e-9,weight)*blend_);
      right=float(right*(1.-blend_)+sum[1]/std::max(1e-9,weight)*blend_);
    }
  }
};
}
