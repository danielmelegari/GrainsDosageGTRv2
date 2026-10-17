#include "src/parameter_settings.h"
#include "src/preset_catalog.h"
#include <cassert>
#include <cstring>
#include <set>
#include <iostream>
using namespace aztec;
int main(){
 const auto& tables=qg::waveBank();std::set<uint64_t> hashes;
 for(int n=0;n<qg::tableWaveCount;++n){uint64_t hash=1469598103934665603ULL;double lo=1,hi=-1;
  for(int i=0;i<qg::tableSize;++i){float v=tables[n][i];assert(std::isfinite(v)&&v>=-1&&v<=1);lo=std::min(lo,double(v));hi=std::max(hi,double(v));uint32_t bits;std::memcpy(&bits,&v,4);hash=(hash^bits)*1099511628211ULL;}
  assert(hi-lo>.1);assert(tables[n][0]==tables[n][qg::tableSize]);if(n>=128)assert(hashes.insert(hash).second);else hashes.insert(hash);
 }
 auto p=initialParameters();for(int w=0;w<130;++w){p[lfoID(0,lWave)]=w/129.;auto s=settings(p,120,48000);assert(s.lfos[0].wave==w&&s.lfos[0].randomWaveCount==qg::tableWaveCount);}
 for(int w=130;w<=qg::maxWave;++w){p[kLfoWaveChoice0]=(w+1.)/qg::waveChoiceCount;auto s=settings(p,120,48000);assert(s.lfos[0].wave==w&&s.lfos[0].randomWaveCount==328);qg::Lfo l;l.prepare(13);s.lfos[0].enabled=true;
  for(int n=0;n<100;++n){double v=l.process(s.lfos[0],n*.07,48000,false,0);assert(std::isfinite(v)&&v>=-1&&v<=1);assert(l.wave()==w);}
  std::array<double,kCount> restored{};assert(decodePreset(encodePreset([&](int id){return p[id];}),restored));assert(restored[kLfoWaveChoice0]==p[kLfoWaveChoice0]);
 }
 qg::Lfo random;random.prepare(15);qg::LfoSettings s;s.enabled=true;s.waveRandom=1;s.randomWaveCount=328;bool extra=false;
 for(int n=0;n<100;++n){random.process(s,n*4.,48000,false,0);assert(random.wave()!=128&&random.wave()!=129);extra|=random.wave()>=130;}assert(extra);
 // Reproduce enabling MOD RANDOM directly from Default or a saved legacy wave,
 // without first touching the new wave selector. All new IDs must be reachable.
 for(double selected:{0.,1./qg::waveChoiceCount,1.})for(int rate=1;rate<=6;++rate){
  auto patch=initialParameters();patch[kLfoWaveChoice0]=selected;patch[kModWaveRate0]=(rate+1.)/7.;
  const double intervals[]={4.,2.,1.,.5,8.,16.};
  for(int mod=0;mod<4;++mod){patch[lfoID(mod,lEnabled)]=1.;patch[kModWaveRate0+mod]=(rate+1.)/7.;patch[kLfoWaveChoice0+mod]=selected;
   auto config=settings(patch,120,48000).lfos[mod];qg::Lfo l;l.prepare(17+mod);std::set<int> reached;
   for(int step=0;step<12000;++step){double value=l.process(config,step*intervals[rate-1]+.001,48000,false,0);assert(std::isfinite(value)&&value>=-1&&value<=1);reached.insert(l.wave());}
   assert(reached.size()==qg::waveChoiceCount);
   for(int wave=130;wave<=qg::maxWave;++wave)assert(reached.count(wave));
  }
 }
 // Inclusive bounds, reversed endpoints, single-wave and stochastic-only ranges.
 for(auto bounds:{std::pair<int,int>{123,287},{287,123},{330,330},{1,1},{129,130},{200,330}}){
  auto patch=initialParameters();patch[kModWaveFrom0]=(bounds.first-1.)/qg::maxWave;patch[kModWaveTo0]=(bounds.second-1.)/qg::maxWave;
  patch[kModWaveRate0]=2./7.;patch[lfoID(0,lEnabled)]=1.;auto config=settings(patch,120,48000).lfos[0];
  qg::Lfo l;l.prepare(41);std::set<int> seen;int low=std::min(bounds.first,bounds.second),high=std::max(bounds.first,bounds.second);
  for(int step=0;step<12000;++step){double v=l.process(config,step*4.+.001,48000,false,0);assert(std::isfinite(v)&&v>=-1&&v<=1);int number=l.wave()+1;assert(number>=low&&number<=high);seen.insert(number);}
  assert(seen.size()==size_t(high-low+1));
  std::array<double,kCount> restored;assert(decodePreset(encodePreset([&](int id){return patch[id];}),restored));assert(restored[kModWaveFrom0]==patch[kModWaveFrom0]&&restored[kModWaveTo0]==patch[kModWaveTo0]);
  config.waveRandom=0;config.wave=7;l.process(config,0,48000,false,0);assert(l.wave()==7);
 }
 auto factory=bankPreset(1,8);auto bytes=encodePreset([&](int id){return factory[id];});assert(untouchedFactoryPreset(bankPresetName(1,8),bytes));assert(!untouchedFactoryPreset("My own sound",bytes));factory[kSize]=.123456789;bytes=encodePreset([&](int id){return factory[id];});assert(!untouchedFactoryPreset(bankPresetName(1,8),bytes));
 assert(adjacentPreset(-1,1,0)==-1);assert(adjacentPreset(-1,1,2)==0);assert(adjacentPreset(1,1,2)==-1);assert(adjacentPreset(-1,-1,2)==1);
 std::cout<<"PASS: 200 distinct added waves, bounded DSP, legacy selection compatibility, new preset round trips, random pool and preservation of edited presets\n";
}
