#include "src/randomize.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
  for(int stage=0;stage<3;++stage){std::array<double,aztec::kCount> p;p.fill(.12345);uint32_t seed=1234;
    for(int run=0;run<100;++run){aztec::randomizeModule(stage,7,seed,[&](aztec::ParamID id,double v){assert(std::isfinite(v)&&v>=0&&v<=1);p[id]=v;});}
    assert(p[aztec::kMix]==.12345&&p[aztec::kModuleOrder]==.12345&&p[aztec::kRepeatOn]==.12345);
    if(stage==0)assert(p[aztec::kSize]!=.12345&&p[aztec::kGlitchMix]==.12345&&p[aztec::kRepeatMix]==.12345);
    if(stage==1)assert(p[aztec::kGlitchMix]!=.12345&&p[aztec::kSize]==.12345&&p[aztec::kRepeatMix]==.12345);
    if(stage==2)assert(p[aztec::kRepeatMix]!=.12345&&p[aztec::kSize]==.12345&&p[aztec::kGlitchMix]==.12345);
  }
  for(int stage=0;stage<3;++stage){std::array<double,aztec::kCount> p{};p[aztec::kMixLock0+stage]=1;p[aztec::lockableMixes[stage]]=.271;
    auto get=[&](aztec::ParamID id){return p[id];};auto set=[&](aztec::ParamID id,double v){p[id]=v;};uint32_t seed=317;
    for(int n=0;n<100;++n)aztec::randomizeModule(stage,3,seed,get,set);
    assert(p[aztec::lockableMixes[stage]]==.271);p[aztec::kMixLock0+stage]=0;aztec::randomizeModule(stage,3,seed,get,set);assert(p[aztec::lockableMixes[stage]]!=.271);
  }
  for(int i=0;i<6;++i){assert(aztec::mixLockFor(aztec::lockableMixes[i])==int(aztec::kMixLock0)+i);}
  std::cout<<"PASS: bounded module-specific random, preserving other modules and transport controls\n";
}

