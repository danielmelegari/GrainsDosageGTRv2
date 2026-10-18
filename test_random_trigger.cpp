#include "src/parameter_settings.h"
#include "src/factory_presets.h"
#include "src/preset_io.h"
#include "src/mockup_ui.h"
#include <cassert>
#include <set>
#include <iostream>
using namespace qg;
using namespace aztec;
int main(){
  for(int root=0;root<12;++root)for(bool minor:{false,true})for(int note=0;note<128;++note){int pc=(note-root+120)%12;assert(RandomTrigger::chordNote(note,root,minor)==(pc==0||pc==(minor?3:4)||pc==7));}
  auto p=initialParameters();assert(p[lfoID(4,lEnabled)]==0.);auto s=settings(p,120,48000);
  assert(s.lfos[4].rndNoteMin==48&&s.lfos[4].rndNoteMax==72&&s.lfos[4].speed==1.);
  for(auto n:s.lfos[4].amount)assert(n==0.);
  LfoSettings r;r.enabled=true;r.rndNoteMin=r.rndNoteMax=60;r.rndMin=r.rndMax=.01;
  RandomTrigger gen;double peak=0.;for(int i=0;i<1000;++i)peak=std::max(peak,std::abs(gen.process(0,r,48000,false)));assert(peak>0.);assert(gen.envelope()==0.); // no repeated triggers while in one bin
  gen.reset();r.rndNoteMin=r.rndNoteMax=61;for(int i=0;i<100;++i)assert(gen.process(0,r,48000,false)==0.);
  r.rndNoteMin=r.rndNoteMax=63;r.rndMinor=true;gen.reset();for(int i=0;i<100;++i)gen.process(0,r,48000,false);assert(gen.envelope()>0.);
  r.rndSource=1;r.rndEvery=2;gen.reset();gen.process(0,r,48000,true);assert(gen.envelope()>0.);
  for(int i=0;i<1000;++i)gen.process(0,r,48000,false);assert(gen.envelope()==0.);
  gen.process(0,r,48000,true);assert(gen.envelope()==0.);gen.process(0,r,48000,true);assert(gen.envelope()>0.);
  // Each pulse ends inside the requested bounds (including its short fade).
  r.rndEvery=1;r.rndMin=.015;r.rndMax=.04;std::set<int> lengths;
  for(int n=0;n<30;++n){gen.process(0,r,48000,true);int frames=0;while(gen.envelope()>0.&&frames<5000){gen.process(0,r,48000,false);++frames;}assert(frames>=int(.015*48000)-2&&frames<=int(.042*48000)+2);lengths.insert(frames);}assert(lengths.size()>10);
  r.enabled=false;assert(gen.process(0,r,48000,true)==0.&&gen.envelope()==0.);
  Modulation mod;mod.prepare();std::array<LfoSettings,lfoCount> mods{};mods[4]=r;mods[4].enabled=true;mods[4].positive[0]=1.;ModValues base{};
  mod.process(mods,base,0,48000,false,0,false,true);for(int i=1;i<10000;++i)mod.process(mods,base,i/48000.,48000,false,0);assert(mod.process(mods,base,1,48000,false,0)[0]==0.); // idle unipolar routes do not add 50%
  Gater gate;gate.prepare(48000);GaterSettings gs;gs.enabled=true;gs.state[1]=0;gate.set(gs);float l=0,rch=0;gate.process(l,rch,0,120);assert(gate.trigger());gate.process(l,rch,.0001,120);assert(!gate.trigger());gate.process(l,rch,.25,120);assert(!gate.trigger());gate.process(l,rch,.5,120);assert(gate.trigger());
  // All appended IDs and six routes are unique; originals stay in their old range.
  std::set<int> ids;for(int i=0;i<5;++i){for(int j=0;j<15;++j)assert(ids.insert(lfoID(i,j)).second);for(auto b:{kRandomSteps0,kLfoSpeed0,kUiLfoPhase0,kUiLfoCycle0,kUiLfoEpoch0,kModWaveRnd0,kUiModWave0,kModWaveRate0,kUiLfoValue0,kLfoWaveChoice0,kModWaveFrom0,kModWaveTo0})assert(ids.insert(modParam(b,i)).second);for(int j=0;j<6;++j){assert(ids.insert(slotTarget(i,j)).second);assert(ids.insert(slotAmount(i,j)).second);assert(ids.insert(slotPolarity(i,j)).second);}}
  mockup::RackState rack;rack.page=1;for(int source=0;source<2;++source){p[kRndSource]=source;auto controls=mockup::controls([&](ParamID id){return p[id];},0,4,0,0,0,rack);std::set<int> seen;for(const auto& c:controls){assert(c.id<kCount&&!isMonitor(c.id));assert(seen.insert(c.id).second);assert(c.r.x>=54&&c.r.x+c.r.w<=1028);}for(int j=0;j<6;++j)assert(seen.count(slotTarget(4,j)));assert(seen.count(kRndSource)&&seen.count(kRndLengthMin)&&seen.count(kRndLengthMax));}
  p[lfoID(4,lEnabled)]=1.;p[kRndMinor]=1.;p[slotTarget(4,5)]=.7;auto bytes=encodePreset([&](int id){return p[id];});std::array<double,kCount> restored;assert(decodePreset(bytes,restored));for(int id=0;id<kCount;++id)if(presetParameter(id))assert(restored[id]==p[id]);
  // Fifth-modulator routes execute in both serial and legacy parallel engines.
  for(double sr:{44100.,96000.})for(int order:{0,6})for(int source:{0,1}){
    Engine engine;engine.prepare(sr);auto params=initialParameters();params[lfoID(4,lEnabled)]=1.;params[kRndSource]=source;params[kGaterEnabled]=1.;params[slotTarget(4,0)]=3./modTargetCount;params[slotAmount(4,0)]=.7;
    auto config=settings(params,120,sr);config.order=order;config.routing=-1;engine.set(config);
    for(int n=0;n<12000;++n){float left,right;engine.process(.1f,.1f,n*2./sr,left,right);assert(std::isfinite(left)&&std::isfinite(right));assert(std::abs(engine.lfoOutput(4))<=1.);}
  }
  std::cout<<"PASS: triads, pulse bounds, edge triggers, Gater, routing silence, IDs, layout and preset round trip\n";
}
