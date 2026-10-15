#include "src/randomize.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
  for(int stage=0;stage<3;++stage){std::array<double,aztec::kCount> p;p.fill(.12345);uint32_t seed=1234;
    for(int run=0;run<100;++run){aztec::randomizeModule(stage,7,seed,[&](aztec::ParamID id,double v){assert(std::isfinite(v)&&v>=0&&v<=1);p[id]=v;});}
    assert(p[aztec::kMix]==.12345&&p[aztec::kModuleOrder]==.12345&&p[aztec::kRepeatOn]==.12345);
    if(stage==0){assert(p[aztec::kGrainPan]==.5&&p[aztec::kPanMode]==0);assert(p[aztec::kTranspose]==.12345&&p[aztec::kStretchSpeed]==.12345);}
    if(stage==2){for(int i=0;i<16;++i){double rate=p[aztec::kRepeatRate0+i];assert(rate!=.12345&&std::abs(rate*15-std::round(rate*15))<1e-9);}}
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
  for(bool locked:{false,true}){
    std::array<double,aztec::kCount> p;p.fill(.12345);p[aztec::kResliceMix]=.271;p[aztec::kMixLock0+3]=locked?1:0;
    auto get=[&](aztec::ParamID id){return p[id];};uint32_t seed=0;
    for(int run=0;run<100;++run){std::array<bool,aztec::kCount> touched{};
      aztec::randomizeReslice(seed,get,[&](aztec::ParamID id,double v){
        assert((id>=aztec::kResliceAlgorithm&&id<=aztec::kResliceMaxDelay)||id==aztec::kResliceLength||id==aztec::kResliceMix||id==aztec::kModuleLoopOn0+2||id==aztec::kModuleLoopWindow0+2);
        assert(std::isfinite(v)&&v>=0&&v<=1);touched[id]=true;p[id]=v;
      });
      for(const auto& spec:aztec::resliceControlSpecs){assert(touched[spec.id]);if(spec.steps)assert(std::abs(p[spec.id]*spec.steps-std::round(p[spec.id]*spec.steps))<1e-8);}
      assert(touched[aztec::kResliceMix]==!locked);
      assert(p[aztec::kResliceEnabled]==.12345&&p[aztec::kMixLock0+3]==(locked?1:0));
      assert(p[aztec::kResliceMinAmp]<=p[aztec::kResliceMaxAmp]);
      assert(1+std::round(p[aztec::kResliceMinPhrase]*7)<=(1<<int(std::round(p[aztec::kReslicePhrase]*3))));
      if(locked)assert(p[aztec::kResliceMix]==.271);
    }if(!locked)assert(p[aztec::kResliceMix]!=.271);
  }
  std::cout<<"PASS: bounded module-specific random, preserving other modules and transport controls\n";
}

