#pragma once
#include <algorithm>
#include <cmath>
namespace qg {
// Stereo-linked RMS matching. Freeze during silence to preserve effect tails.
class LevelMatch {
 double a_=0.,smooth_=0.,dry_=0.,wet_=0.,gain_=1.;
public:
 void prepare(double sr){a_=std::exp(-1./(.25*sr));smooth_=std::exp(-1./(.15*sr));reset();}
 void reset(){dry_=wet_=0.;gain_=1.;}
 void process(double dl,double dr,double& wl,double& wr,bool on){
  dry_=a_*dry_+(1.-a_)*.5*(dl*dl+dr*dr);wet_=a_*wet_+(1.-a_)*.5*(wl*wl+wr*wr);
  double target=1.;if(on){target=gain_;if(dl*dl+dr*dr>1e-10&&dry_>1e-8&&wet_>1e-8)target=std::clamp(std::sqrt(dry_/wet_),.25,4.);}
  gain_=target+smooth_*(gain_-target);wl*=gain_;wr*=gain_;
 }
};
}
