#pragma once
#include "factory_presets.h"
#include "preset_io.h"
#include <cstdio>
namespace aztec {
constexpr int presetBankCount=3,presetsPerBank=128;
constexpr const char* presetBankNames[]={"Legacy","Circuit Fractures","Forest Escape"};
constexpr const char* bankFamilies[2][8]={
 {"Micro Fractures","Clock Mutations","Digital Debris","Reverse Machines","Percussive Gates","Resonant Circuits","Spatial Shards","Hybrid Experiments"},
 {"Moss Clouds","Harmonic Canopy","Root Pulses","Slow Spirals","Night Insects","Woodland Resonance","Deep Clearings","Forest Journeys"}};
inline const char* bankCategoryName(int bank,int category){return bank==0?factoryCategories[category]:bankFamilies[bank-1][category];}
inline std::string bankPresetName(int bank,int index){
 if(bank==0)return factoryNames[index];
 const char* fragments[]={"Lattice","Relay","Splinter","Current","Scatter","Fold","Tessellate","Impulse","Trace","Shard","Glitch","Phase","Displace","Signal","Feedback","Collapse"};
 const char* forest[]={"Dawn","Moss","Canopy","Roots","Mist","Fern","Creek","Spore","Grove","Moon","Bark","Firefly","Rain","Hollow","Bloom","Sanctuary"};
 char number[16];std::snprintf(number,sizeof(number),"%s %03d ",bank==1?"CF":"FE",index+1);
 const char* shortFamily[2][8]={{"Micro","Clock","Debris","Reverse","Pulse","Resonant","Spatial","Hybrid"},{"Moss","Canopy","Root","Spiral","Night","Woodland","Clearing","Journey"}};
 return std::string(number)+shortFamily[bank-1][index/16]+" "+(bank==1?fragments[index%16]:forest[index%16]);
}
// Previous release retained solely to recognize untouched installed factory files.
inline std::array<double,kCount> bankPresetBeforeMixRevision(int bank,int index){
 if(bank==0)return factoryPreset(index);const int family=index/16,v=index%16;
 auto p=initialParametersBeforeRemoval();p[kMix]=1;p[kMasterLimiter]=1;p[kLimiterCeiling]=.85;p[kGrainBuffer]=bank==1?.5:1.;
 p[kRoutingOrder]=(1+(index*37+(bank==1?17:53))%120)/120.;p[kGrainEnabled]=1;p[kDensityFlow]=1;p[kGrainPan]=.5;p[kPanMode]=v%3==0?.5:0.;
 auto mask=[&](int id,unsigned bits){for(int i=0;i<16;++i)p[id+i]=(bits>>i)&1;};
 auto route=[&](int l,int target,double amount,int wave,int grid){p[lfoID(l,lEnabled)]=1;p[lfoID(l,lSync)]=1;p[lfoID(l,lWave)]=wave/129.;p[lfoID(l,lGrid)]=grid/20.;p[lfoID(l,lDepth)]=1;p[lfoID(l,lGlide)]=bank==1?.08:.85;p[slotTarget(l,0)]=(target+1.)/qg::modTargetCount;p[slotAmount(l,0)]=.5+amount*.5;};
 static constexpr unsigned masks[]={0x9249,0x5555,0x9653,0x1111,0x4a95,0x7878,0xa5a5,0x6996,0x3333,0x8421,0xdada,0x6666,0x1717,0x0f0f,0x5a69,0xf0f0};
 if(bank==1){
  p[kSize]=(.017+.006*(v%8)+.003*family-.015)/.235;p[kDensity]=(5+(v*3+family)%24-1)/31.;p[kPosition]=(.03+.037*v+.021*family-.015)/1.985;
  p[kChaos]=.18+.04*(v%8);p[kGrainMix]=.45+.05*(family%6);p[kPitch]=(48.+(v%4==0?-12:v%4==1?7:v%4==2?12:0))/96.;
  p[kGlitchEnabled]=family!=4;p[kGlitchMix]=.35+.035*(v%9);p[kGlitchDivision]=(4+(v+family)%8)/15.;p[kGlitchChance]=.55+.025*v;p[kGlitchReverse]=(v+family)%3==0;p[kGlitchVariation]=.25+.03*(v%8);p[kGlitchTriggerRate]=(v%4)/3.;
  p[kRepeatEnabled]=family==1||family==2||family==7;p[kRepeatSeq]=1;p[kRepeatMix]=.65;p[kRepeatDivision]=(4+v%8)/15.;mask(kRepeatStep0,masks[v]);
  p[kResliceEnabled]=family==0||family==2||family==3||family==7;p[kResliceMix]=.55+.025*(v%12);p[kResliceLength]=(1+v%3)/3.;mask(kResliceStep0,masks[(v+3)%16]);p[kResliceRndOn]=v%4==3;p[kResliceRndRate]=(v%8)/7.;
  p[kGaterEnabled]=family==4||family==7;p[kGaterGrid]=(1+v%3)/3.;mask(kGaterState0,masks[(v+7)%16]);
  for(int i=0;i<16;++i){p[kRepeatRate0+i]=(4+(i*3+v)%8)/15.;p[kRepeatPitch0+i]=(48.+(i%4==3?12:i%5==0?-12:0))/96.;p[kResliceIndex0+i]=((i*5+v+family)%16)/15.;p[kGaterLength0+i]=.15+.045*((i+v)%12);p[kGaterSustain0+i]=.02;}
  p[kMasterFilter]=family==5||family==7;p[kFilterModel]=(family==5?7+v%2:v%5)/18.;p[kFilterCutoff]=.35+.025*v;p[kFilterResonance]=.2+.025*(v%10);p[kFilterDrive]=.05+.025*(v%6);
  p[kReverbOn]=family==6||family==7;p[kReverbModel]=(v%5)/4.;p[kReverbMix]=.12+.02*(v%8);p[kReverbLength]=(1.2+.35*v-.2)/19.8;p[kReverbSource]=family==6;p[kReverbRandomRate]=(v%4)/3.;
  route(0,family%2?3:0,.12+.015*(v%8),96+v,3+v%8);p[kModWaveRate0]=(2+v%4)/7.;route(1,4,.2,112+v,5+family%4);
 }else{
  const int harmony[]={0,7,12,-12,0,5,7,12,0,-5,-12,7,12,0,5,0};
  p[kSize]=(.14+.006*v-.015)/.235;p[kDensity]=(3+v%6-1)/31.;p[kPosition]=(.4+.06*v-.015)/1.985;p[kChaos]=.015+.006*(v%6);p[kPitch]=(48.+harmony[v])/96.;p[kGrainMix]=.65+.02*(v%12);
  p[kGlitchEnabled]=family==4;p[kGlitchMix]=.2;p[kGlitchChance]=.35;p[kGlitchDivision]=(v%3)/15.;p[kGlitchVariation]=.08;p[kGlitchReverse]=v%4==0;
  p[kRepeatEnabled]=family==1||family==2;p[kRepeatMix]=.2+.015*(v%8);p[kRepeatSeq]=1;p[kRepeatDivision]=(v%3)/15.;mask(kRepeatStep0,0x1111);
  p[kResliceEnabled]=family==3||family==7;p[kResliceLength]=0;p[kResliceMix]=.18+.012*v;mask(kResliceStep0,0xffff);
  p[kGaterEnabled]=family==2||family==4;p[kGaterGrid]=0;mask(kGaterState0,0xffff);
  for(int i=0;i<16;++i){p[kRepeatRate0+i]=(v%3)/15.;p[kRepeatPitch0+i]=(48.+(i%4==3?7:0))/96.;p[kResliceIndex0+i]=((i+4*(v%4))%16)/15.;p[kGaterLength0+i]=.75+.015*(i%8);p[kGaterSustain0+i]=.4;}
  p[kMasterFilter]=1;p[kFilterModel]=(family==5?7:v%2)/18.;p[kFilterType]=0;p[kFilterCutoff]=.35+.018*v;p[kFilterResonance]=.12+.01*(v%8);p[kFilterDrive]=.02;
  p[kReverbOn]=1;p[kReverbModel]=(family==6?4:1+v%3)/4.;p[kReverbMix]=.24+.015*(v%12);p[kReverbLength]=(4.+.55*v-.2)/19.8;p[kReverbSource]=0;mask(kReverbStep0,0xffff);p[kReverbRateV2]=1./6.;
  route(0,8,.08+.007*v,v%16,18+v%3);route(1,3,.06,v%2?16:0,18+(v+1)%3);p[kModWaveRate0]=(v%2?6:7)/7.;
  if(family==7)route(2,19,.08,0,20);
 }
 for(auto& value:p)value=std::clamp(value,0.,1.);return p;
}
constexpr ParamID bankMixEnabled[]={kGrainEnabled,kGlitchEnabled,kRepeatEnabled,kResliceEnabled,kReverbOn};
constexpr ParamID bankMixParameters[]={kGrainMix,kGlitchMix,kRepeatMix,kResliceMix,kReverbMix};
inline std::array<double,kCount> bankPresetFullMixRevision(int bank,int index){
 auto p=bankPresetBeforeMixRevision(bank,index);
 for(int i=0;i<5;++i)p[bankMixParameters[i]]=p[bankMixEnabled[i]]>=.5?1.:0.;
 return p;
}
inline std::array<double,kCount> bankPresetFourModuleRevision(int bank,int index){
 auto p=bankPresetFullMixRevision(bank,index);bool retired=p[kGlitchEnabled]>.5;
 p[kGlitchEnabled]=p[kGlitchMix]=0.;p[kGrainPan]=.5;p[kPanMode]=0.;
 for(int i=0;i<16;++i)p[kResliceStep0+i]=1.;
 // Replace Preslicer-focused patches with equivalent rhythmic Reslice/repeater work.
 if(retired){p[kResliceEnabled]=p[kResliceMix]=1.;p[kResliceLength]=bank==2?0.:2./3.;for(int i=0;i<16;++i)p[kResliceIndex0+i]=((i*(bank==2?1:5)+index)%16)/15.;p[kResliceRndOn]=bank==1?1.:0.;}
 std::array<int,4> order{{0,2,3,4}};for(int n=0;n<index%24;++n)std::next_permutation(order.begin(),order.end());p[kRoutingOrder]=fourModuleRouting(order);
 for(int l=0;l<4;++l)for(int slot=0;slot<6;++slot){int target=int(std::round(p[slotTarget(l,slot)]*qg::modTargetCount))-1;if((target>=12&&target<=15)||target==27||target==29){p[slotTarget(l,slot)]=1./qg::modTargetCount;}}
 for(auto id:{kXTarget,kYTarget})if(retiredPreslicerParam(int(std::round(p[id]*kXYX))-1))p[id]=double(kSize+1)/kXYX;
 return p;
}
inline std::array<double,kCount> bankPreset(int bank,int index){
 auto p=bankPresetFourModuleRevision(bank,index);int v=index%16,family=index/16;
 p[kResliceAlgorithm]=((index+family)%3)/2.;p[kReslicePhrase]=(bank==2?2+v%2:v%4)/3.;
 p[kResliceRepeat]=bank==2?.15+.015*v:.3+.035*v;p[kResliceVariation]=bank==2?.12+.015*v:.35+.035*v;
 p[kResliceFill]=bank==2?.08+.01*v:.3+.04*v;p[kResliceReverse]=bank==2?.015*(v%5):.04*(v%10);
 p[kResliceSeed]=(1+index+bank*128)/65534.;
 resliceDefaults(p);p[kResliceMinPhrase]=(std::pow(2.,std::round(p[kReslicePhrase]*3))-1)/7.;
 p[kResliceRndOn]=0.; // Retired sequencer mode; the cutters evolve continuously.
 return p;
}
// Only upgrade untouched factory files. User edits and session states are preserved.
inline bool needsBankMixUpgrade(const std::string& text,int bank,int index){
 std::array<double,kCount> saved{};if(!decodePreset(text,saved))return false;
 std::istringstream header(text);std::string magic;int version=0,count=0;header>>magic>>version>>count;
 if(count>int(kResliceAlgorithm))return false; // Never rewrite this generation's user saves.
 const auto previous=bankPresetBeforeMixRevision(bank,index),fullMix=bankPresetFullMixRevision(bank,index),four=bankPresetFourModuleRevision(bank,index);
 bool a=true,b=true,c=true;for(int id=0;id<int(kResliceAlgorithm);++id)if(presetParameter(id)){a&=saved[id]==previous[id];b&=saved[id]==fullMix[id];c&=saved[id]==four[id];}
 return a||b||c;
}
}
