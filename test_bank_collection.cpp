#include "src/preset_banks.h"
#include "src/preset_io.h"
#include "src/parameter_settings.h"
#include <cassert>
#include <set>
#include <memory>
#include <iostream>
int main(){using namespace aztec;std::set<std::string> names,states;
 for(int bank=0;bank<3;++bank){std::set<int> routes;for(int i=0;i<128;++i){auto p=bankPreset(bank,i);auto name=bankPresetName(bank,i);assert(names.insert(name).second);assert(presetBank(name)==bank&&presetCategory(name)==i/16);for(double v:p)assert(std::isfinite(v)&&v>=0&&v<=1);routes.insert(int(std::round(p[kRoutingOrder]*120))-1);
  auto previous=bankPresetBeforeMixRevision(bank,i);bool changed=false;
  for(int id=0;id<kCount;++id){bool mix=false;for(auto m:bankMixParameters)mix|=id==m;if(!mix)assert(p[id]==previous[id]);else changed|=p[id]!=previous[id];}
  for(int m=0;m<5;++m)assert(p[bankMixParameters[m]]==(p[bankMixEnabled[m]]>=.5?1.:0.));
  auto oldText=encodePreset([&](int id){return previous[id];});assert(needsBankMixUpgrade(oldText,bank,i)==changed);
  auto edited=previous;edited[kSize]=previous[kSize]==.123?.456:.123;assert(!needsBankMixUpgrade(encodePreset([&](int id){return edited[id];}),bank,i));
  edited=previous;edited[kGrainMix]=.371;assert(!needsBankMixUpgrade(encodePreset([&](int id){return edited[id];}),bank,i));

  auto encoded=encodePreset([&](int id){return p[id];});assert(states.insert(encoded).second);assert(!needsBankMixUpgrade(encoded,bank,i));std::array<double,kCount> decoded;assert(decodePreset(encoded,decoded));for(int id=0;id<kCount;++id)if(presetParameter(id))assert(p[id]==decoded[id]);
  if(bank==2){assert(p[kChaos]<=.05&&p[kReverbOn]==1);assert(p[lfoID(0,lGrid)]>=.9);}
  auto e=std::make_unique<qg::Engine>();auto s=settings(p,240,8000);e->set(s);e->prepare(8000);e->set(s);double energy=0,peak=0;
  for(int n=0;n<48000;++n){if(n%64==0)e->set(s);float input=.12f*std::sin(n*.037)+.07f*std::sin(n*.111),l=0,r=0;e->process(input,input*.8f,n/2000.,l,r);assert(std::isfinite(l)&&std::isfinite(r));energy+=l*l+r*r;peak=std::max({peak,std::abs(double(l)),std::abs(double(r))});}assert(energy>1e-4&&peak<8.);
 }assert(routes.size()==120);}
 assert(names.size()==384&&states.size()==384);std::cout<<"PASS: 3 banks x 128, ON/OFF Mix endpoints, other settings unchanged, safe factory upgrades, distinct patches, complete routes, roundtrips, Forest constraints and finite audible DSP\n";
}
