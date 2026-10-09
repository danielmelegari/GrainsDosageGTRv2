#include "src/engine.h"
#include <cassert>
#include <iostream>
#include <vector>
using namespace qg;
static constexpr double sr=24000.;
static std::vector<float> render(Settings s,int n=72000,int change=-1,double newDivision=.125) {
  Engine e; e.prepare(sr); s.sampleRate=sr; s.tempo=120.; e.set(s);
  std::vector<float> out(n);
  for(int i=0;i<n;++i) {
    if(i==change) { s.repeatBeats=newDivision; e.set(s); }
    float r; float in=float(.4*std::sin(tau*220.*i/sr)+.1*std::sin(tau*137.*i/sr));
    if(i==12000) { s.repeatOn=true; e.set(s); }
    e.process(in,in,i*2./sr,out[i],r); assert(std::isfinite(out[i]) && out[i]==r);
  }
  return out;
}
static double energy(const std::vector<float>& x,int a,int b) { double v=0.; for(int i=a;i<b;++i) v+=x[i]*x[i]; return v/(b-a); }
static double difference(const std::vector<float>& a,const std::vector<float>& b,int start) { double d=0; for(size_t i=start;i<a.size();++i) d+=std::abs(a[i]-b[i]); return d; }
int main() {
  Settings s; s.pattern=0xFFFF; s.attack=.0002; s.release=.0002; s.grainMix=0.; s.repeatMix=1.;
  auto held=render(s),changed=render(s,72000,24000,.125);
  assert(difference(held,changed,26000)>100.); // Division changes without releasing Hold.
  s.repeatSequence=true; s.repeatPattern=0xAAAA;
  auto gated=render(s);
  assert(energy(gated,24100,26900)<1e-9 && energy(gated,27100,29900)>.001);
  s.repeatRates[9]=.0625; s.repeatPitches[9]=12.;
  auto perStep=render(s); assert(difference(gated,perStep,27000)>10.);
  s.repeatMix=0.; s.glitchMix=1.; s.glitchSequence=true; s.glitchPattern=0xAAAA; s.pattern=0;
  auto glitch=render(s);s.glitchMix=0.;assert(glitch==render(s)); // Retired stage cannot process audio.
  s={}; s.sampleRate=sr; s.pattern=0xFFFF; s.densityFlow=true; s.density=32.; s.chaos=0.;
  s.pitch=-48.; auto low=render(s); s.pitch=48.; auto high=render(s);
  assert(energy(low,48000,72000)>.00001 && energy(high,48000,72000)>.00001 && difference(low,high,48000)>100.);
  s.pitch=0.; s.density=1.; auto sparse=render(s); s.density=32.; auto dense=render(s);
  assert(difference(sparse,dense,24000)>100.);
  Lfo l; l.prepare(42); LfoSettings ls; ls.enabled=true; ls.wave=128; ls.randomSteps=64; ls.beats=32.;
  auto a=l.process(ls,.1,sr,false,0),b=l.process(ls,.6,sr,false,0),c=l.process(ls,.2,sr,false,0);
  assert(a==c && a!=b); ls.wave=129;
  for(int i=1;i<64;++i) assert(std::abs(l.process(ls,i*.5-1e-6,sr,false,0)-l.process(ls,i*.5+1e-6,sr,false,0))<1e-4);
  // Stretch and transpose are separate: stretch preserves local sinusoid pitch,
  // transpose doubles it. Disabled and neutral processing are exact dry paths.
  for(double speed:{.25,.5,1.,2.,4.}) {
    FreeStretch stretch; stretch.prepare(sr); Transpose transpose; transpose.prepare(sr);
    int crossings=0; float previous=0; double e=0;
    for(int i=0;i<72000;++i) {
      float original=float(.4*std::sin(tau*220.*i/sr)),v=original,r=v;
      stretch.process(v,r,true,speed); assert(std::isfinite(v) && v==r);
      if(speed==1.) assert(v==original);
      if(i>48000) { if(previous<=0 && v>0) ++crossings; e+=v*v; } previous=v;
    }
    assert(crossings>190 && crossings<250 && e>10.);
  }
  for(double st:{-12.,0.,12.,48.}) {
    Transpose transpose; transpose.prepare(sr); int crossings=0; float previous=0;
    for(int i=0;i<72000;++i) {
      float original=float(.4*std::sin(tau*220.*i/sr)),v=original,r=v; transpose.process(v,r,st);
      assert(std::isfinite(v) && v==r); if(st==0.) assert(v==original);
      if(i>48000 && previous<=0 && v>0) ++crossings; previous=v;
    }
    double expected=220.*std::pow(2.,st/12.); assert(std::abs(crossings-expected)<expected*.1+5.);
  }
  Settings route; route.pitch=7.; route.pattern=0xFFFF; route.densityFlow=true; route.density=12.;
  route.size=.12; route.position=.05; route.glitchMix=.85; route.glitchChance=1.;
  route.glitchSequence=true; route.glitchPattern=0xFFFF; route.glitchBeats=.0625;
  route.repeatMix=.9; route.repeatSequence=true; route.repeatPattern=0xFFFF;
  route.order=0; auto grainFirst=render(route);
  route.order=2; auto glitchFirst=render(route);
  route.order=5; auto repeatFirst=render(route);
  assert(difference(grainFirst,glitchFirst,30000)==0.);
  assert(difference(glitchFirst,repeatFirst,30000)>100.);
  for(int order=0;order<6;++order) {
    route.order=order;auto audio=render(route,36000);
    assert(energy(audio,26000,35000)>.00001);
  }
  std::cout<<"PASS: live Hold division, independent grids, step division/pitch, +/-48 grain pitch, distributed density, 64-point SH/SG, stretch pitch preservation and final transpose; six serial module orders\n";
}

