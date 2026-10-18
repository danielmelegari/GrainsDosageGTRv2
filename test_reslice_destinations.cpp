#include "src/parameter_settings.h"
#include "src/factory_presets.h"
#include "src/preset_io.h"
#include <cassert>
#include <vector>
#include <iostream>
using namespace aztec;
static std::vector<double> snapshot(const qg::ResliceSettings& s){return {
 double(s.enabled),s.beats,s.mix,double(s.algorithm),s.phraseBeats,s.repeat,s.variation,s.fill,s.reverse,double(s.seed),
 double(s.subdivision),s.fadeMs,s.minAmp,s.maxAmp,s.minPan,s.maxPan,s.minPitch,s.maxPitch,s.duty,s.fillDuty,s.minPhraseBeats,
 double(s.minRepeats),double(s.maxRepeats),s.stutter,s.area,s.straight,s.regular,s.ritard,s.warpSpeed,s.activity,
 double(s.crusher),double(s.minBits),double(s.maxBits),s.minFreq,s.maxFreq,double(s.comb),double(s.combType),s.combFeedback,s.minDelay,s.maxDelay};}
static void equal(const qg::ResliceSettings& a,const qg::ResliceSettings& b){auto x=snapshot(a),y=snapshot(b);for(size_t i=0;i<x.size();++i)assert(std::abs(x[i]-y[i])<1e-8);}
int main(){
 static_assert(qg::modTargetCount==101+extendedResliceCount);
 for(int i=0;i<extendedResliceCount;++i)for(double v:{0.,.2,.5,.8,1.}){
  auto p=initialParameters();auto base=settings(p,120,48000,3.);p[extendedResliceIds[i]]=v;auto direct=settings(p,120,48000,3.);
  if(i<extendedResliceCount-2){applyResliceDestination(base.reslice,i,v);equal(base.reslice,direct.reslice);}
  assert(availableMenuOption(slotTarget(0,0),102+i));
  p[lfoID(0,lEnabled)]=1;p[slotTarget(0,0)]=(102.+i)/qg::modTargetCount;p[slotAmount(0,0)]=1.;
  auto routed=settings(p,120,48000,3.);assert(routed.lfos[0].amount[101+i]==1.);assert(routed.resliceModBase[i]==v);
 }
 for(int i=0;i<int(sizeof(xyResliceIds)/sizeof(*xyResliceIds));++i){
  auto p=initialParameters();p[xyResliceIds[i]]=0.;p[kXYEnable]=1;p[kXYX]=1.;p[kXAmount]=1.;p[kYAmount]=.5;
  p[kXTarget]=(int(kXYX)+i+1.)/xyDestCount;auto morphed=settings(p,120,48000);
  p[kXYEnable]=0;p[xyResliceIds[i]]=1.;auto direct=settings(p,120,48000);
  equal(morphed.reslice,direct.reslice);assert(morphed.loopOn==direct.loopOn&&morphed.loopBeats==direct.loopBeats);
 }
 auto p=initialParameters();p[slotTarget(2,4)]=140./qg::modTargetCount;p[kXTarget]=1.;
 auto text=encodePreset([&](int id){return p[id];});std::array<double,kCount> loaded;assert(decodePreset(text,loaded));assert(loaded[slotTarget(2,4)]==p[slotTarget(2,4)]&&loaded[kXTarget]==1.);
 std::cout<<"All Reslice destinations map consistently in modulation, Morph and presets\n";
}
