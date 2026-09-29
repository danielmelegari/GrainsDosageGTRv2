#include "src/engine.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <set>
#include <vector>

int main() {
  using namespace qg;
  std::set<uint64_t> shapes;
  for(const auto& table:waveBank()) {
    uint64_t hash=14695981039346656037ULL;
    for(float v:table) {
      assert(std::isfinite(v) && v>=-1.000001f && v<=1.000001f);
      uint32_t bits; std::memcpy(&bits,&v,sizeof(bits)); hash=(hash^bits)*1099511628211ULL;
    }
    shapes.insert(hash); assert(table[0]==table[tableSize]);
  }
  assert(shapes.size()==128); // 128 distinct periodic tables, not duplicate names.
  Lfo a,b; a.prepare(1); b.prepare(2);
  LfoSettings s; s.enabled=true; s.wave=0; s.beats=4.;
  assert(std::abs(a.process(s,1.,48000.,false,0)-1.)<1e-6);
  assert(std::abs(a.process(s,1.,96000.,false,0)-1.)<1e-6);
  assert(std::abs(a.process(s,5.,48000.,false,0)-1.)<1e-6);
  s.phase=.5; assert(a.process(s,1.,48000.,false,0)<-.999);
  s.phase=0.; s.wave=128;
  const double hold=a.process(s,.1,48000.,false,0);
  assert(hold==a.process(s,3.99,48000.,false,0));
  assert(hold!=a.process(s,4.01,48000.,false,0));
  assert(hold!=b.process(s,.1,48000.,false,0));
  s.wave=129; s.glide=1.;
  const double left=a.process(s,4.-1e-6,48000.,false,0), right=a.process(s,4.,48000.,false,0);
  assert(std::abs(left-right)<1e-8);
  s.wave=0; s.gateReset=true;
  assert(std::abs(a.process(s,7.25,48000.,true,29))<1e-6);
  assert(a.process(s,8.25,48000.,false,29)>.999);
  s.gateReset=false; s.sync=false; s.hz=2.; a.reset();
  double freeValue=0.; for(int i=0;i<=6000;++i) freeValue=a.process(s,1000.,48000.,false,0);
  assert(freeValue>.999); // Free rate ignores PPQ: quarter cycle at 2 Hz.
  Modulation matrix; matrix.prepare();
  std::array<LfoSettings,lfoCount> lfos{};
  ModValues base{.5,.5,.5,.5,.5,.5};
  for(int i=0;i<lfoCount;++i) { lfos[i].enabled=true; lfos[i].beats=4.; }
  lfos[0].amount[0]=.2; lfos[1].amount[0]=-.1;
  lfos[2].amount[5]=.8; lfos[3].amount[2]=-.8;
  auto m=matrix.process(lfos,base,1.,48000.,false,0);
  assert(std::abs(m[0]-.6)<1e-6 && m[1]==.5 && m[2]==0. && m[5]==1.);
  for(auto& l:lfos) l.enabled=false;
  assert(matrix.process(lfos,base,1.,48000.,false,0)==base);
  // Route every waveform through every destination under worst-case feedback.
  Engine engine; engine.prepare(48000.); Settings settings; settings.sampleRate=48000.;
  settings.pattern=0xffff; settings.tempo=145.; settings.feedback=.85;
  settings.glitchMix=.5; settings.repeatMix=.5; settings.repeatOn=true;
  for(int i=0;i<lfoCount;++i) {
    settings.lfos[i].enabled=true; settings.lfos[i].sync=false; settings.lfos[i].hz=2.+i;
    settings.lfos[i].amount.fill(i%2 ? -.7 : .7);
  }
  for(int wave=0;wave<130;++wave) {
    for(auto& l:settings.lfos) l.wave=wave;
    engine.set(settings);
    for(int i=0;i<1024;++i) {
      const int index=wave*1024+i; float l,r;
      engine.process(float(.2*std::sin(index*.1)),.1f,index*145./(60.*48000.),l,r);
      assert(std::isfinite(l)&&std::isfinite(r)&&std::abs(l)<20.);
    }
  }
  // Compare active pitch modulation with disabled modulation: routing must affect audio.
  auto render=[](bool enabled) {
    Engine e; e.prepare(48000.); Settings s; s.sampleRate=48000.; s.pattern=0xffff;
    s.lfos[0].enabled=enabled; s.lfos[0].amount[2]=.25; e.set(s);
    std::vector<float> out;
    for(int i=0;i<48000;++i) { float l,r; e.process(float(std::sin(i*.05)),0.f,i/24000.,l,r); out.push_back(l); }
    return out;
  };
  const auto dry=render(false), modulated=render(true); double diff=0.;
  for(size_t i=0;i<dry.size();++i) diff+=std::abs(dry[i]-modulated[i]);
  assert(diff>100.);
  // Both appended routes must affect real output in serial and legacy modes.
  for(int order : {0,1,2,3,4,5,6}) for(int target : {6,7}) {
    auto renderExtra=[&](double amount) {
      Engine e; e.prepare(48000.); Settings s; s.sampleRate=48000.;
      s.order=order; s.pattern=0xffff; s.grainMix=order==6?1.:0.; s.densityFlow=true; s.mix=.5; s.stretchOn=true;
      s.lfos[0].enabled=true; s.lfos[0].amount[target]=amount; e.set(s);
      std::vector<float> out;
      for(int i=0;i<48000;++i) {
        float l,r; const float input=float(.3*std::sin(i*.041)+.1*std::sin(double(i)*i*.0000007));
        e.process(input,input,i/24000.,l,r);
        assert(std::isfinite(l)&&std::isfinite(r)); out.push_back(l);
      }
      return out;
    };
    auto base=renderExtra(0.), moved=renderExtra(.2); double difference=0.;
    for(size_t i=0;i<base.size();++i) difference+=std::abs(base[i]-moved[i]);
    assert(difference>100.);
  }
  std::cout<<"PASS: 128 distinct waves, bounds, sync, phase, S&H, S&G continuity, independent seeds, retrigger, free rate, bipolar routing, bypass, all-wave audio stress and audible pitch/speed/transpose modulation in all orders\n";
}
