#include "src/engine.h"
#include "src/factory_presets.h"
#include "src/preset_io.h"
#include <cassert>
#include <set>
#include <iostream>
using namespace aztec;
int main(){
 std::set<std::array<int,5>> orders;
 for(int i=0;i<120;++i){auto order=fiveModuleOrder(i);orders.insert(order);auto sorted=order;std::sort(sorted.begin(),sorted.end());assert((sorted==std::array<int,5>{{0,1,2,3,4}}));
  qg::Settings s;s.sampleRate=8000;s.routing=i;s.moduleOn={false,false,false};s.grainMix=s.glitchMix=s.repeatMix=0;s.reslice.enabled=s.gater.enabled=false;
  qg::Engine e;e.set(s);e.prepare(8000);e.set(s);
  for(int n=0;n<128;++n){float x=float(std::sin(n*.3)*.2),l,r;e.process(x,-x,n/4000.,l,r);assert(std::isfinite(l)&&std::isfinite(r));assert(std::abs(l-x)<1e-5&&std::abs(r+x)<1e-5);}
 }
 assert(orders.size()==120);
 // A repeater after a gater repeats its captured gate envelope; a gater after
 // a repeater gates its output. This must be a real audio-order change.
 auto render=[](std::array<int,5> desired){int order=0;while(order<120&&fiveModuleOrder(order)!=desired)++order;assert(order<120);
  qg::Settings s;s.sampleRate=8000;s.tempo=120;s.routing=order;s.moduleOn={false,false,true};s.grainMix=s.glitchMix=0;s.repeatMix=1;s.repeatOn=false;s.repeatSequence=true;s.repeatPattern=0xffff;s.repeatBeats=.25;
  s.gater.enabled=true;s.gater.grid=.25;for(int i=0;i<16;++i){s.gater.state[i]=i%2;s.gater.length[i]=.5;}
  qg::Engine e;e.set(s);e.prepare(8000);e.set(s);std::vector<float> out;
  for(int n=0;n<16000;++n){float l,r;float x=float(.2*std::sin(n*.137)+.1*std::sin(n*.017));e.process(x,x,n/4000.,l,r);if(n>8000)out.push_back(l);}return out;
 };
 auto a=render({0,1,4,2,3}),b=render({0,1,2,4,3});double difference=0;for(size_t i=0;i<a.size();++i)difference+=std::abs(a[i]-b[i]);assert(difference>.01);
 std::array<qg::LfoSettings,4> lfos{};qg::ModValues base{};base.fill(.5);
 lfos[0].enabled=true;lfos[0].wave=0;lfos[0].beats=1;lfos[0].depth=.6;
 for(int polarity=0;polarity<3;++polarity){qg::Modulation mod;mod.prepare();lfos[0].amount[0]=polarity==0?.4:0;lfos[0].positive[0]=polarity==1?.4:0;lfos[0].negative[0]=polarity==2?.4:0;
  double lo=1,hi=0;for(int n=0;n<1000;++n){double v=mod.process(lfos,base,n/1000.,48000,false,0)[0];lo=std::min(lo,v);hi=std::max(hi,v);}
  if(polarity==0){assert(lo<.3&&hi>.7);}if(polarity==1){assert(lo>=.5-1e-9&&hi>.7);}if(polarity==2){assert(hi<=.5+1e-9&&lo<.3);}
 }
 lfos[0].enabled=false;qg::Modulation disabled;disabled.prepare();assert(disabled.process(lfos,base,.25,48000,false,0)[0]==.5);
 // Opposite unipolar routes to the same destination cancel without changing
 // bipolar routes or creating an offset when the LFO is disabled.
 lfos[0].enabled=true;lfos[0].amount[0]=0;lfos[0].positive[0]=lfos[0].negative[0]=.4;
 assert(std::abs(disabled.process(lfos,base,.25,48000,false,0)[0]-.5)<1e-12);
 auto preset=initialParameters();preset[slotPolarity(2,5)]=1; preset[kRoutingOrder]=1;
 std::array<double,kCount> loaded{};assert(decodePreset(encodePreset([&](int id){return preset[id];}),loaded));assert(loaded[slotPolarity(2,5)]==1&&loaded[kRoutingOrder]==1);
 std::cout<<"PASS: 120 five-stage routes, audible reorder, three modulation polarities, disabled/cancelling routes and preset round-trip\n";
}
