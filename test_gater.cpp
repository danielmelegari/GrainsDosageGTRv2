#include "src/gater.h"
#include "src/engine.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
 qg::Gater gate;gate.prepare(48000);qg::GaterSettings s;gate.set(s);
 for(int i=0;i<1000;++i){float l=.4f,r=-.2f;gate.process(l,r,i/24000.,120);assert(l==.4f&&r==-.2f);}

 // Short Wet steps release into following Off steps for the selected sustain time.
 s.enabled=true;s.state.fill(0);s.state[0]=1;s.length.fill(1.);s.length[0]=.25;s.sustain.fill(.2);
 gate.prepare(8000);gate.set(s);float atTail=0.,afterTail=0.;
 for(int i=0;i<2200;++i){float l=1.,r=1.;gate.process(l,r,i/4000.,120);if(i==700)atTail=l;if(i==2000)afterTail=l;}
 assert(atTail>.2&&atTail<1.);assert(afterTail==0.);

 // The global minimum clamps both fixed and randomized step lengths.
 s.minimumLength=.35;s.length.fill(.12);s.state.fill(1);s.lengthRandom=false;gate.prepare(8000);gate.set(s);
 float l=1,r=1;gate.process(l,r,.03,120);assert(std::abs(gate.length(0)-.35)<1e-12);
 s.lengthRandom=true;s.length.fill(.75);s.minimumLength=.4;s.chance=.5;s.stepRandom=true;gate.prepare(8000);gate.set(s);
 int wet=0,off=0;double previous=-1.;int changing=0;
 for(int cycle=0;cycle<12;++cycle){l=r=1;gate.process(l,r,cycle*4.+.01,120);for(int i=0;i<16;++i){assert(gate.state(i)==0||gate.state(i)==1);assert(gate.length(i)>=.4&&gate.length(i)<=.75);wet+=gate.state(i)==1;off+=gate.state(i)==0;}if(previous!=gate.length(0))++changing;previous=gate.length(0);gate.process(l,r,cycle*4.+.03,120);assert(gate.length(0)==previous);}
 assert(wet>0&&off>0&&changing>8);

 // TIE removed: adjacent Wet steps now re-trigger per step (length .1 => short gated notes).
 s.stepRandom=false;s.lengthRandom=false;s.minimumLength=.05;s.length.fill(.1);s.sustain.fill(0.);s.state.fill(1);s.tie=true; // tie flag ignored
 gate.prepare(8000);gate.set(s);{int openSamples=0;for(int i=0;i<34000;++i){l=r=1;gate.process(l,r,i/4000.,120);if(i>500&&gate.envelope()>.999)++openSamples;}assert(openSamples<34000);} // no longer one continuous note
 s.tie=false;gate.prepare(8000);gate.set(s);l=r=1;gate.process(l,r,.02,120);for(int i=1;i<300;++i){l=r=1;gate.process(l,r,.02+i/4000.,120);}assert(gate.envelope()<.01);

 // Latch works independently of the removed Tie: empty steps hold until an explicit release;
 // transport stop closes it cleanly.
 s.tie=false;s.latch=true;s.state.fill(0);s.state[0]=1;s.release=1u<<6;gate.prepare(8000);gate.set(s);
 for(int i=0;i<8000;++i){l=r=1;gate.process(l,r,i/4000.,120);int step=i/1000;if(step<6&&i%1000>80)assert(l>.99);if(step>=7&&i%1000>100)assert(l<.01);}
 for(int i=0;i<100;++i){l=r=1;gate.process(l,r,2.1,120,false);}assert(gate.envelope()<.01);

 // Gating remains correctly placed for all module orders; reverb can ring out
 // after the Gater closes its input.
 for(int order=0;order<7;++order){qg::Engine e;e.prepare(8000);qg::Settings p;p.sampleRate=8000;p.order=order;p.mix=1.;p.moduleOn={false,false,false};p.gater.enabled=true;p.gater.state.fill(0);p.gater.state[0]=1;p.gater.length.fill(1.);p.gater.sustain.fill(.05);e.set(p);for(int i=0;i<700;++i)e.process(.3f,-.2f,i/4000.,l,r);assert(l>.2f);for(int i=700;i<1800;++i)e.process(.3f,-.2f,i/4000.,l,r);assert(std::abs(l)<1e-5&&std::abs(r)<1e-5);}
 {qg::Engine e;e.prepare(8000);qg::Settings p;p.sampleRate=8000;p.order=1;p.mix=1.;p.moduleOn={false,false,false};p.gater.enabled=true;p.gater.state.fill(1);p.gater.length.fill(1.);p.reverbOn=true;p.reverbMix=1.;p.reverbLength=3.;p.reverbPattern=0xffff;e.set(p);for(int i=0;i<4000;++i)e.process(float(.3*std::sin(i*.12)),float(.3*std::sin(i*.12)),i/4000.,l,r);p.gater.state.fill(0);e.set(p);double energy=0.;for(int i=4000;i<12000;++i){e.process(0,0,i/4000.,l,r);if(i>5000)energy+=l*l+r*r;}assert(energy>1e-6);}
 std::cout<<"PASS: Off/Wet steps, per-step sustain tails, minimum-length clamp, random ranges, Tie across the loop, independent Latch and reverb tails\n";
}
