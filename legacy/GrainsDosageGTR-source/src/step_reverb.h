#pragma once
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>
namespace qg {
// Eight-line orthogonal feedback delay network. Sequencer gates the send,
// never the return, so the decay continues across unlit steps.
class StepReverb {
 struct Line{std::vector<double>b;size_t pos=0;double low=0.;};
 std::array<Line,8> lines_;std::array<double,8> feedback_{};
 void coefficients(){const double rt[]={.45,1.25,2.8,5.5,9.};for(int i=0;i<8;++i)feedback_[i]=std::pow(.001,double(lines_[i].b.size())/(sr_*(length_>0.?length_:rt[type_])));}double length_=0.;double sr_=48000.,send_=0.,mix_=0.,smooth_=0.;
 double kill_=0.;bool killTarget_=false;
 bool random_=false,gateOpen_=false;double randomGrid_=.25;
 bool enabled_=false;int type_=0;double grid_=.25,amount_=.25;uint16_t pattern_=0x1111;
public:
 void prepare(double sr){sr_=sr;smooth_=std::exp(-1./(.005*sr));
  const double times[]={.0297,.0371,.0411,.0437,.0531,.0613,.0719,.0797};
  for(int i=0;i<8;++i){lines_[i].b.assign(size_t(times[i]*sr)+1,0.);lines_[i].pos=0;lines_[i].low=0.;}send_=mix_=kill_=0.;gateOpen_=false;coefficients();}
 void reset(){for(auto& l:lines_){std::fill(l.b.begin(),l.b.end(),0.);l.pos=0;l.low=0.;}send_=mix_=kill_=0.;gateOpen_=false;}
 void set(bool on,int type,double grid,uint16_t pattern,double amount,double length=0.,bool kill=false,bool random=false,double randomGrid=.25){random_=random;randomGrid_=std::clamp(randomGrid,.25,1.);killTarget_=kill;length_=std::clamp(length,0.,20.);enabled_=on;type_=std::clamp(type,0,4);grid_=grid;pattern_=pattern;amount_=amount;coefficients();}
 bool gateOpen()const{return gateOpen_;}
 static bool randomGate(int64_t step){uint64_t h=uint64_t(step)+0x9e3779b97f4a7c15ULL+0x726576657262ULL;h=(h^(h>>30))*0xbf58476d1ce4e5b9ULL;h=(h^(h>>27))*0x94d049bb133111ebULL;return ((h^(h>>31))&1)!=0;}
 double killAmount()const{return kill_;}
 void process(float& left,float& right,double beat){
  int step=int((int64_t(std::floor(beat/grid_+1e-8))%16+16)%16);
  gateOpen_=enabled_&&(random_?randomGate(int64_t(std::floor(beat/randomGrid_+1e-8))):bool(pattern_&(1u<<step)));
  double target=gateOpen_?1.:0.;send_=target+smooth_*(send_-target);
  double mt=enabled_?amount_:0.;mix_=mt+smooth_*(mix_-mt);
  double kt=enabled_&&killTarget_?1.:0.;kill_=kt+smooth_*(kill_-kt);
  const double damp[]={.32,.55,.72,.82,.9};
  double v[8],sum=0.;for(int i=0;i<8;++i){auto& l=lines_[i];double x=l.b[l.pos];l.low=damp[type_]*l.low+(1.-damp[type_])*x;v[i]=l.low;sum+=v[i];}
  double wetL=(v[0]+v[2]-v[4]-v[6])*.5,wetR=(v[1]+v[3]-v[5]-v[7])*.5;
  for(int i=0;i<8;++i){auto& l=lines_[i];double fb=feedback_[i];
   double input=(i%2?right:left)*send_*.35*(i<4?1.:-1.);
   l.b[l.pos]=input+fb*(sum*.25-v[i]);if(std::abs(l.b[l.pos])<1e-20)l.b[l.pos]=0.;l.pos=(l.pos+1)%l.b.size();}
  left=float(left*(1.-kill_)+wetL*mix_);right=float(right*(1.-kill_)+wetR*mix_);
 }
};
}
