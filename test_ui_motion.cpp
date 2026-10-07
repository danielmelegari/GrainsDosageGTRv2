#include "src/mockup_ui.h"
#include "src/engine.h"
#include "src/factory_presets.h"
#include <cassert>
#include <iostream>
int main(){using namespace aztec;
 for(int n=0;n<120;++n)for(int from=0;from<5;++from)for(int gap=0;gap<=5;++gap){auto old=fiveModuleOrder(n);auto actual=fiveModuleOrder(int(std::round(mockup::moveRoute(old,from,gap)*120))-1);std::vector<int> expected(old.begin(),old.end());int moved=expected[from];expected.erase(expected.begin()+from);expected.insert(expected.begin()+gap-(gap>from),moved);assert(std::equal(expected.begin(),expected.end(),actual.begin()));}
 auto p=initialParameters();auto value=[&](ParamID id){return p[id];};mockup::WaveVisual visual;
 for(int i=0;i<waveformBins;++i){p[kUiWaveLow0+i]=0;p[kUiWaveHigh0+i]=1;}p[kUiGrainActive]=1;
 visual.update(value);assert(std::abs(visual.hi[0]-.12)<1e-12&&std::abs(visual.lo[0]+.12)<1e-12&&visual.active==.12);
 for(int i=0;i<100;++i)visual.update(value);double previous=visual.hi[0];p[kUiWaveHigh0]=.5;visual.update(value);assert(previous-visual.hi[0]<.121);
 qg::Settings a;a.sampleRate=8000;a.feedback=0;a.bufferSeconds=1;a.pitch=-12;a.position=.02;
 auto b=a;b.pitch=24;b.position=.7;qg::Engine one,two;one.set(a);two.set(b);one.prepare(8000);two.prepare(8000);one.set(a);two.set(b);
 for(int n=0;n<12000;++n){float l,r,x=.2f*std::sin(n*.071);one.process(x,-x,n/4000.,l,r);two.process(x,-x,n/4000.,l,r);}
 auto w1=one.grainView(),w2=two.grainView();assert(w1.low==w2.low&&w1.high==w2.high&&w1.seconds==w2.seconds);
 assert(mockup::routeInsertion(100,100)==-1&&mockup::routeInsertion(197,1556)==0&&mockup::routeInsertion(1597,1556)==5);
 std::cout<<"PASS: 3600 insertion moves, bounded waveform transitions, display independent of grain pitch/location, outside-drop cancellation\n";
}
