#pragma once
#include <cmath>
#include <cstdint>
#include <limits>
namespace qg {
// Musical positions are quarter-note beats. No wall clock or GUI timer.
class QuantizedTrigger {
  int64_t cycle_=INT64_MIN;
  double period_=0., lastBeat_=0.;
public:
  void reset(){cycle_=INT64_MIN;period_=0.;}
  bool tick(double beat,double period,bool running) {
    if(!running||!std::isfinite(beat)||!std::isfinite(period)||period<=0.){reset();return false;}
    const auto cycle=int64_t(std::floor(beat/period+1e-8));
    const bool continuous=cycle_!=INT64_MIN&&period==period_&&beat>=lastBeat_&&beat-lastBeat_<period;
    const bool boundary=std::abs(beat-double(cycle)*period)<1e-7;
    const bool fire=(continuous&&cycle!=cycle_)||(!continuous&&boundary);
    cycle_=cycle;period_=period;lastBeat_=beat;return fire;
  }
};
}
