#pragma once
#include <algorithm>
#include <cmath>
namespace qg {
// TPT state-variable filter, followed by a stereo-linked sample-peak limiter.
// No lookahead or latency; selectable ceiling. Not an oversampled true-peak limiter.
class MasterFx {
  double sr_=48000.,smooth_=0.,release_=0.,g_=0.,targetG_=0.,k_=1.,targetK_=1.,blend_=0.,gain_=1.;
  double ic1_[2][2]={},ic2_[2][2]={},drive_=0.,driveTarget_=0.,slope_=0.,slopeTarget_=0.,mode_[3]={1.,0.,0.};
  float limit_=1.f;bool enabled_=false,limiter_=false;int type_=0;
public:
  static constexpr double ceiling=1.;
  void prepare(double sr){sr_=std::max(8000.,sr);smooth_=std::exp(-1./(.005*sr_));release_=std::exp(-1./(.080*sr_));reset();}
  void reset(){for(int s=0;s<2;++s)for(int c=0;c<2;++c)ic1_[s][c]=ic2_[s][c]=0.;drive_=driveTarget_;slope_=slopeTarget_;blend_=0.;gain_=1.;g_=targetG_;k_=targetK_;}
  void set(bool filter,int type,double cutoff,double resonance,bool limiter,int slope=0,double drive=0.,double ceilingDb=0.){
    const double amplitude=std::pow(10.,std::clamp(ceilingDb,-10.,0.)/20.);
    limit_=float(amplitude);if(double(limit_)>amplitude)limit_=std::nextafter(limit_,0.f);
    slopeTarget_=slope?1.:0.;driveTarget_=std::clamp(drive,0.,24.);
    enabled_=filter;type_=std::clamp(type,0,2);limiter_=limiter;
    targetG_=std::tan(3.14159265358979323846*std::clamp(cutoff,20.,std::min(20000.,sr_*.45))/sr_);
    targetK_=1./(.5+11.5*std::clamp(resonance,0.,1.));
  }
  void process(float& left,float& right){
    g_=targetG_+smooth_*(g_-targetG_);k_=targetK_+smooth_*(k_-targetK_);
    blend_=(enabled_?1.:0.)+smooth_*(blend_-(enabled_?1.:0.));
    for(int i=0;i<3;++i)mode_[i]=(i==type_?1.:0.)+smooth_*(mode_[i]-(i==type_?1.:0.));
    drive_=driveTarget_+smooth_*(drive_-driveTarget_);slope_=slopeTarget_+smooth_*(slope_-slopeTarget_);
    double a1=1./(1.+g_*(g_+k_)),a2=g_*a1,a3=g_*a2;
    double samples[2]={left,right};
    for(int ch=0;ch<2;++ch){
      double input=samples[ch];
      if(drive_>1e-6){double d=std::pow(10.,drive_/20.);input=std::tanh(input*d)/std::tanh(d);}
      double first=0.,filtered=0.;
      for(int stage=0;stage<2;++stage){
        double v3=input-ic2_[stage][ch],v1=a1*ic1_[stage][ch]+a2*v3,v2=ic2_[stage][ch]+a2*ic1_[stage][ch]+a3*v3;
        ic1_[stage][ch]=2.*v1-ic1_[stage][ch];ic2_[stage][ch]=2.*v2-ic2_[stage][ch];
        if(std::abs(ic1_[stage][ch])<1e-20)ic1_[stage][ch]=0.;if(std::abs(ic2_[stage][ch])<1e-20)ic2_[stage][ch]=0.;
        filtered=mode_[0]*v2+mode_[1]*(input-k_*v1-v2)+mode_[2]*k_*v1;
        if(stage==0)first=filtered;input=filtered;
      }
      filtered=first+slope_*(filtered-first);
      if(blend_>1e-8)samples[ch]+=blend_*(filtered-samples[ch]);
    }
    if(limiter_)for(auto& sample:samples)if(!std::isfinite(sample))sample=0.;
    double peak=std::max(std::abs(samples[0]),std::abs(samples[1]));
    double desired=peak>limit_?double(limit_)/peak:1.;
    gain_=desired<gain_?desired:desired+release_*(gain_-desired);
    if(limiter_){samples[0]*=gain_;samples[1]*=gain_;}
    left=float(samples[0]);right=float(samples[1]);
    // Final sample guard also covers floating-point rounding and ceiling automation.
    if(limiter_){left=std::clamp(left,-limit_,limit_);right=std::clamp(right,-limit_,limit_);}
  }
};
}
