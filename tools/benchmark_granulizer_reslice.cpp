// Build with -O3 -std=c++17 -Isrc. Compare before/after on the same machine.
#include "engine.h"
#include <chrono>
#include <ctime>
#include <cstring>
#include <iostream>
#include <iomanip>
int main(){for(int scenario=0;scenario<5;++scenario){
 const int sr=scenario==4?8000:scenario==3?96000:48000,block=scenario==0?256:64;
 qg::Engine engine;engine.prepare(sr);qg::Settings s;s.sampleRate=sr;s.routing=0;s.order=0;s.pattern=0xffff;s.size=.25;s.density=32;s.division=.0625;s.densityFlow=scenario!=0;s.chaos=.4;s.pitch=7;s.position=.3;s.grainMix=.85;s.reslice.enabled=true;s.reslice.mix=.85;s.reslice.algorithm=scenario%3;s.reslice.subdivision=32;s.reslice.repeat=.8;s.reslice.variation=.9;s.reslice.fill=.8;s.reslice.reverse=.5;s.reslice.comb=scenario>=2;s.reslice.crusher=scenario>=2;s.reslice.minBits=8;s.reslice.maxBits=24;s.reslice.minPitch=-700;s.reslice.maxPitch=900;
 std::vector<double> times;uint64_t hash=14695981039346656037ULL;double energy=0;const int seconds=scenario==4?40:6;const int samples=sr*seconds;std::clock_t cpu=std::clock();
 for(int n=0;n<samples;n+=block){auto start=std::chrono::steady_clock::now();if(scenario==4){s.routing=(n/(sr*2))%2?119:-1;s.order=6;s.bufferSeconds=(n/(sr*4))%2?1.:16.;engine.set(s);engine.setFrozen((n/(sr*3))%2!=0);}else engine.set(s);for(int j=0;j<block&&n+j<samples;++j){int i=n+j;float l,r;engine.process(float((i*17)%997)*.0004f-.2f,float((i*31)%991)*.0004f-.2f,i*2./sr,l,r);energy+=double(l)*l+double(r)*r;for(float v:{l,r}){uint32_t bits;std::memcpy(&bits,&v,4);hash=(hash^bits)*1099511628211ULL;}}times.push_back(std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count());}
 double percent=100.*double(std::clock()-cpu)/CLOCKS_PER_SEC/seconds;std::sort(times.begin(),times.end());std::cout<<scenario<<' '<<sr<<' '<<block<<' '<<std::fixed<<std::setprecision(3)<<percent<<' '<<times[size_t(times.size()*.99)]<<' '<<times.back()<<' '<<hash<<' '<<energy<<'\n';
}}
