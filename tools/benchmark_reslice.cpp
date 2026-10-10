// Standalone CPU benchmark. Build -O3; override RESLICE_HEADER for a baseline.
#ifndef RESLICE_HEADER
#define RESLICE_HEADER "../src/reslice.h"
#endif
#include RESLICE_HEADER
#include <chrono>
#include <ctime>
#include <iostream>
#include <vector>
int main(){constexpr double sr=48000,seconds=30;volatile float sink=0;
 for(int block:{64,256,1024})for(int mode=-1;mode<6;++mode){
#ifdef BASELINE_RESLICE
 if(mode>=3)continue;
#endif
 qg::Reslice e;e.prepare(sr);qg::ResliceSettings s;s.enabled=mode>=0;s.algorithm=std::max(mode,0)%3;s.variation=.8;s.fill=.8;s.reverse=.3;
#ifndef BASELINE_RESLICE
 s.crusher=s.comb=mode>=3;s.minBits=4;s.maxBits=16;s.minFreq=4000;s.maxFreq=24000;s.combFeedback=.9;
#endif
 e.set(s);std::vector<double> times;times.reserve(int(sr*seconds/block)+1);int count=0;const auto cpuStart=std::clock();
 while(count<int(sr*seconds)){auto t=std::chrono::steady_clock::now();for(int j=0;j<block;++j,++count){float l=float(count%997)/9970.f,r=-l;e.process(l,r,count/24000.,120);sink=l;}times.push_back(std::chrono::duration<double>(std::chrono::steady_clock::now()-t).count());}
 double total=double(std::clock()-cpuStart)/CLOCKS_PER_SEC;std::sort(times.begin(),times.end());std::cout<<"block="<<block<<" mode="<<mode<<" cpu_one_core_pct="<<100*total/(count/sr)<<" p95_deadline_pct="<<100*times[size_t(times.size()*.95)]/(block/sr)<<" memory_MiB="<<e.allocatedBytes()/1048576.<<'\n';
 }return sink==12345;
}
