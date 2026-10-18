// c++ -std=c++17 -O3 -Isrc tools/benchmark_quality.cpp -o benchmark-quality
#include "parameter_settings.h"
#include "factory_presets.h"
#include <ctime>
#include <iostream>
#include <iomanip>
int main(){
 for(int scenario=0;scenario<2;++scenario)for(int tier=0;tier<3;++tier){
  auto p=aztec::initialParameters();p[aztec::kAudioQuality]=tier*.5;p[aztec::kDensity]=1.;p[aztec::kSize]=1.;p[aztec::kRoutingOrder]=1./120.;
  if(scenario){p[aztec::kMasterFilter]=1.;p[aztec::kFilterModel]=15./18.;p[aztec::lfoID(0,aztec::lEnabled)]=1.;p[aztec::slotTarget(0,0)]=9./qg::modTargetCount;p[aztec::slotAmount(0,0)]=.8;}
  qg::Engine e;e.prepare(48000);double sum=0;const auto begin=std::clock();
  for(int n=0;n<480000;++n){if(n%256==0)e.set(aztec::settings(p,180,48000));float l,r;const float x=.2*std::sin(n*.0072);e.process(x,x,n/16000.,l,r);sum+=l*l+r*r;}
  std::cout<<"scenario="<<(scenario?"dense-grains-phaser":"dense-grains")<<" quality="<<tier<<" cpu_one_core_pct="<<std::fixed<<std::setprecision(3)<<100.*(std::clock()-begin)/CLOCKS_PER_SEC/10.<<" energy="<<sum<<'\n';
 }
}
