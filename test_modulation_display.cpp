#include "src/modulation_display.h"
#include "src/parameter_settings.h"
#include "src/factory_presets.h"
#include <cassert>
#include <iostream>
using namespace aztec;
int main(){
 // Compare every drawable destination/polarity against the real DSP modulator,
 // including legacy route amounts, clipping and extended Reslice controls.
 for(int active:{0,4})for(int target=0;target<qg::modTargetCount;++target)for(int polarity=0;polarity<3;++polarity){
  int id=modulationParameter(target);assert(id>=0&&id<kUiLfoValue0);
  if(retiredPreslicerParam(id))continue;
  auto p=initialParameters();p[id]=.5;
  for(int l=0;l<qg::lfoCount;++l){p[lfoID(l,lEnabled)]=l==active?1.:0.;for(int t=0;t<8;++t)p[routeID(l,t)]=.5;for(int slot=0;slot<6;++slot)p[slotTarget(l,slot)]=0.;}
  p[lfoID(active,lDepth)]=.8;p[slotTarget(active,0)]=(target+1.)/qg::modTargetCount;p[slotAmount(active,0)]=.9;p[slotPolarity(active,0)]=polarity*.5;
  auto s=settings(p,120,48000);qg::Modulation m;m.prepare();qg::ModValues base{};base.fill(.5);
  for(int n=0;n<16;++n){auto expected=m.process(s.lfos,base,n*.127,48000,false,n);p[kUiModulationActive]=1.;for(int l=0;l<qg::lfoCount;++l)p[modParam(kUiLfoValue0,l)]=(m.output(l)+1.)*.5;p[kUiRndEnvelope]=m.rndEnvelope();
   auto shown=modulationDisplayValues([&](ParamID k){return p[k];});assert(std::abs(shown[id]-expected[target])<1e-12);
  }
 }
 auto p=initialParameters();p[kSize]=.4;p[kUiModulationActive]=1.;p[lfoID(0,lEnabled)]=1.;p[kUiLfoValue0]=.8;
 p[routeID(0,0)]=.75;p[slotTarget(0,0)]=1./qg::modTargetCount;p[slotAmount(0,0)]=.75;p[slotPolarity(0,0)]=0.;
 auto shown=modulationDisplayValues([&](ParamID k){return p[k];});assert(std::abs(shown[kSize]-1.)<1e-12);
 p[kUiModulationActive]=0.;shown=modulationDisplayValues([&](ParamID k){return p[k];});assert(shown[kSize]==.4);
 p[kXYEnable]=1.;p[kXTarget]=(kSize+1.)/xyDestCount;p[kYTarget]=(kSize+1.)/xyDestCount;p[kXYX]=1.;p[kXYY]=0.;p[kXAmount]=1.;p[kYAmount]=.75;
 shown=modulationDisplayValues([&](ParamID k){return p[k];});assert(shown[kSize]==.5); // X clamps at 1, then Y subtracts .5.
 auto other=initialParameters();auto otherShown=modulationDisplayValues([&](ParamID k){return other[k];});assert(otherShown[kSize]==other[kSize]);
 qg::Modulation m;m.prepare();m.reset();for(int i=0;i<4;++i)assert(m.output(i)==0.);
 std::cout<<"PASS: live DSP/display destinations, polarity, additive routes, clamps, XY and instance isolation\n";
}
