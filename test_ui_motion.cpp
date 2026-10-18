#include "src/mockup_ui.h"
#include "src/engine.h"
#include "src/factory_presets.h"
#include "src/preset_io.h"
#include <cassert>
#include <iostream>
int main(){using namespace aztec;
 for(int n=0;n<120;++n)for(int from=0;from<4;++from)for(int gap=0;gap<=4;++gap){auto old=fourModuleOrder(n);auto actual=fourModuleOrder(int(std::round(mockup::moveRoute(old,from,gap)*120))-1);std::vector<int> expected(old.begin(),old.end());int moved=expected[from];expected.erase(expected.begin()+from);expected.insert(expected.begin()+gap-(gap>from),moved);assert(std::equal(expected.begin(),expected.end(),actual.begin()));}
 // Hosts may restore a stale landscape window or an unusual DPI size.
 for(auto size:std::vector<std::pair<double,double>>{{816,600},{600,1200},{758,1016},{1083,1452}}){
  mockup::Viewport fit(size.first,size.second);auto circle=fit.screen({100,200,80,80});assert(std::abs(circle.w-circle.h)<1e-9);
  auto logical=fit.logical(circle.x+circle.w/2,circle.y+circle.h/2);assert(std::abs(logical.first-140)<1e-9&&std::abs(logical.second-240)<1e-9);
  for(int page=0;page<3;++page){auto b=fit.screen(mockup::pageRect(page));auto xy=fit.logical(b.x+b.w*.4,b.y+b.h*.5);assert(mockup::hitPage(xy.first,xy.second)==page);}
 }
 auto p=initialParameters();auto value=[&](ParamID id){return p[id];};mockup::WaveVisual visual;
 for(int i=0;i<waveformBins;++i){p[kUiWaveLow0+i]=0;p[kUiWaveHigh0+i]=1;}p[kUiGrainActive]=1;
 visual.update(value);assert(std::abs(visual.hi[0]-.06)<1e-12&&std::abs(visual.lo[0]+.06)<1e-12&&visual.active==.06);
 for(int i=0;i<100;++i)visual.update(value);double previous=visual.hi[0];p[kUiWaveHigh0]=.5;visual.update(value);assert(previous-visual.hi[0]<.121);
 qg::Settings a;a.sampleRate=8000;a.feedback=0;a.bufferSeconds=1;a.pitch=-12;a.position=.02;
 auto b=a;b.pitch=24;b.position=.7;qg::Engine one,two;one.set(a);two.set(b);one.prepare(8000);two.prepare(8000);one.set(a);two.set(b);
 for(int n=0;n<12000;++n){float l,r,x=.2f*std::sin(n*.071);one.process(x,-x,n/4000.,l,r);two.process(x,-x,n/4000.,l,r);}
 auto w1=one.grainView(),w2=two.grainView();assert(w1.low==w2.low&&w1.high==w2.high&&w1.seconds==w2.seconds);
 mockup::RackState rack;assert(mockup::routeInsertion(100,100,value,rack)==-1);auto first=mockup::moduleRect(mockup::routeChain(value)[0],value,rack);assert(mockup::routeInsertion(first.x+100,first.y+5,value,rack)==0);mockup::scrollBy(rack,mockup::maxScroll(rack));auto last=mockup::moduleRect(mockup::routeChain(value)[3],value,rack);assert(mockup::routeInsertion(last.x+100,last.y+last.h-5,value,rack)==4);

 for(double rate:{1.5,.75}){qg::StepReverb reverb;reverb.prepare(8000);reverb.set(true,0,rate,0x5555,1.,1.,false,false,rate);
  for(int i=0;i<100;++i){double beat=i*.071;float l=0,r=0;reverb.process(l,r,beat);assert(reverb.gateOpen()==(int(std::floor(beat/rate))%2==0));}
  reverb.set(true,0,rate,0,1.,1.,false,true,rate);
  for(int i=0;i<100;++i){double beat=i*.071;float l=0,r=0;reverb.process(l,r,beat);assert(reverb.gateOpen()==qg::StepReverb::randomGate(int64_t(std::floor(beat/rate))));}
 }
 assert(reverbRate(5./6.,0.,false)==1.5&&reverbRate(1.,0.,true)==.75);
 qg::Gater gate;qg::GaterSettings gs;gs.enabled=true;gate.prepare(8000);gate.set(gs);float l=.1f,r=.1f;gate.process(l,r,.125,120.,true);assert(gate.phase()==.5);gate.process(l,r,.125,120.,false);assert(gate.phase()==0);
 mockup::Motion motion;motion.update(value,-1,-1);double oldY=motion.y[4];motion.last-=std::chrono::milliseconds(16);motion.update(value,3,0);assert(motion.y[4]<oldY&&motion.y[4]>430);

 p[kReverbRateV2]=5./6.;std::array<double,kCount> restored{};assert(decodePreset(encodePreset(value),restored));assert(restored[kReverbRateV2]==5./6.);
 std::cout<<"PASS: 3600 insertion moves, bounded waveform transitions, display independent of grain pitch/location, outside-drop cancellation\n";
}
