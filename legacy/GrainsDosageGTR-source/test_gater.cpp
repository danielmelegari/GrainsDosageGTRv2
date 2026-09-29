#include "src/gater.h"
#include "src/engine.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
 qg::Gater gate;gate.prepare(48000);qg::GaterSettings s;gate.set(s);
 for(int i=0;i<1000;++i){float l=.4f,r=-.2f;gate.process(l,r,i/24000.,120);assert(l==.4f&&r==-.2f&&gate.dryGain()==0.);}
 s.enabled=true;s.state.fill(0);s.state[0]=1;s.state[1]=2;s.length.fill(.5);gate.prepare(48000);gate.set(s);
 for(int i=0;i<18000;++i){float l=.4f,r=-.2f;gate.process(l,r,i/24000.,120);int step=i/6000,phase=i%6000;if(phase>350&&phase<2600){if(step==0)assert(std::abs(l-.4)<1e-6&&gate.dryGain()==0);if(step==1)assert(l==0&&gate.dryGain()==1);if(step==2)assert(l==0&&gate.dryGain()==0);}if(phase>3500)assert(l==0&&gate.dryGain()==0);}
 s.stepRandom=s.lengthRandom=true;s.chance=.5;s.state.fill(1);s.state[3]=2;s.length.fill(.9);gate.prepare(48000);gate.set(s);int changed=0,wet=0,off=0;double old=0;for(int cycle=0;cycle<12;++cycle){float l=1,r=1;gate.process(l,r,cycle*4.+.01,120);assert(gate.state(3)==2);for(int j=0;j<16;++j){assert(gate.length(j)>=.05&&gate.length(j)<=.9);wet+=gate.state(j)==1;off+=gate.state(j)==0;}changed+=gate.length(0)!=old;old=gate.length(0);gate.process(l,r,cycle*4.+.03,120);assert(gate.length(0)==old);}assert(changed>10&&wet>0&&off>0);
 float a=1,b=1;gate.process(a,b,.01,120);assert(gate.length(0)!=old);double loop=gate.length(0);gate.process(a,b,1.,120);gate.process(a,b,.01,120);assert(gate.length(0)!=loop);
 // Across all module orders a dry step bypasses filter and pitch; an off step mutes.
 for(int order=0;order<7;++order){qg::Engine e;e.prepare(8000);qg::Settings p;p.sampleRate=8000;p.order=order;p.mix=1;p.moduleOn={false,false,false};p.transpose=12;p.filterOn=true;p.filterType=1;p.filterCutoff=1;p.gater.enabled=true;p.gater.state.fill(2);p.gater.length.fill(1);e.set(p);float l=0,r=0;for(int i=0;i<800;++i)e.process(.3f,-.2f,i/4000.,l,r);assert(std::abs(l-.3)<1e-5&&std::abs(r+.2)<1e-5);p.gater.state.fill(0);e.set(p);for(int i=800;i<1800;++i)e.process(.3f,-.2f,i/4000.,l,r);assert(std::abs(l)<1e-5&&std::abs(r)<1e-5);}
 // A closed gate stops the source but leaves an already-excited reverb tail alive.
 {qg::Engine e;e.prepare(8000);qg::Settings p;p.sampleRate=8000;p.order=1;p.mix=1;p.moduleOn={false,false,false};p.gater.enabled=true;p.gater.state.fill(1);p.gater.length.fill(1);p.reverbOn=true;p.reverbMix=1;p.reverbLength=3;p.reverbPattern=0xffff;e.set(p);float l,r;for(int i=0;i<4000;++i)e.process(float(.3*std::sin(i*.12)),float(.3*std::sin(i*.12)),i/4000.,l,r);p.gater.state.fill(0);e.set(p);double energy=0;for(int i=4000;i<12000;++i){e.process(0,0,i/4000.,l,r);if(i>5000)energy+=l*l+r*r;}assert(energy>1e-6);}
 std::cout<<"PASS: transparent bypass, wet/dry/off routing, length, random stability, loop regeneration, all audio orders\n";
}
