#include "src/engine.h"
#include "src/master_fx.h"
#include "src/preset_io.h"
#include "src/factory_presets.h"
#include <cassert>
#include <iostream>
#include <array>
static double response(int slope,int type,double hz){qg::MasterFx f;f.prepare(48000);f.set(true,type,1000,0,false,slope,0);double sum=0;for(int i=0;i<96000;++i){float l=std::sin(2*qg::pi*hz*i/48000),r=l;f.process(l,r);if(i>48000)sum+=l*l;}return std::sqrt(sum/48000);}
int main(){
 qg::LevelMatch m;m.prepare(48000);double sumD=0,sumW=0;for(int i=0;i<480000;++i){double d=.2*std::sin(i*.1),l=d*.3,r=l;m.process(d,d,l,r,true);if(i>400000){sumD+=d*d;sumW+=l*l;}}assert(std::abs(10*std::log10(sumW/sumD))<.15);
 assert(response(1,0,8000)<response(0,0,8000)*.1);assert(response(1,1,125)<response(0,1,125)*.1);
 qg::MasterFx clean,drive;clean.prepare(48000);drive.prepare(48000);clean.set(true,0,18000,0,false,0,0);drive.set(true,0,18000,0,false,0,18);double diff=0;for(int i=0;i<48000;++i){float a=.2*std::sin(i*.1),b=a,c=a,d=a;clean.process(a,b);drive.process(c,d);if(i>20000)diff+=std::abs(a-c);}assert(diff>100.);
 for(int order=0;order<7;++order){qg::Engine e;e.prepare(8000);qg::Settings s;s.sampleRate=8000;s.order=order;s.moduleOn={{false,false,false}};s.grainMix=1;s.glitchMix=1;s.repeatMix=1;s.repeatOn=true;e.set(s);float l=0,r=0;for(int i=0;i<16000;++i)e.process(.2f,-.1f,i/4000.,l,r);assert(std::abs(l-.2)<1e-6&&std::abs(r+.1)<1e-6);}
 for(double pan:{-1.,1.}){qg::Engine e;e.prepare(8000);qg::Settings s;s.sampleRate=8000;s.order=0;s.pattern=0xffff;s.densityFlow=true;s.density=8;s.grainPan=pan;e.set(s);double el=0,er=0;for(int i=0;i<24000;++i){float l,r;e.process(.1f,.1f,i/4000.,l,r);if(i>8000){el+=l*l;er+=r*r;}}assert(pan<0?(el>1&&er<1e-12):(er>1&&el<1e-12));}
 // The master dry path stays intact even with wet stretch/transpose/reverb on.
 {qg::Engine e;e.prepare(8000);qg::Settings s;s.sampleRate=8000;s.order=0;s.mix=0;s.stretchOn=true;s.stretchSpeed=.5;s.transpose=12;s.reverbOn=true;s.normalize=true;e.set(s);for(int i=0;i<16000;++i){float d=float(.2*std::sin(i*.1)),l,r;e.process(d,-d,i/4000.,l,r);assert(l==d&&r==-d);}}
 std::array<double,5> energies{};
 for(int type=0;type<5;++type){qg::StepReverb rv;rv.prepare(8000);rv.set(true,type,.25,1,1.);double energy=0;for(int i=0;i<48000;++i){float l=i==100?1.f:0.f,r=0;rv.process(l,r,i/4000.);assert(std::isfinite(l)&&std::isfinite(r));if(i>2000)energy+=l*l+r*r;}assert(energy>1e-8);energies[type]=energy;}
 for(int i=1;i<5;++i)assert(std::abs(energies[i]-energies[i-1])>1e-6);
 for(double grid:{1.,.5,.25,.125}){qg::StepReverb rv;rv.prepare(8000);rv.set(true,2,grid,0,1.);for(int i=0;i<8000;++i){float l=.1f,r=.2f;rv.process(l,r,i/4000.);assert(l==.1f&&r==.2f);}}
 // A closed sequencer step leaves an audible tail; Length extends its decay.
 auto tail=[](double length){qg::StepReverb rv;rv.prepare(8000);rv.set(true,2,.25,1,1.,length);double sum=0;for(int i=0;i<24000;++i){float l=i==100?1.f:0.f,r=0;rv.process(l,r,i<1000?i/4000.:.3);if(i>8000)sum+=l*l+r*r;}return sum;};
 assert(tail(6.)>tail(.3)*100.);assert(tail(6.)>1e-8);
 // Transport loops must not clear the reverb return.
 {qg::Engine e;e.prepare(8000);qg::Settings s;s.sampleRate=8000;s.order=0;s.grainMix=0.;s.reverbOn=true;s.reverbType=2;s.reverbLength=6.;s.reverbMix=1.;s.reverbPattern=1;e.set(s);double tailEnergy=0;for(int i=0;i<24000;++i){if(i==1000)e.resetTransport();float l,r;e.process(i==100?1.f:0.f,0.f,i<1000?i/4000.:.3,l,r);if(i>8000)tailEnergy+=l*l+r*r;}assert(tailEnergy>1e-8);}
 // Presets preserve all editable values, reject malformed input transactionally.
 std::array<double,aztec::kCount> original{},loaded{};for(int i=0;i<aztec::kCount;++i)original[i]=(i%101)/100.;
 auto text=aztec::encodePreset([&](int i){return original[i];});assert(aztec::decodePreset(text,loaded));
 for(int i=0;i<aztec::kCount;++i)if(aztec::presetParameter(i))assert(loaded[i]==original[i]);
 auto before=loaded;assert(!aztec::decodePreset(text.substr(0,text.size()/2),loaded)&&loaded==before);
 assert(!aztec::decodePreset(text+"garbage",loaded)&&loaded==before);
 assert(!aztec::decodePreset(std::string(70000,'x'),loaded)&&loaded==before);
 // Actual grain buffer and active source span, not a decorative oscillator.
 {qg::Engine e;e.prepare(8000);qg::Settings s;s.sampleRate=8000;s.order=0;s.densityFlow=true;s.density=8;s.pattern=0xffff;e.set(s);for(int i=0;i<24000;++i){float l,r;e.process(.2f,.1f,i/4000.,l,r);}auto v=e.grainView();assert(v.active&&v.start<=v.head&&v.head<=v.end&&std::abs((v.end-v.start)*v.seconds-s.size)<.001);assert(v.head>.5&&v.head<=1.);int filled=0,empty=0;for(auto a:v.amplitude){assert(a>=0.&&a<=.200001);filled+=a>.19;empty+=a==0.;}assert(filled>40&&empty>10);for(int i=0;i<100;++i){float l,r;e.process(.4f,.1f,(24000+i)/4000.,l,r);}auto moved=e.grainView();assert(moved.head>=moved.start&&moved.head<=moved.end);assert(moved.amplitude!=v.amplitude);s.moduleOn[0]=false;e.set(s);assert(!e.grainView().active);}
 std::string previousPreset;for(int n=0;n<aztec::factoryPresetCount;++n){auto p=aztec::factoryPreset(n);if(n>=10&&n<19){assert(p[aztec::kInputDeclick]==0.);assert(p[aztec::kFilterModel]==0.);assert(p[aztec::kResliceEnabled]==0.);assert(p[aztec::kGaterLatch]==0.);if(n==10)assert(std::abs(p[aztec::kSize]-.95347222222222205)<1e-12);}for(auto v:p)assert(std::isfinite(v)&&v>=0&&v<=1.);auto bytes=aztec::encodePreset([&](int i){return p[i];});assert(bytes!=previousPreset);previousPreset=bytes;std::array<double,aztec::kCount> decoded{};assert(aztec::decodePreset(bytes,decoded));for(int i=0;i<aztec::presetCount;++i)if(aztec::presetParameter(i))assert(decoded[i]==p[i]);}
 // Reslice switches from the live input to slices recorded after the modules and follows host PPQ.
 {qg::Engine e;e.prepare(8000);qg::Settings s;s.sampleRate=8000;s.order=0;s.moduleOn={{false,false,false}};s.mix=1.;s.reslice.enabled=true;s.reslice.beats=1.;s.reslice.on.fill(true);s.reslice.slice.fill(0);e.set(s);float before=0.,after=0.;for(int i=0;i<9000;++i){float in=float(i/16000.),l,r;e.process(in,-in,i/4000.,l,r);if(i==3900)before=l;if(i==4100)after=l;}assert(std::abs(before-after)>.1);s.reslice.enabled=false;e.set(s);float a=.31f,b=-.27f;for(int i=0;i<100;++i)e.process(a,b,2.+i/4000.,a,b);assert(std::abs(a-.31f)<1e-5&&std::abs(b+.27f)<1e-5);}
 // The new reverbs keep tails alive with the send sequencer closed.
 for(int model=1;model<=4;++model){qg::SpaceReverb r;r.prepare(8000);r.set(true,2,.25,0xFFFF,1.,4.,false,false,.25,model);double tail=0.;for(int i=0;i<24000;++i){float l=i<2000?.3f:0.,rr=i<2000?.2f:0.;r.process(l,rr,i/4000.);if(i>8000)tail+=l*l+rr*rr;}assert(tail>1e-6);}
 // Quarter-speed timing remains independent of all 64 random points.
 {qg::Lfo slow,normal;slow.prepare(1);normal.prepare(1);qg::LfoSettings a;a.enabled=true;a.wave=128;a.randomSteps=64;a.beats=4;a.speed=.25;auto b=a;b.speed=1.;slow.process(a,1.,48000.,false,0);normal.process(b,1.,48000.,false,0);assert(std::abs(slow.phase()-.0625)<1e-12&&std::abs(normal.phase()-.25)<1e-12);}
 // Kill Dry cannot leak the source, including via the master dry/wet control.
 {qg::Engine e;e.prepare(8000);qg::Settings s;s.sampleRate=8000;s.order=0;s.grainMix=0;s.mix=0;s.reverbOn=true;s.reverbKill=true;s.reverbPattern=0;s.reverbMix=1;e.set(s);float l=0,r=0;for(int i=0;i<8000;++i)e.process(.3f,.2f,i/4000.,l,r);assert(std::abs(l)<1e-6&&std::abs(r)<1e-6);}
 // Filtering the send does not cut a pre-existing reverb tail.
 auto filteredTail=[](bool change){qg::Engine e;e.prepare(8000);qg::Settings s;s.sampleRate=8000;s.order=0;s.grainMix=0;s.reverbOn=true;s.reverbMix=1;s.reverbLength=6;s.reverbPattern=1;e.set(s);double energy=0;for(int i=0;i<24000;++i){if(i==1000&&change){s.filterOn=true;s.filterType=1;s.filterCutoff=1.;e.set(s);}float l,r;e.process(i==100?1.f:0.f,0.f,i<1000?i/4000.:.3,l,r);if(i>4000)energy+=l*l+r*r;}return energy;};assert(std::abs(filteredTail(true)-filteredTail(false))<1e-10);
 // Cutoff modulation changes the actual filtered audio.
 auto filterSound=[](double amount){qg::Engine e;e.prepare(8000);qg::Settings s;s.sampleRate=8000;s.order=0;s.grainMix=0;s.filterOn=true;s.filterCutoff=.5;s.lfos[0].enabled=true;s.lfos[0].amount[8]=amount;e.set(s);double energy=0;for(int i=0;i<32000;++i){float l,r;e.process(float(.2*std::sin(i*1.3)),0.f,i/4000.,l,r);energy+=l*l;}return energy;};assert(std::abs(filterSound(.4)-filterSound(0.))>1.);
 // Wave randomization follows the musical clock independently of Mod speed.
 {for(int mode=1;mode<=4;++mode){qg::Lfo a,b;a.prepare(123);b.prepare(456);qg::LfoSettings s;s.enabled=true;s.wave=128;s.waveRandom=mode;s.speed=.25;double interval=4./std::pow(2.,mode-1);int changed=0,previous=-1,different=0;
 for(int step=-2;step<20;++step){a.process(s,(step+.1)*interval,48000,false,0);int chosen=a.wave();assert(chosen>=0&&chosen<128);a.process(s,(step+.9)*interval,48000,false,0);assert(a.wave()==chosen);b.process(s,(step+.1)*interval,48000,false,0);different+=a.wave()!=b.wave();changed+=previous!=chosen;previous=chosen;}assert(changed>15&&different>15);s.waveRandom=0;a.process(s,0.,48000,false,0);assert(a.wave()==128);}}
 // Changing transpose retains a continuous, finite stereo output and settles at neutral.
 {qg::Transpose pitch;pitch.prepare(48000);float previous=0;double peakJump=0;for(int i=0;i<144000;++i){double semitones=i<24000?0:i<48000?12:i<72000?-12:0;float input=float(.2*std::sin(qg::tau*220*i/48000.)),l=input,r=input;pitch.process(l,r,semitones);assert(std::isfinite(l)&&l==r);peakJump=std::max(peakJump,double(std::abs(l-previous)));previous=l;if(i>120000)assert(std::abs(l-input)<1e-6);}assert(peakJump<.04);}
 // Random Impulse ignores the step pattern, holds each random gate for its grid,
 // and preserves the return/tail when the send closes or the mode changes.
 {for(double grid:{1.,.5,.25}){qg::StepReverb a,b;a.prepare(8000);b.prepare(8000);a.set(true,2,.125,0,1.,4.,true,true,grid);b.set(true,2,.125,0xffff,1.,4.,true,true,grid);int opened=0,closed=0;double tail=0.;for(int i=0;i<64000;++i){double beat=i/4000.;float l=i<8000?.2f:0.f,r=l,u=l,v=r;a.process(l,r,beat);b.process(u,v,beat);assert(l==u&&r==v);assert(a.gateOpen()==qg::StepReverb::randomGate(int64_t(std::floor(beat/grid+1e-8))));opened+=a.gateOpen();closed+=!a.gateOpen();if(i>9000&&!a.gateOpen())tail+=l*l+r*r;}assert(opened>0&&closed>0&&tail>1e-8);a.set(true,2,grid,0,1.,4.,true,false,grid);double continued=0.;for(int i=0;i<4000;++i){float l=0,r=0;a.process(l,r,16.+i/4000.);assert(!a.gateOpen());continued+=l*l+r*r;}assert(continued>0.);}}
 // Automatic wave glide starts at the prior output; Glide controls transition time.
 {qg::Lfo a;a.prepare(123);qg::LfoSettings s;s.enabled=true;s.sync=true;s.beats=1.;double before=a.process(s,.75,48000,false,0);auto b=a;s.waveRandom=4;s.glide=1.;double first=a.process(s,.5,48000,false,0);assert(std::abs(first-before)<1e-5);auto fast=s;fast.glide=0.;double slowOut=first,fastOut=before;for(int i=0;i<240;++i){slowOut=a.process(s,.5,48000,false,0);fastOut=b.process(fast,.5,48000,false,0);}assert(std::abs(slowOut-before)<std::abs(fastOut-before));}
 std::cout<<"PASS: normalize, modelled filter banks, Reslice host clock/capture, five additional reverbs, all 30 factory patches including nine imported, dry and module-order regressions\n";
}

