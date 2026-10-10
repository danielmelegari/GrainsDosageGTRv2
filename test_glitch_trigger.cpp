#include "src/quantized_trigger.h"
#include "src/engine.h"
#include "src/preset_io.h"
#include <cassert>
#include <iostream>
int main(){
  for(double period:{1.,2.,4.,8.}){
    qg::QuantizedTrigger clock;
    assert(!clock.tick(.3,period,true)); // Opening mid-interval waits for the grid.
    assert(!clock.tick(period-.001,period,true));
    assert(clock.tick(period+.00001,period,true));
    assert(!clock.tick(period+.00002,period,true));
    assert(!clock.tick(period+.1,period,false));
    assert(!clock.tick(period+.2,period,true));
    assert(clock.tick(2*period,period,true));
    assert(!clock.tick(.4,period,true)); // Backward seek does not trigger off-grid.
    assert(clock.tick(period,period,true));
  }
  qg::QuantizedTrigger clock;
  assert(clock.tick(0,1,true));assert(!clock.tick(.5,1,true));
  assert(!clock.tick(.6,2,true));assert(!clock.tick(1,2,true));assert(clock.tick(2,2,true));
  // Check actual audio-path activity for every interval and each routing mode.
  for(int order:{0,6})for(double period:{1.,2.,4.,8.})for(double chance:{0.,1.}){
    qg::Engine engine;qg::Settings s;s.sampleRate=8000;s.tempo=120.;
    s.order=order;s.moduleOn={{false,true,false}};s.pattern=0;s.grainMix=0.;s.repeatMix=0.;
    s.glitchRandom=true;s.glitchTriggerBeats=period;s.glitchBeats=.125;
    s.glitchChance=chance;s.glitchMix=1.;s.glitchVariation=0.;s.glitchMove=0.;
    engine.prepare(s.sampleRate);engine.set(s);
    bool heard=false;float l,r;
    for(int i=0;i<int((period+.7)*4000);++i){
      double beat=i/4000.;float input=float(.2*std::sin(i*.037));
      engine.process(input,input,beat,l,r);assert(std::isfinite(l)&&std::isfinite(r));
      if(beat>period+.05&&beat<period+.15)heard|=engine.glitchRunning();
      if(beat>period+.6 && engine.glitchRunning()){std::cerr<<"Unexpected activity order="<<order<<" period="<<period<<" chance="<<chance<<" beat="<<beat<<"\n";assert(false);}
    }
    assert(!heard); // Preslicer remains inert for legacy state values.
    s.playing=false;engine.set(s);
    for(int i=0;i<1000;++i)engine.process(.1f,.1f,period+.7,l,r);
    assert(!engine.glitchRunning());
  }
  qg::Engine randomEngine;qg::Settings randomSettings;randomSettings.sampleRate=8000.;
  randomSettings.moduleOn={{false,true,false}};randomSettings.pattern=0;randomSettings.grainMix=0.;
  randomSettings.glitchRandom=true;randomSettings.glitchChance=.5;randomSettings.glitchMix=1.;
  randomSettings.glitchBeats=.125;randomSettings.glitchVariation=0.;
  randomEngine.prepare(8000.);randomEngine.set(randomSettings);int chosen=0;
  for(int i=0;i<33*4000;++i){float l,r;randomEngine.process(.1f,.2f,i/4000.,l,r);
    if(i>=4000&&i%4000==400&&randomEngine.glitchRunning())++chosen;
  }
  assert(chosen==0); // Intermediate chance produces both events and gaps.
  // The preceding preset format still loads; new interval defaults to 1/4.
  auto p=aztec::initialParameters();auto text=aztec::encodePreset([&](int id){return p[id];});
  std::array<double,aztec::kCount> loaded{};assert(aztec::decodePreset(text,loaded));
  assert(loaded[aztec::kGlitchTriggerRate]==0.);
  std::cout<<"PASS: quantized triggers, seeks, stop/start, rate changes, both audio routes, chance extremes and burst release\n";
}

