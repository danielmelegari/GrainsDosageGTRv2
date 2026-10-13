// Build: c++ -std=c++17 -O3 -Isrc tools/benchmark_engine.cpp -o benchmark-engine
// Override the include path to compare an earlier source tree on the same machine.
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include "parameter_settings.h"
#include "factory_presets.h"
#include <ctime>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <stdexcept>

static double cpuSeconds(){
#if defined(_WIN32)
 FILETIME created,exited,kernel,user;
 if(!GetProcessTimes(GetCurrentProcess(),&created,&exited,&kernel,&user))
  throw std::runtime_error("GetProcessTimes failed");
 auto ticks=[](FILETIME t){return (uint64_t(t.dwHighDateTime)<<32)|t.dwLowDateTime;};
 return double(ticks(kernel)+ticks(user))*1e-7;
#else
 return double(std::clock())/CLOCKS_PER_SEC;
#endif
}
int main(){
 for(int preset: {-1,0,1,8,24,25,29,100}) {
  auto p=preset<0?aztec::initialParameters():aztec::legacyFactoryPreset(preset==100?29:preset);
  if(preset==100)p[aztec::slotTarget(0,0)]=22./qg::modTargetCount; // Extended Reslice Mix route.
  qg::Engine e;e.prepare(48000);uint64_t hash=14695981039346656037ULL;
  const double start=cpuSeconds();
  for(int n=0;n<480000;++n){
   if(n%256==0)e.set(aztec::settings(p,120,48000));
   float l,r;e.process(float(n%997)*.0001f,.1f,n/24000.,l,r);
   for(float v:{l,r}){uint32_t bits;std::memcpy(&bits,&v,sizeof(bits));hash=(hash^bits)*1099511628211ULL;}
  }
  std::cout<<"preset="<<preset<<" cpu_one_core_pct="<<std::fixed<<std::setprecision(3)<<100.*(cpuSeconds()-start)/10<<" audio_hash="<<hash<<'\n';
 }
}
