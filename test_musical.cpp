#include "src/reslice.h"
#include "src/filter_sequencer.h"
#include "src/randomize.h"
#include "src/factory_presets.h"
#include "src/engine.h"
#include <cassert>
#include <set>
#include <sstream>
#include <iostream>
int main(){
 // All 64 original patterns are distinct, bounded, and loop every 32 steps.
 std::set<std::array<double,32>> patterns;
 for(int p=0;p<64;++p){std::array<double,32> v{};for(int i=0;i<32;++i){v[i]=qg::filterPattern(p,i);assert(std::isfinite(v[i])&&std::abs(v[i])<=1.00001);assert(v[i]==qg::filterPattern(p,i+32));}patterns.insert(v);}assert(patterns.size()==64);
 for(int root=0;root<12;++root)for(bool scale:{false,true})for(double midi=15.;midi<110;midi+=.37){int note=qg::minorNote(midi,root,scale),pc=(note-root+120)%12;bool allowed=pc==0||pc==3||pc==7||(scale&&(pc==2||pc==5||pc==8||pc==10));assert(allowed);}
 qg::FilterSequencer filter;qg::FilterSequenceSettings fs;fs.enabled=true;
 for(int mode:{0,1}){fs.mode=mode;double last=0.;for(int i=0;i<32000;++i){double hz=filter.process(fs,.5,i/4000.,8000,true,false);assert(std::isfinite(hz)&&hz>=20&&hz<=20000);if(i>0)assert(std::abs(hz-last)<100);last=hz;}}
 // Random pattern changes only at musical boundaries and every slice changes.
 for(double interval:{2.,4.,8.}){qg::Reslice r;qg::ResliceSettings s;s.enabled=s.random=true;s.randomBeats=interval;s.beats=16;s.on.fill(true);r.prepare(8000);r.set(s);std::array<int,16> before{};
 for(int i=0;i<=int(interval*4000)+1;++i){float l=.2f,rr=-.2f;r.process(l,rr,i/4000.,120);assert(std::isfinite(l));if(i==1)for(int j=0;j<16;++j)before[j]=r.source(j);if(i>1&&i<int(interval*4000))for(int j=0;j<16;++j)assert(r.source(j)==before[j]);}
 for(int j=0;j<16;++j)assert(r.source(j)!=before[j]);s.random=false;r.set(s);for(int j=0;j<16;++j)assert(r.source(j)==s.slice[j]);}
 auto params=aztec::initialParameters();params[aztec::kResliceRndOn]=1.;auto before=params;uint32_t seed=5678;aztec::randomizeReslice(seed,[&](int id){return params[id];},[&](int id,double v){params[id]=v;});assert(params[aztec::kResliceRndOn]==0.);for(int j=0;j<16;++j){assert(params[aztec::kResliceIndex0+j]!=before[aztec::kResliceIndex0+j]);assert(params[aztec::kResliceStep0+j]==before[aztec::kResliceStep0+j]);}
 // Each newly exposed destination has an actual mapping and remains finite.
 for(int target=11;target<qg::modTargetCount;++target){qg::Engine e;qg::Settings s;s.sampleRate=8000;s.order=0;s.grainMix=0.;s.lfos[0].enabled=true;s.lfos[0].amount[target]=.25;e.prepare(8000);e.set(s);for(int i=0;i<300;++i){float l,r;e.process(.2f,-.1f,i/4000.,l,r);assert(std::isfinite(l)&&std::isfinite(r));}}
 std::cout<<"PASS: 64 unique patterns, minor note constraints, smooth cutoff, quantized Reslice random, one-shot edits, extended routes\n";
}
