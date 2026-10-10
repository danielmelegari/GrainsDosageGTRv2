#include "src/engine.h"
#include <cassert>
#include <iostream>
// Input processing must be identical to applying the same upstream processors
// before feeding an otherwise neutral engine, across every four-module order.
int main(){
 for(int route:{0,5,23,47,71,95,119,-1})for(bool stretch:{false,true}){
  qg::Settings s;s.sampleRate=8000;s.routing=route;s.order=route<0?6:0;s.pattern=0xffff;s.densityFlow=true;s.moduleOn={true,false,true};s.grainMix=.65;s.repeatMix=.7;s.repeatOn=true;s.repeatBeats=.5;
  s.reslice.enabled=true;s.reslice.algorithm=1;s.reslice.seed=31;s.gater.enabled=true;s.gater.state.fill(1);s.transpose=12;s.stretchOn=stretch;s.stretchSpeed=.7;s.mix=1;s.normalize=false;
  qg::Engine actual,reference;actual.prepare(8000);reference.prepare(8000);actual.set(s);auto neutral=s;neutral.transpose=0;neutral.stretchOn=false;reference.set(neutral);
  qg::FreeStretch preStretch;qg::Transpose preTranspose;preStretch.prepare(8000);preTranspose.prepare(8000);
  double energy=0;for(int i=0;i<32000;++i){float input=.2f*std::sin(i*.17)+.05f*std::sin(i*.043),pl=input,pr=-input;preStretch.process(pl,pr,s.stretchOn,s.stretchSpeed);preTranspose.process(pl,pr,s.transpose);float a,b,x,y;actual.process(input,-input,i/4000.,a,b);reference.process(pl,pr,i/4000.,x,y);assert(std::isfinite(a)&&std::isfinite(b));assert(std::abs(a-x)<1e-6&&std::abs(b-y)<1e-6);energy+=a*a;}
  assert(energy>1);
 }
 // Global dry remains the original input even with Input Options engaged.
 qg::Engine e;e.prepare(8000);qg::Settings s;s.sampleRate=8000;s.mix=0;s.transpose=12;s.stretchOn=true;s.stretchSpeed=.5;e.set(s);
 for(int i=0;i<10000;++i){float input=.2f*std::sin(i*.13),l,r;e.process(input,-input,i/4000.,l,r);assert(l==input&&r==-input);}
 std::cout<<"PASS: Input Options precede all captures/modules across serial and legacy parallel routes; original dry path preserved\n";
}
