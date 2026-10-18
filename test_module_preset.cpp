#include "src/module_preset.h"
#include "src/mockup_ui.h"
#include <cassert>
#include <iostream>
int main(){using namespace aztec;for(int stage:{0,2,3,4}){
 auto source=initialParameters();for(int id=0;id<kCount;++id)source[id]=(id%97)/96.;auto text=encodeModulePreset(stage,[&](int id){return source[id];});
 auto target=initialParameters(),before=target;assert(decodeModulePreset(text,stage,target));
 for(int id=0;id<kCount;++id){assert(target[id]==(modulePresetParameter(stage,id)?source[id]:before[id]));for(int other:{0,2,3,4})if(other!=stage)assert(!(modulePresetParameter(stage,id)&&modulePresetParameter(other,id)));}
 auto saved=target;auto defaults=initialParameters();resetModulePreset(stage,[&](ParamID id){return defaults[id];},[&](ParamID id,double v){target[id]=v;});
 for(int id=0;id<kCount;++id)assert(target[id]==(modulePresetParameter(stage,id)?defaults[id]:saved[id]));
 auto unchanged=target;assert(!decodeModulePreset(text,stage==0?2:0,target));assert(target==unchanged);assert(!decodeModulePreset(text+"garbage",stage,target));assert(!decodeModulePreset(text.substr(0,text.size()/2),stage,target));assert(target==unchanged);
 auto invalid=encodeModulePreset(stage,[](int){return 1.5;});assert(!decodeModulePreset(invalid,stage,target));assert(target==unchanged);
 // Every visible module control, in every algorithm, is included; global controls are excluded.
 for(int mode=0;mode<3;++mode){auto values=initialParameters();values[kResliceAlgorithm]=mode*.5;auto get=[&](ParamID id){return values[id];};mockup::RackState rack;auto rect=mockup::moduleRect(stage,get,rack);for(auto c:mockup::controls(get,0,0,0,0,0,rack))if(c.r.y>=rect.y&&c.r.y+c.r.h<=rect.y+rect.h)assert(modulePresetParameter(stage,c.id));}
 assert(!modulePresetParameter(stage,kRoutingOrder)&&!modulePresetParameter(stage,kMix));
 }std::cout<<"PASS module preset roundtrip, isolation, control coverage and invalid files\n";
}
