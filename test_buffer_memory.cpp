#include "src/engine.h"
#include <cassert>
#include <cstdlib>
#include <new>
#include <iostream>

static bool watch=false;
static size_t allocations=0,deallocations=0,allocatedBytes=0;
void* operator new(std::size_t n){if(watch){++allocations;allocatedBytes+=n;}if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{if(watch&&p)++deallocations;std::free(p);}
void operator delete[](void* p)noexcept{::operator delete(p);}
void operator delete(void* p,std::size_t)noexcept{::operator delete(p);}
void operator delete[](void* p,std::size_t)noexcept{::operator delete(p);}

int main(){
 qg::Engine e;qg::Settings s;s.sampleRate=8000;s.routing=0;s.pattern=0xffff;s.bufferSeconds=1.;
 e.prepare(s.sampleRate);e.set(s);float l=0,r=0;
 // Wrap the physical ring with identifiable history while the selected window
 // is only one second. Growing must reveal retained audio, without moving it.
 for(int i=0;i<20*8000;++i){float x=float(i)/160000.f;e.process(x,x,i/4000.,l,r);}
 e.setFrozen(true);
 for(int i=0;i<1000;++i)e.process(-1,-1,40.+i/4000.,l,r);
 auto before=e.grainView();assert(before.seconds==1.);
 // Holding for longer than the ring must not advance the captured head.
 for(int i=0;i<17*8000;++i)e.process(-1,-1,41.+i/4000.,l,r);
 auto held=e.grainView();assert(before.low==held.low&&before.high==held.high);
 s.bufferSeconds=8.;e.set(s);
 for(int i=0;i<1000;++i)e.process(-1,-1,80.+i/4000.,l,r);
 auto expanded=e.grainView();assert(expanded.seconds==8.);
 // Captured samples run from 12 s (0.6) to 20 s (1.0), including physical wrap.
 for(int i=0;i<aztec::waveformBins;++i){double expected=.6+.4*(i+1.)/aztec::waveformBins;assert(std::abs(expanded.high[i]-expected)<.001);}
 // Exercise every five-module order, legacy parallel/serial transitions, and
 // every buffer selection both live and frozen: no allocation OR release.
 watch=true;
 for(int route=-2;route<120;++route){
  s.routing=route<0?-1:route;s.order=route==-2?6:0;
  for(double seconds:{1.,2.,4.,8.,16.}){
   s.bufferSeconds=seconds;e.set(s);e.setFrozen(route%2==0);
   for(int i=0;i<256;++i){e.process(.1f,-.1f,100.+i/4000.,l,r);assert(std::isfinite(l)&&std::isfinite(r));}
  }
 }
 watch=false;assert(allocations==0&&deallocations==0);
 // A new parallel capture after serial playback must not expose stale samples
 // from the shared bank when no new input has been recorded.
 e.setFrozen(false);s.routing=0;s.moduleOn={false,false,false};e.set(s);
 for(int i=0;i<8000;++i)e.process(.5f,.5f,110.+i/4000.,l,r);
 s.routing=-1;s.order=6;s.moduleOn={false,false,true};s.grainMix=0;s.repeatMix=1;s.repeatOn=true;s.repeatBeats=1.;e.set(s);
 for(int i=0;i<8000;++i){e.process(0,0,114.+i/4000.,l,r);if(i>1000)assert(std::abs(l)<1e-6&&std::abs(r)<1e-6);}
 // Keep the prepared storage budget bounded at all supported host rates.
 for(double rate:{44100.,48000.,96000.,192000.}){
  qg::Engine prepared;allocatedBytes=0;watch=true;prepared.prepare(rate);watch=false;
  assert(allocatedBytes<size_t(rate*1600.)+1048576);
  qg::Settings high;s=high;s.sampleRate=rate;s.routing=0;
  allocations=deallocations=0;watch=true;
  for(double seconds:{1.,16.,2.,8.,4.}){s.bufferSeconds=seconds;prepared.set(s);prepared.process(.1f,-.1f,0,l,r);}
  watch=false;assert(allocations==0&&deallocations==0);
 }
 std::cout<<"PASS: allocation-free buffer/routing changes, wrapped history, fixed Freeze anchor, frozen window expansion, shared-bank validity\n";
}
