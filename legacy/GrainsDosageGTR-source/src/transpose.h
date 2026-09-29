#pragma once
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>
namespace qg {
// Two overlapping, windowed variable delay heads; independent of host tempo.
class Transpose {
  std::array<std::vector<float>,2> ring_;
  int64_t write_=0; double phase_=0.,window_=4096.,blend_=0.,smooth_=0.,pitch_=0.,pitchSmoothing_=0.;
  double sample(int ch,double delay) const {
    double pos=double(write_)-delay,n=ring_[ch].size();
    pos=std::fmod(pos,n); if(pos<0.) pos+=n;
    auto i=size_t(pos); double f=pos-i;
    return ring_[ch][i]*(1.-f)+ring_[ch][(i+1)%ring_[ch].size()]*f;
  }
public:
  void prepare(double sr) { window_=sr*.10; for(auto& b:ring_) b.assign(size_t(window_+8),0.f); write_=0; phase_=blend_=pitch_=0.;pitchSmoothing_=std::exp(-1./(.025*sr)); smooth_=std::exp(-1./(.01*sr)); }
  void reset() { phase_=blend_=pitch_=0.; }
  void process(float& l,float& r,double semitones) {
    if(ring_[0].empty()) return;
    auto i=size_t(write_%int64_t(ring_[0].size())); ring_[0][i]=l; ring_[1][i]=r;
    // Slew the pitch before changing delay-head speed; preserve head phase.
    const double targetPitch=std::clamp(semitones,-48.,48.);
    pitch_=targetPitch+pitchSmoothing_*(pitch_-targetPitch);
    double ratio=std::pow(2.,pitch_/12.);
    double target=std::abs(pitch_)>.001 ? 1. : 0.; blend_=target+smooth_*(blend_-target);
    double p=phase_,q=std::fmod(p+.5,1.);
    double w=.5-.5*std::cos(6.283185307179586*p);
    double out[2];
    for(int ch=0;ch<2;++ch) out[ch]=sample(ch,2.+p*window_)*w+sample(ch,2.+q*window_)*(1.-w);
    phase_+=(1.-ratio)/window_; phase_-=std::floor(phase_);
    if(blend_>1e-8) { l=float(l*(1.-blend_)+out[0]*blend_); r=float(r*(1.-blend_)+out[1]*blend_); }
    ++write_;
  }
};
}
