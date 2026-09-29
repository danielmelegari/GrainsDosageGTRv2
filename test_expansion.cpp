#include "src/filter_bank.h"
#include "src/reslice.h"
#include "src/gater.h"
#include <cassert>
#include <iostream>
#include <vector>
#include <cmath>
int main(){
 // Legacy dispatch remains bit-identical, including automation.
 qg::MasterFx legacy;qg::FilterBank bank;legacy.prepare(48000);bank.prepare(48000);
 for(int i=0;i<30000;++i){double fc=20+19000.*(i%1000)/1000.;legacy.set(true,(i/500)%3,fc,.5,false,(i/1000)%2,4);bank.set(true,(i/500)%3,fc,.5,false,(i/1000)%2,4,0,0);float l=.2f*std::sin(i*.17),r=.15f*std::cos(i*.23),a=l,b=r;legacy.process(l,r);bank.process(a,b);assert(l==a&&r==b);}
 // Exercise every model at extreme control values without relying on the limiter.
 for(double sr:{8000.,48000.,192000.}){bank.prepare(sr);for(int model=0;model<qg::filterModelCount;++model){double energy=0;for(int i=0;i<12000;++i){double fc=i<6000?std::min(1500.,sr*.2):20.+(sr*.4-20)*(i%500)/500.;bank.set(true,0,fc,i<6000?.5:1.,false,1,i<6000?0.:24.,0,model);float l=.2f*std::sin(i*.093),r=.1f*std::cos(i*.13);bank.process(l,r);assert(std::isfinite(l)&&std::isfinite(r));assert(std::abs(l)<100&&std::abs(r)<100);if(i>2000&&i<5000)energy+=l*l+r*r;}assert(energy>1e-10);}}
 // Every disabled new filter settles to an exact clean path.
 for(int model=1;model<qg::filterModelCount;++model){bank.prepare(48000);bank.set(false,0,1000,.8,false,1,24,0,model);for(int i=0;i<3000;++i){float l=.4f,r=-.2f;bank.process(l,r);if(i>1000){assert(l==.4f&&r==-.2f);}}}
 qg::Gater gate;qg::GaterSettings gs;gs.enabled=gs.latch=true;gs.state.fill(0);gs.state[0]=1;gs.state[3]=1;gs.release=1<<6;gs.sustain.fill(.01);gate.prepare(8000);gate.set(gs);
 for(int i=0;i<8000;++i){float l=1,r=1;gate.process(l,r,i/4000.,120);int step=i/1000;if(step<6&&i%1000>100)assert(l>.99&&r>.99);if(step>=7&&i%1000>200)assert(l==0&&r==0);}
 // Hold across sequencer wrap; close on stop, known closed state after seek.
 gs.state.fill(0);gs.state[15]=1;gs.release=0;gate.prepare(8000);gate.set(gs);for(int i=0;i<18000;++i){float l=1,r=1;gate.process(l,r,i/4000.,120);if(i>15100)assert(l==1);}for(int i=0;i<120;++i){float l=1,r=1;gate.process(l,r,4.5,120,false);if(i>100)assert(l==0);}gate.resetLatch();for(int i=0;i<120;++i){float l=1,r=1;gate.process(l,r,1+i/4000.,120,true);if(i>100)assert(l==0);}
 qg::Reslice slice;qg::ResliceSettings rs;slice.prepare(8000);slice.set(rs);for(int i=0;i<1000;++i){float l=float(i*.0001),r=-l,a=l,b=r;slice.process(l,r,i/4000.,120);assert(l==a&&r==b);}
 // Capture previous musical window, rearrange slices, and re-align to host PPQ.
 rs.enabled=true;rs.beats=1.;rs.on.fill(true);rs.slice.fill(0);slice.prepare(8000);slice.set(rs);std::vector<float> outputs(12000);for(int i=0;i<12000;++i){float l=float(i/16000.),r=-l;slice.process(l,r,i/4000.,120);outputs[i]=l;assert(std::isfinite(l)&&std::abs(l+r)<1e-6);}assert(std::abs(outputs[4100]-(outputs[8100]-.25))<.002);assert(outputs[4350]<.03);assert(outputs[8100]>.24&&outputs[8100]<.29);
 for(int i=0;i<200;++i){float l=.3f,r=-.2f;slice.process(l,r,.5+i/4000.,120);if(i>100)assert(std::abs(l-.3f)<1e-6);}assert(!slice.active());
 // 1/3 is represented as 4/3 beats; fractional step boundaries stay deterministic.
 rs.beats=4./3.;slice.set(rs);for(int i=0;i<500;++i){double beat=i*.001;float l=.2f,r=.2f;slice.process(l,r,beat,137);int expected=int(std::floor(beat/(rs.beats/16.)+1e-9))%16;assert(slice.step()==expected);}float l=.2f,r=.2f;slice.process(l,r,0,137,false);assert(!slice.active());
 std::cout<<"PASS: legacy filter equivalence, 19 models finite/active/bypass; Gater Wet/Off/Release latch/wrap/stop; Reslice bypass/capture/reorder/seek/rational clock\n";
}
