#include "src/parameter_settings.h"
#include "src/preset_io.h"
#include "src/mockup_ui.h"
#include <cassert>
#include <iostream>
using namespace aztec;
int main(){
 // Quality is persistent, defaults for old presets, and is never an LFO target.
 auto p=initialParameters();assert(p[kAudioQuality]==.5);
 for(int q=0;q<3;++q){p[kAudioQuality]=q*.5;assert(int(settings(p,120,48000).quality)==q);
  std::array<double,kCount> restored{};assert(decodePreset(encodePreset([&](int id){return p[id];}),restored));assert(restored[kAudioQuality]==p[kAudioQuality]);}
 std::ostringstream old;old<<"GrainsDosagePreset 2 "<<kAudioQuality<<'\n';for(int i=0;i<kAudioQuality;++i)old<<i<<' '<<p[i]<<'\n';
 assert(decodePreset(old.str(),p)&&p[kAudioQuality]==.5);
 mockup::RackState rack;rack.page=1;auto controls=mockup::controls([&](ParamID id){return p[id];},0,0,0,0,0,rack);int found=0;
 for(auto c:controls)if(c.id==kAudioQuality){assert(c.kind==mockup::Select);++found;}assert(found==1);
 // Sparse routes must exactly match the original dense accumulation, including
 // duplicate destinations, signed/unipolar amounts, and disabled LFOs.
 qg::Modulation sparse;sparse.prepare();std::array<qg::Lfo,4> reference;
 std::array<qg::LfoSettings,4> lfos{};qg::ModValues base{};
 for(int i=0;i<4;++i){reference[i].prepare(0x13579BDFULL+uint64_t(i)*104729);lfos[i].enabled=i!=2;lfos[i].wave=i*40;lfos[i].depth=.7;
  lfos[i].amount[4]=.2;lfos[i].amount[101+i]=-.3;lfos[i].positive[6]=.4;lfos[i].negative[6]=.1;}
 base.fill(.5);sparse.configure(lfos);
 for(int n=0;n<4096;++n){double beat=n/24000.;auto expected=base;for(int i=0;i<4;++i){double v=reference[i].process(lfos[i],beat,48000,n==0,0);if(!lfos[i].enabled)continue;double u=(v+lfos[i].depth)*.5;
   for(int t=0;t<qg::modTargetCount;++t)expected[t]+=v*lfos[i].amount[t]+u*(lfos[i].positive[t]-lfos[i].negative[t]);}
  for(auto& v:expected)v=std::clamp(v,0.,1.);assert(sparse.process(lfos,base,beat,48000,n==0,0,true)==expected);}
 // Every filter/quality pair must stay finite during rapid cutoff/resonance
 // changes and quality switching, including high-rate and low-rate hosts.
 for(double rate:{44100.,96000.})for(int model=0;model<qg::filterModelCount;++model){qg::FilterBank f;f.prepare(rate);
  for(int n=0;n<16000;++n){f.setQuality(qg::audioQuality((n/1000)%3));f.set(true,0,40.+(rate*.35-40.)*(.5+.5*std::sin(n*.005)),.85,false,1,6.,0.,model);
   float l=.1f*std::sin(n*.007),r=l;f.process(l,r);assert(std::isfinite(l)&&std::isfinite(r)&&std::abs(l)<20&&std::abs(r)<20);}}
 // Bass/grain overlap: retiring silent grains must not step the gain of the
 // grains that are still sounding. This fails before normalization smoothing.
 for(int route:{-1,0}){qg::Engine e;e.prepare(48000);qg::Settings s;s.sampleRate=48000;s.routing=route;s.order=route<0?6:0;s.pattern=0xffff;s.size=.25;s.density=8;s.chaos=.4;s.position=.1;s.moduleOn={{true,false,false}};s.quality=qg::AudioQuality::High;e.set(s);
  double previous=0,maxDelta=0;for(int n=0;n<144000;++n){float l,r,x=.5*std::sin(n*6.283185307179586*30/48000);e.process(x,x,n/24000.,l,r);if(n>48000)maxDelta=std::max(maxDelta,std::abs(l-previous));previous=l;}
  assert(maxDelta<.02);
 }
 // Switching tiers does not abruptly drop active grains or change latency.
 for(int q=0;q<3;++q){qg::Engine changed,referenceEngine;changed.prepare(48000);referenceEngine.prepare(48000);qg::Settings s;s.sampleRate=48000;s.routing=0;s.pattern=0xffff;s.densityFlow=true;s.density=32;s.size=.25;s.quality=qg::AudioQuality::High;
  changed.set(s);referenceEngine.set(s);
  for(int n=0;n<36000;++n){if(n==24000){s.quality=qg::audioQuality(q);changed.set(s);}float x=.2*std::sin(n*.007),l,r,a,b;changed.process(x,x,n/24000.,l,r);referenceEngine.process(x,x,n/24000.,a,b);
   assert(std::isfinite(l)&&std::isfinite(r));if(n>=24000&&n<24032)assert(std::abs(l-a)<.001);}}
 std::cout<<"PASS: quality presets/UI, exact sparse routes, filter stability, bass continuity and live tier switching\n";
}
