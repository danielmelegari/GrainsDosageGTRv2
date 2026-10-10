#include "src/reslice.h"
#include "src/parameter_settings.h"
#include "src/factory_presets.h"
#include <cassert>
#include <iostream>
#include <vector>
using namespace qg;
static std::vector<float> render(ResliceSettings s){Reslice e;e.prepare(8000);e.set(s);std::vector<float> out(128000);for(int i=0;i<int(out.size());++i){float l=.3f*std::sin(i*.17)+.1f*std::sin(i*.039),r=l;e.process(l,r,i/4000.,120);assert(std::isfinite(l)&&std::isfinite(r)&&std::abs(l)<2.);out[i]=l;}return out;}
static void different(const std::vector<float>& a,const std::vector<float>& b){double d=0;for(size_t i=8000;i<a.size();++i)d+=std::abs(a[i]-b[i]);assert(d>1.);}
int main(){
 ResliceSettings s;s.enabled=true;s.variation=.9;s.fill=1;auto original=render(s);
 for(int n=0;n<12;++n){auto v=s;switch(n){case 0:v.subdivision=32;break;case 1:v.fadeMs=40;break;case 2:v.minAmp=v.maxAmp=.2;break;case 3:v.minPan=v.maxPan=.8;break;case 4:v.minPitch=v.maxPitch=1200;break;case 5:v.duty=.1;break;case 6:v.fillDuty=0;break;case 7:v.minRepeats=v.maxRepeats=8;break;case 8:v.stutter=1;break;case 9:v.area=0;break;case 10:v.crusher=true;v.minBits=v.maxBits=2;break;case 11:v.comb=true;v.combType=1;v.combFeedback=.9;break;}different(original,render(v));}
 s.algorithm=1;original=render(s);for(int n=0;n<4;++n){auto v=s;switch(n){case 0:v.straight=1;break;case 1:v.regular=1;break;case 2:v.ritard=1;break;case 3:v.warpSpeed=0;break;}different(original,render(v));}
 s.algorithm=2;original=render(s);s.activity=0;different(original,render(s));
 s.crusher=true;s.minFreq=s.maxFreq=100;different(original,render(s));
 // Range endpoints may be reversed by independent automation. All remain finite.
 s.minAmp=2;s.maxAmp=0;s.minPan=1;s.maxPan=-1;s.minPitch=2400;s.maxPitch=-2400;s.minBits=32;s.maxBits=1;s.minFreq=48000;s.maxFreq=100;s.comb=true;s.minDelay=100;s.maxDelay=1;s.combFeedback=.95;s.minPhraseBeats=32;s.phraseBeats=4;
 render(s);
 auto p=aztec::initialParameters();for(const auto& spec:aztec::resliceControlSpecs){assert(p[spec.id]>=0&&p[spec.id]<=1);assert(std::abs(aztec::reslicePlain(p,spec.id)-spec.initial)<1e-8);}
 auto settings=aztec::settings(p,120,48000);assert(settings.reslice.minAmp==1&&settings.reslice.duty==1&&settings.reslice.minBits==32&&!settings.reslice.comb&&!settings.reslice.crusher);
 std::cout<<"PASS: global/mode controls change sound, crusher/comb, reversed ranges and parameter defaults\n";
}
