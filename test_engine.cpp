#include "src/engine.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

constexpr double rate=48000.;
static double signal(int i) { return .5*std::sin(i*.019)+.2*std::sin(i*.031); }
static std::vector<float> render(qg::Settings s,int frames=96000) {
  qg::Engine e; e.prepare(rate); s.sampleRate=rate; e.set(s);
  std::vector<float> out; out.reserve(frames);
  for(int i=0;i<frames;++i) { float l,r; e.process(float(signal(i)),float(signal(i+13)),i/(rate*60./s.tempo),l,r); assert(std::isfinite(l)&&std::isfinite(r)); out.push_back(l); }
  return out;
}
static double energy(const std::vector<float>& v,size_t begin=24000) { double sum=0.; for(size_t i=begin;i<v.size();++i) sum+=v[i]*v[i]; return sum/(v.size()-begin); }
int main() {
  qg::Settings s; s.sampleRate=rate; s.pattern=0xffff;
  s.mix=0.; auto dry=render(s); for(int i=0;i<int(dry.size());++i) assert(std::abs(dry[i]-signal(i))<1e-6);
  s.mix=1.; s.pattern=0; assert(energy(render(s))==0.);
  s.pattern=0xffff; assert(energy(render(s))>1e-4);
  s.grainMix=0.; s.glitchMix=1.; auto forward=render(s); assert(energy(forward)>1e-4);
  s.glitchReverse=true; auto reverse=render(s); double difference=0.; for(size_t i=24000;i<forward.size();++i) difference+=std::abs(forward[i]-reverse[i]); assert(difference>100.);
  s.glitchChance=0.; assert(energy(render(s))==0.);
  // Record history, then hold a 1/16 slice. New silent input must not erase it.
  qg::Engine e; e.prepare(rate); s.glitchMix=0.; s.repeatMix=1.; s.repeatOn=false; s.release=.001; e.set(s);
  float l=0,r=0; std::vector<float> held;
  for(int i=0;i<120000;++i) {
    if(i==24000) { s.repeatOn=true; e.set(s); }
    e.process(i<24000 ? float(signal(i)) : 0.f,0.f,i/24000.,l,r);
    if(i>=48000) held.push_back(l);
  }
  assert(energy(held,0)>1e-4);
  for(size_t i=6000;i<held.size();++i) assert(std::abs(held[i]-held[i-6000])<2e-5);
  s.repeatOn=false; e.set(s); e.process(0.f,0.f,5.,l,r); assert(l==0.f);
  // Stress all sections, feedback, rate/tempo changes and negative transport positions.
  for(double sr:{44100.,48000.,96000.}) {
    e.prepare(sr); s.sampleRate=sr; s.grainMix=1.; s.glitchMix=1.; s.glitchChance=1.; s.repeatOn=true;
    s.feedback=.85; s.chaos=1.; s.density=8.; s.pitch=12.; s.tempo=400.; e.set(s);
    for(int i=0;i<100000;++i) { if(i==50000) e.resetTransport(); e.process(float(signal(i)),.1f,-2.+i/(sr*60./s.tempo),l,r); assert(std::isfinite(l)&&std::isfinite(r)); assert(std::abs(l)<20.); }
  }
  std::cout << "PASS: dry path, closed gate, grains, glitch/reverse/probability, repeat capture/hold/release/period, transport and feedback stress\n";
}
