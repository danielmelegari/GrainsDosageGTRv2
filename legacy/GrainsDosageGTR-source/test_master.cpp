#include "src/master_fx.h"
#include <cassert>
#include <cmath>
#include <iostream>
double response(int mode,double hz,double resonance=0.){
  qg::MasterFx fx;fx.prepare(48000.);fx.set(true,mode,1000.,resonance,false);double energy=0.;
  for(int i=0;i<48000;++i){float l=float(.1*std::sin(6.283185307179586*hz*i/48000.)),r=l;fx.process(l,r);assert(std::isfinite(l)&&l==r);if(i>24000)energy+=l*l;}
  return std::sqrt(energy/23999.);
}
int main(){
  qg::MasterFx fx;fx.prepare(48000.);fx.set(false,0,1000.,0.,false);
  for(int i=0;i<5000;++i){float l=float(std::sin(i*.1)*3.),r=-.4f*l,a=l,b=r;fx.process(l,r);assert(l==a&&r==b);}
  fx.set(false,0,1000.,0.,true);
  for(int i=0;i<48000;++i){float l=float(4.*std::sin(i*.14)),r=l*.25f;fx.process(l,r);assert(std::abs(l)<=qg::MasterFx::ceiling+1e-6);assert(std::abs(r-l*.25f)<1e-6);}
  fx.set(false,0,1000.,0.,false);float l=2.f,r=-3.f;fx.process(l,r);assert(l==2.f&&r==-3.f);
  assert(response(0,100.)>response(0,10000.)*50.);
  assert(response(1,10000.)>response(1,100.)*50.);
  assert(response(2,1000.)>response(2,50.)*8.);
  assert(response(2,1000.)>response(2,18000.)*8.);
  assert(response(0,1000.,.8)>response(0,1000.,0.)*5.);
  for(double sr:{8000.,44100.,48000.,96000.}){
    fx.prepare(sr);
    for(int i=0;i<100000;++i){if(i%100==0)fx.set(i%2000!=0,(i/100)%3,i%200?20000.:20.,1.,true);
      float a=float(5.*std::sin(i*.23)),b=float(7.*std::cos(i*.37));fx.process(a,b);
      assert(std::isfinite(a)&&std::isfinite(b)&&std::abs(a)<=qg::MasterFx::ceiling+1e-6&&std::abs(b)<=qg::MasterFx::ceiling+1e-6);
    }
  }
  // Strict bound, including the first sample after lowering ceiling or enabling.
  for(double sr:{8000.,44100.,48000.,96000.,192000.}){fx.prepare(sr);for(int i=0;i<30000;++i){double db=(i/137)%3==0?0.:(i/137)%3==1?-6.:-10.;bool on=i%89!=0;fx.set(false,0,1000,0,on,0,0,db);float l=float(12.*std::sin(i*.73)),r=float(9.*std::cos(i*.41)),a=l,b=r;fx.process(l,r);if(on){double bound=std::pow(10.,db/20.);assert(std::abs(double(l))<=bound&&std::abs(double(r))<=bound);}else assert(l==a&&r==b);}}
  std::cout<<"PASS: disabled transparent path; LP/HP/BP response; resonance; stereo-linked selectable sample-peak ceiling; mode/cutoff automation stability\n";
}
