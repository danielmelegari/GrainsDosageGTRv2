#pragma once
#include <cmath>

namespace aztec {
// Track the host timeline separately from the free-running live-input clock.
// A stopped host repeats its position; that is not a seek on every block.
class HostTransport {
  bool valid_=false, playing_=false;
  double expected_=0.;
public:
  void reset(){valid_=false;playing_=false;expected_=0.;}
  bool update(double& renderBeat,double hostBeat,bool valid,bool playing,int samples,double increment){
    if(samples<=0)return false; // automation-only flush cannot move transport
    if(!valid){valid_=false;return false;}
    const bool discontinuity=std::abs(hostBeat-(valid_?expected_:renderBeat))>2.*increment;
    const bool transition=valid_&&playing!=playing_;
    if(!valid_||playing||discontinuity||transition)renderBeat=hostBeat;
    expected_=hostBeat+(playing?samples*increment:0.);
    valid_=true;playing_=playing;
    return discontinuity||transition;
  }
};
}
