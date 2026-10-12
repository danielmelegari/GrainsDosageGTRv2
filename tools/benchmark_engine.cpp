// Build: c++ -std=c++17 -O3 -Isrc tools/benchmark_engine.cpp -o benchmark-engine
// Override the include path to compare an earlier source tree on the same machine.
#include "parameter_settings.h"
#include "factory_presets.h"
#include <ctime>
#include <cstring>
#include <iomanip>
#include <iostream>
int main(){
 for(int preset: {-1,0,1,8,24,25,29}) {
  auto p=preset<0?aztec::initialParameters():aztec::legacyFactoryPreset(preset);
  qg::Engine e;e.prepare(48000);uint64_t hash=14695981039346656037ULL;
  const auto start=std::clock();
  for(int n=0;n<480000;++n){
   if(n%256==0)e.set(aztec::settings(p,120,48000));
   float l,r;e.process(float(n%997)*.0001f,.1f,n/24000.,l,r);
   for(float v:{l,r}){uint32_t bits;std::memcpy(&bits,&v,sizeof(bits));hash=(hash^bits)*1099511628211ULL;}
  }
  std::cout<<"preset="<<preset<<" cpu_one_core_pct="<<std::fixed<<std::setprecision(3)<<100.*double(std::clock()-start)/CLOCKS_PER_SEC/10<<" audio_hash="<<hash<<'\n';
 }
}
