#include "src/module_loop.h"
#include "src/engine.h"
#include "src/factory_presets.h"
#include "src/parameter_settings.h"
#include <cassert>
#include <cstdlib>
#include <new>
#include <iostream>
static bool guard=false;static int allocations=0;
void* operator new(size_t n){if(guard)++allocations;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void* p)noexcept{std::free(p);}void operator delete(void* p,size_t)noexcept{std::free(p);}
void* operator new[](size_t n){return ::operator new(n);}void operator delete[](void* p)noexcept{::operator delete(p);}void operator delete[](void* p,size_t)noexcept{::operator delete(p);}
int main(){
 for(double beats:{8.,4.,2.,1.}){qg::ModuleLoop loop;loop.prepare(8000);const int period=int(beats*4000);guard=true;
  for(int i=0;i<period*3;++i){float l=i<period?.25f:-.5f,r=-l;loop.process(l,r,i>=period,beats,true,i/4000.,120,true);if(i>period+100)assert(std::abs(l-.25)<1e-6&&std::abs(r+.25)<1e-6);}
  assert(loop.active());for(int i=0;i<200;++i){float l=-.5f,r=.5f;loop.process(l,r,false,beats,true,(period*3+i)/4000.,120,true);if(i>100)assert(l==-.5f&&r==.5f);}assert(!loop.active());guard=false;
 }
 // Captures the newest window in order, then repeats it at the host tempo.
 {qg::ModuleLoop loop;loop.prepare(8000);double beat=0;int captured=0;guard=true;
  for(int i=0;i<12000;++i){float l=float(i)/12000.f,r=-l;loop.process(l,r,false,1,true,beat,120,true);beat+=1./4000;}
  for(int i=0;i<8000;++i){float l=-1,r=1;loop.process(l,r,true,1,true,beat,120,true);beat+=1./4000;if(i%4000>100){assert(std::abs(l-float(8000+i%4000)/12000.f)<1e-5);++captured;}}
  assert(captured>7000);
  // Doubling tempo halves the captured loop's playback duration.
  for(int i=0;i<4000;++i){float l=-1,r=1;loop.process(l,r,true,1,true,beat,240,true);beat+=1./2000;if(i%2000>100)assert(std::abs(l-float(8000+2*(i%2000))/12000.f)<1e-5);}
  // OFF invalidates the frozen history. Re-arming waits for fresh audio.
  for(int i=0;i<4100;++i){float l=-.3f,r=.3f;loop.process(l,r,i>=100,1,true,beat,240,true);beat+=1./2000;if(i>2300)assert(std::abs(l+.3)<1e-6);}
  assert(loop.active());loop.transportReset();
  for(int i=0;i<5000;++i){float l=-1,r=1;loop.process(l,r,true,1,true,0,240,false);assert(std::abs(l+.3)<1e-6);}
  assert(loop.active());guard=false;
 }
 // All four loops are independent and bounded through transport/tempo/window edits.
 for(double rate:{8000.,48000.,192000.}){qg::ModuleLoop loop;loop.prepare(rate);guard=true;double beat=0;for(int i=0;i<30000;++i){double tempo=i<15000?400:20;beat+=tempo/(60*rate);if(i==10000)beat=-1;float l=.2f,r=-l;loop.process(l,r,(i/1500)%2,1<<(i/1000%4),i%3000!=0,beat,tempo,i<29000);assert(std::isfinite(l)&&std::abs(l)<=.201&&std::abs(l+r)<1e-6);}guard=false;}
 assert(allocations==0);
 for(int stage:{0,2,3,4}){qg::Engine e;e.prepare(8000);qg::Settings s;s.sampleRate=8000;s.routing=0;s.pattern=0xffff;s.densityFlow=true;s.moduleOn={false,false,false};s.reslice.enabled=false;s.gater.enabled=false;const int index=aztec::moduleLoopIndex(stage);if(stage<3)s.moduleOn[stage]=true;else if(stage==3)s.reslice.enabled=true;else s.gater.enabled=true;s.loopBeats[index]=1.;s.grainMix=0;s.repeatMix=0;s.reslice.mix=0;s.gater.length.fill(1.);s.mix=1;e.set(s);
  guard=true;for(int i=0;i<12000;++i){if(i==8000){s.loopOn[index]=true;e.set(s);}float l,r;e.process(i<8000?.2f:-.5f,i<8000?.2f:-.5f,i/4000.,l,r);if(i==9000)e.resetTransport();if(i>8500)assert(l>-.1f&&std::isfinite(l));}guard=false;assert(allocations==0);
 }
 auto p=aztec::initialParameters();for(int i=0;i<4;++i){assert(p[aztec::moduleLoopOn(i)]==0);for(int window=0;window<4;++window){p[aztec::moduleLoopWindow(i)]=window/3.;assert(aztec::settings(p,120,48000).loopBeats[i]==double(8>>window));}}
 std::cout<<"PASS: four loop windows, held capture, OFF release, transport/tempo/window changes, zero realtime allocation and per-stage routing\n";
}
