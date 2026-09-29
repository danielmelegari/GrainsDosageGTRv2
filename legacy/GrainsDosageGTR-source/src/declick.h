#pragma once
#include <array>
#include <algorithm>
#include <cmath>
namespace qg {
// Short impulsive damage repair. Fixed lookahead is also present when disabled.
// A discontinuity must return to the local trajectory within 16 samples;
// sustained edges and longer musical transients are deliberately left alone.
class InputDeclick {
 struct Channel {std::array<double,128> raw{},work{};double slope=0.,previous=0.;};
 std::array<Channel,2> channels_{};int write_=0;double blend_=0.,slew_=1./480.;
 static int index(int n){return n&127;}
 double repair(Channel& c,int at,double sensitivity){
  const double before=c.work[index(at-1)],prior=c.work[index(at-2)];
  const double slope=before-prior;
  const double threshold=std::max(.015+.105*(1.-sensitivity),(5.+13.*(1.-sensitivity))*c.slope);
  const double jump=c.work[index(at)]-before;
  if(std::abs(jump-slope)>threshold){
   for(int length=1;length<=16;++length){
    double end=c.work[index(at+length)],last=c.work[index(at+length-1)],after=c.work[index(at+length+1)];
    const double fall=end-last;
    // Require opposite abrupt edges and locally consistent clean endpoints.
    if(jump*fall<0.&&std::abs(fall)>threshold&&std::abs(end-(before+slope*(length+1)))<threshold*.6&&std::abs((after-end)-slope)<threshold*.4){
     for(int j=0;j<length;++j)c.work[index(at+j)]=before+(end-before)*double(j+1)/double(length+1);
     break;
    }
   }
  }
  double cleaned=c.work[index(at)],delta=std::abs(cleaned-c.previous);c.previous=cleaned;
  c.slope+=.02*(delta-c.slope);
  return cleaned;
 }
public:
 static constexpr unsigned latency=32;
 void prepare(double rate,bool enabled){channels_={};write_=0;blend_=enabled?1.:0.;slew_=1./std::max(1.,rate*.01);}
 void process(double& l,double& r,bool enabled,double sensitivity,bool bypass=false){
  sensitivity=std::clamp(sensitivity,0.,1.);double input[2]={l,r},output[2]{};
  blend_+=std::clamp((enabled?1.:0.)-blend_,-slew_,slew_);
  for(int ch=0;ch<2;++ch){auto& c=channels_[ch];c.raw[write_]=c.work[write_]=input[ch];int at=index(write_-int(latency));double dry=c.raw[at],clean=repair(c,at,sensitivity);output[ch]=bypass?dry:dry+blend_*(clean-dry);}
  write_=index(write_+1);l=output[0];r=output[1];
 }
};
}
