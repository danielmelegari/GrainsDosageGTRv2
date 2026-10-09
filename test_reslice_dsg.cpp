#include "src/reslice.h"
#include <cassert>
#include <cstdlib>
#include <new>
#include <iostream>
#include <array>
#include <vector>
static bool guard=false;static size_t allocations=0;
void* operator new(size_t n){if(guard)++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void* p)noexcept{std::free(p);}void operator delete(void* p,size_t)noexcept{std::free(p);}
void* operator new[](size_t n){return ::operator new(n);}void operator delete[](void* p)noexcept{::operator delete(p);}void operator delete[](void* p,size_t)noexcept{::operator delete(p);}
int main(){
 std::array<std::vector<float>,3> audio;
 for(int mode=0;mode<3;++mode){qg::Reslice e;qg::ResliceSettings s;s.enabled=true;s.algorithm=mode;s.beats=8;s.phraseBeats=8;s.repeat=.6;s.variation=.9;s.fill=.9;s.reverse=.3;s.seed=47;e.prepare(8000);e.set(s);audio[mode].resize(128000);auto bytes=e.allocatedBytes();guard=true;double energy=0;uint64_t last=0;
  for(int i=0;i<128000;++i){float in=.2f*std::sin(i*.037)+.1f*std::sin(i*.091),l=in,r=-in;e.process(l,r,i/4000.,120);audio[mode][i]=l;assert(std::isfinite(l)&&std::abs(l)<=.31&&std::abs(l+r)<1e-6);energy+=l*l;if(e.events()!=last){assert(e.nextCut()>i/4000.-1e-6);last=e.events();}}
  assert(energy>1&&e.events()>40&&e.allocatedBytes()==bytes);s.enabled=false;e.set(s);for(int i=0;i<100;++i){float l=.123f,r=-.21f;e.process(l,r,32+i/4000.,120);if(i>50)assert(l==.123f&&r==-.21f);}guard=false;
 }
 assert(allocations==0);for(int a=0;a<3;++a)for(int b=a+1;b<3;++b){double difference=0;for(size_t i=4000;i<audio[a].size();++i)difference+=std::abs(audio[a][i]-audio[b][i]);assert(difference>100.);}
 // Tempo changes, seeks, pre-roll and algorithm automation stay bounded without allocation.
 for(double sr:{8000.,48000.,192000.}){qg::Reslice e;e.prepare(sr);qg::ResliceSettings s;s.enabled=true;s.reverse=1;s.variation=1;s.repeat=1;s.fill=1;e.set(s);auto memory=e.allocatedBytes();double beat=-1;guard=true;
  for(int i=0;i<24000;++i){double tempo=i<12000?137:190;beat+=tempo/(60*sr);if(i==10000)beat=-.5;if(i%4000==0){s.algorithm=(i/4000)%3;s.seed=1+i;s.beats=i%8000?2:16;e.set(s);}float l=.2f,r=-.2f;e.process(l,r,beat,tempo,i<20000);assert(std::isfinite(l)&&std::abs(l)<=.201&&std::abs(l+r)<1e-6);}guard=false;assert(e.allocatedBytes()==memory&&!e.active());
 }
 assert(allocations==0);
 // Respect non-4/4 bar boundaries using the host's quarter-note metre.
 for(double bar:{3.,3.5,5.})for(int mode=0;mode<3;++mode){qg::Reslice e;e.prepare(8000);qg::ResliceSettings s;s.enabled=true;s.algorithm=mode;s.barBeats=bar;s.phraseBeats=bar*2;e.set(s);for(int i=0;i<48000;++i){double beat=i/4000.;float l=.2f,r=.2f;e.process(l,r,beat,120);if(e.active())assert(e.nextCut()<=(std::floor((beat+1e-8)/bar)+1)*bar+1e-6);}}
 // No dependency on obsolete per-step on/off values.
 qg::Reslice e;e.prepare(8000);qg::ResliceSettings s;s.enabled=true;s.on.fill(false);e.set(s);for(int i=0;i<8000;++i){float l=.2f,r=.2f;e.process(l,r,i/4000.,120);}assert(e.active()&&e.events()>0);
 std::cout<<"PASS: three distinct generative cutters, stereo coherence, bypass, tempo/seek/mode changes and zero audio-thread allocations\n";
}
