#include "src/factory_presets.h"
#include "src/preset_io.h"
#include "src/parameter_settings.h"
#include <cassert>
#include <set>
#include <iostream>
#include <memory>
int main(){using namespace aztec;
 std::set<std::string> names,states;std::set<int> routes;std::array<int,8> categories{};
 for(int n=0;n<factoryPresetCount;++n){auto p=factoryPreset(n);assert(names.insert(factoryNames[n]).second);assert(presetCategory(std::string(factoryNames[n]))==n/16);++categories[n/16];
  for(auto v:p)assert(std::isfinite(v)&&v>=0&&v<=1);assert(p[kMix]==1&&p[kRoutingOrder]>0);routes.insert(int(std::round(p[kRoutingOrder]*120))-1);
  auto bytes=encodePreset([&](int id){return p[id];});assert(states.insert(bytes).second);std::array<double,kCount> loaded{};assert(decodePreset(bytes,loaded));for(int id=0;id<kCount;++id)if(presetParameter(id))assert(loaded[id]==p[id]);
  auto s=settings(p,120,8000);auto e=std::make_unique<qg::Engine>();e->set(s);e->prepare(8000);e->set(s);double energy=0,peak=0;
  for(int i=0;i<80000;++i){if(i%64==0)e->set(s);float x=float(.13*std::sin(i*.137)+.07*std::sin(i*.021));float l,r;e->process(x,-x*.73f,i/4000.,l,r);assert(std::isfinite(l)&&std::isfinite(r));energy+=l*l+r*r;peak=std::max({peak,std::abs(double(l)),std::abs(double(r))});}
  assert(energy>1e-4&&peak<8.);
 }
 assert(names.size()==128&&states.size()==128&&routes.size()==120);for(int count:categories)assert(count==16);
 for(int i=0;i<legacyFactoryPresetCount;++i){auto p=legacyFactoryPreset(i);assert(presetCategory(std::string(legacyFactoryNames[i]))==8);assert(std::isfinite(p[kGrainMix]));}
 assert(presetCategory(std::string("My own patch"))==9);
 assert(presetFileLess(L"V2 001 Velvet Cloud.gdspreset",L"01 Clean Grains.gdspreset"));assert(presetFileLess(L"01 Clean Grains.gdspreset",L"Custom.gdspreset"));
 auto p=initialParameters();for(int i=0;i<6;++i)p[kMixLock0+i]=1;std::array<double,kCount> loaded{};assert(decodePreset(encodePreset([&](int id){return p[id];}),loaded));for(int i=0;i<6;++i)assert(loaded[kMixLock0+i]==1);
 std::cout<<"PASS: 128 distinct presets, 8 categories, all 120 routings, preset/lock roundtrip and 10 seconds of finite audible DSP per preset\n";
}
