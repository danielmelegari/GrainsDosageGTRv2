#include "src/engine.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
static double rms(const std::vector<float>& a,int from,int to){double s=0.;for(int i=from;i<to;++i)s+=a[i]*a[i];return std::sqrt(s/(to-from));}
int main(){
  constexpr double sr=24000.;
  // A non-repeating input must become sample-identical loops, including when
  // Hold is on before audio begins and when only GRID is enabled.
  for(bool glitch:{false,true})for(double division:{.03125,.25,1.}){
    qg::Engine e;e.prepare(sr);qg::Settings s;s.sampleRate=sr;s.order=0;s.grainMix=0.;
    s.pattern=0xffff;s.repeatOn=!glitch;s.repeatMix=1.;s.repeatBeats=division;
    s.glitchMix=glitch?1.:0.;s.glitchBeats=division;s.glitchSequence=true;
    e.set(s);std::vector<float> out(96000),dry(96000);
    for(int i=0;i<96000;++i){
      dry[i]=i<12000?0.f:float(.3*std::sin(i*.071)+.1*std::sin(double(i)*i*.000001));
      float r;e.process(dry[i],dry[i],i*2./sr,out[i],r);assert(std::isfinite(out[i]));
    }
    int period=int(std::round(division*sr/2.));
    assert(rms(out,60000,84000)>.1);
    for(int i=60000;i<84000;++i)assert(std::abs(out[i]-out[i-period])<1e-5);
  }
  // Level: unity-pitch granular overlap should preserve a steady source, not
  // attenuate it through Hann envelopes or clamp peaks to 1.0.
  qg::Engine grain;grain.prepare(sr);qg::Settings g;g.sampleRate=sr;g.order=0;
  g.pattern=0xffff;g.densityFlow=true;g.density=4.;g.size=.075;g.position=.1;grain.set(g);
  std::vector<float> wet(72000),dry(72000);
  for(int i=0;i<72000;++i){dry[i]=float(.5*std::sin(qg::tau*220.*i/sr));float r;grain.process(dry[i],dry[i],i*2./sr,wet[i],r);}
  double db=20.*std::log10(rms(wet,48000,72000)/rms(dry,48000,72000));assert(std::abs(db)<.1);
  // Grid alone activates a repeater; off steps return the exact dry signal.
  qg::Engine grid;grid.prepare(sr);g.grainMix=0.;g.repeatSequence=true;g.repeatPattern=0x0f0f;g.repeatMix=1.;g.repeatOn=false;grid.set(g);
  double changed=0.;
  for(int i=0;i<72000;++i){float input=float(.3*std::sin(double(i)*i*.000001)),l,r;grid.process(input,input,i*2./sr,l,r);
    if(i>24000&&i%24000>12500&&i%24000<23000)assert(std::abs(l-input)<1e-5);
    if(i>24000&&i%24000>6000&&i%24000<11000)changed+=std::abs(l-input);
  }assert(changed>100.);
  // Frozen source survives silent live input, live division changes, reversal.
  qg::RhythmicLoop loop;loop.prepare(sr);std::vector<float> held(48000);
  for(int i=0;i<48000;++i){float l=i<12000?float(.4*std::sin(i*.071)):0.f,r=l;
    loop.process(l,r,i>=12000,true,i<24000?3000:1500,0.,i>=36000,1.);held[i]=l;
  }
  assert(rms(held,30000,35000)>.2);
  for(int i=30000;i<35000;++i)assert(std::abs(held[i]-held[i-1500])<1e-5);
  qg::RhythmicLoop gainLoop;gainLoop.prepare(sr);std::vector<float> repeated(72000),source(72000);
  for(int i=0;i<72000;++i){source[i]=float(.5*std::sin(qg::tau*220.*i/sr));float l=source[i],r=l;
    gainLoop.process(l,r,true,true,3000,0.,false,1.);repeated[i]=l;
  }
  double loopDb=20.*std::log10(rms(repeated,48000,72000)/rms(source,48000,72000));
  assert(loopDb> -1. && loopDb<.1);
  // Live recording continues while held: an explicit recapture must pick up
  // the new source, not one fresh sample mixed into stale frozen history.
  qg::RhythmicLoop refresh;refresh.prepare(sr);double late=0.;
  for(int i=0;i<48000;++i){float l=i<24000?.2f:.6f,r=l;refresh.process(l,r,true,true,1500,0.,false,1.,0.,i==30000);if(i>36000)late+=l;}
  assert(late/11999.>.5);
  qg::Engine automatic;automatic.prepare(sr);qg::Settings autoSettings;autoSettings.sampleRate=sr;autoSettings.order=0;autoSettings.grainMix=0.;autoSettings.repeatMix=1.;autoSettings.repeatAuto=true;autoSettings.repeatInterval=2.;autoSettings.repeatDuration=.5;autoSettings.repeatBeats=.125;automatic.set(autoSettings);
  double altered=0.;for(int i=0;i<96000;++i){float input=float(.3*std::sin(double(i)*i*.000001)),l,r;automatic.process(input,input,i*2./sr,l,r);int phase=i%24000;if(i>24000&&phase>8000&&phase<22000)assert(std::abs(l-input)<1e-5);if(i>24000&&phase>1000&&phase<5000)altered+=std::abs(l-input);}
  assert(altered>100.);
  qg::Engine glitchRefresh;glitchRefresh.prepare(sr);qg::Settings fresh;fresh.sampleRate=sr;fresh.order=0;fresh.grainMix=0.;fresh.glitchMix=1.;fresh.glitchSequence=true;fresh.glitchRefresh=.5;fresh.glitchBeats=.0625;glitchRefresh.set(fresh);double latest=0.;
  for(int i=0;i<96000;++i){float l,r;float in=i<24000?.2f:.6f;glitchRefresh.process(in,in,i*2./sr,l,r);if(i>=72000)latest+=l;}
  assert(latest/24000.>.5);
  qg::RhythmicLoop wrapped;wrapped.prepare(8000.);
  for(int i=0;i<200000;++i){float l=float(.4*std::sin(i*.03)),r=l;wrapped.process(l,r,true,true,3000.,0.,false,1.,2000.,i%4000==0);assert(std::isfinite(l)&&std::abs(l)<1.);}
  std::cout<<"PASS: real periodic repeat/glitch, silent-start recovery, grid without Hold, dry off steps, live length/reverse, held source; granular gain "<<db<<" dB, loop gain "<<loopDb<<" dB\n";
}
