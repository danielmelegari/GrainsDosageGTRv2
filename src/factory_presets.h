#pragma once
#include "parameters.h"
#include <cmath>
#include <string>
#include "user_presets.h"
namespace aztec {
inline std::array<double,kCount> initialParametersBeforeRemoval() {
  std::array<double,kCount> p{};for(int i=kUiWaveLow0;i<int(kReverbRateV2);++i)p[i]=.5;p[kResliceLength]=2./3.;p[kFilterSeqDepth]=.5;p[kFilterSeqGlide]=.2;p[kFilterSeqRate]=2./5.;p[kCombOctave]=2./6.;p[kResliceMix]=1.;for(int i=0;i<16;++i)p[kResliceIndex0+i]=i/15.;p[kInputDeclick]=1.;p[kDeclickSensitivity]=.5;p[kReverbRandomRate]=1.;p[kGaterGrid]=2./3.;p[kGaterChance]=.5;p[kGaterMinLength]=0.;p[kGaterTie]=0.;for(int i=0;i<16;++i){p[kGaterState0+i]=1.;p[kGaterLength0+i]=(.75-.05)/.95;p[kGaterSustain0+i]=.1;}
  // Reslice starts with all steps enabled but the module itself stays off (kResliceEnabled default 0).
  for(int i=0;i<16;++i)p[kResliceStep0+i]=1.;
  for(int l=0;l<4;++l){p[kUiLfoCycle0+l]=p[kUiLfoEpoch0+l]=2147483648./4294967295.;p[kLfoSpeed0+l]=2./3.;for(int slot=0;slot<6;++slot)p[slotAmount(l,slot)]=.5;}
  p[kReverbLength]=(2.8-.2)/19.8;
  p[kGrainEnabled]=p[kGlitchEnabled]=p[kRepeatEnabled]=1.;p[kGrainPan]=.5;p[kReverbGrid]=2./3.;p[kReverbMix]=.25;
  for(int i=0;i<16;i+=4)p[kReverbStep0+i]=1.;
  p[kMasterLimiter]=1.;p[kFilterCutoff]=std::log(50.)/std::log(1000.);
  p[kGlitchMove]=.5;p[kGlitchVariation]=.3;p[kGlitchRefresh]=2./7.;p[kRepeatInterval]=4./7.;p[kRepeatDuration]=3./31.;p[kRepeatChance]=1.;
  p[kDensityFlow]=1.; p[kTranspose]=p[kXYX]=p[kXYY]=.5; p[kXAmount]=p[kYAmount]=.75;
  p[kGrainBuffer]=1.; // BUFFER SIZE default: 16 s (0=1 s, .25=2 s, .5=4 s, .75=8 s, 1=16 s)
  p[kXTarget]=double(kSize+1)/kXYX; p[kYTarget]=double(kPitch+1)/kXYX;
  p[kDensity]=3./31.; p[kDivision]=.25; p[kSize]=.25; p[kPosition]=.05; p[kPitch]=.5;
  p[kMix]=1.; p[kAttack]=.1; p[kRelease]=.15; p[kGrainMix]=1.;
  p[kGlitchDivision]=5./15.; p[kGlitchChance]=1.; p[kRepeatMix]=1.; p[kRepeatDivision]=4./15.;
  p[kGlitchSeq]=1.; p[kRepeatSeq]=0.; p[kStretchSpeed]=.2;
  for(int i=0;i<16;++i) { p[kGlitchStep0+i]=p[kRepeatStep0+i]=1.; p[kRepeatPitch0+i]=.5; }
  for(int i=0;i<16;++i) p[kStep0+i] = 1.;
  for(int i=0;i<qg::lfoCount;++i) {
    const int base=lfoID(i,0); p[base+lSync]=1.; p[base+lHz]=.99/39.99;
    p[base+lGrid]=3./20.; p[kRandomSteps0+i]=15./63.; p[base+lDepth]=1.; p[base+lGlide]=1.;
    for(int t=0;t<8;++t) p[routeID(i,t)]=.5;
  }
  return p;
}
inline std::array<double,kCount> initialParameters(){auto p=initialParametersBeforeRemoval();p[kGlitchEnabled]=p[kGlitchMix]=0.;p[kReslicePhrase]=2./3.;p[kResliceRepeat]=.45;p[kResliceVariation]=.5;p[kResliceFill]=.5;p[kResliceReverse]=.1;return p;}
}
namespace aztec {
constexpr const char* legacyFactoryNames[]={"01 Clean Grains","02 Forest Drift","03 Psy Scatter","04 Octave Dust","05 Reverse Glitch","06 Micro Stutter","07 Triplet Chops","08 Half Time Mist","09 Space Bloom","10 Acid Motion","11 Voice FX","12 SbimSbam","13 OctaveDust3","14 OctaveDust2","15 Up and Down","16 Daniel Init","17 RND Verb Slow Mel","18 RND Verb RPT","19 RND Verb","20 Loop Slicer","21 Third Cuts","22 Latch Traffic","23 Northern Sweep","24 Ladder Bass","25 Acid Cuts","26 Comb Orbit","27 Talking Grains","28 Plate Steps","29 Cosmic Impulse","30 Bloom Voyage"};
constexpr int legacyFactoryPresetCount=30;
inline std::array<double,kCount> legacyFactoryPreset(int index){
 auto p=initialParametersBeforeRemoval();p[kNormalize]=1.;p[kGrainMix]=.75;p[kMix]=1.;
 auto pattern=[&](int base,unsigned bits){for(int i=0;i<16;++i)p[base+i]=(bits&(1u<<i))?1.:0.;};
 auto lfo=[&](int wave,int target,double amount,double rate){p[lfoID(0,lEnabled)]=1.;p[lfoID(0,lWave)]=wave/129.;p[slotTarget(0,0)]=(target+1)/double(qg::modTargetCount);p[slotAmount(0,0)]=.5+amount*.5;p[lfoID(0,lGrid)]=rate/20.;};
 if(index>=10&&index<=18){importedPreset(index-10,p);migrateModSlots(p);return p;}
 switch(index){
 case 0:p[kNormalize]=0.;p[kGrainMix]=1.;p[kSize]=.25;p[kDensity]=7./31.;break;
 case 1:p[kSize]=.6;p[kPosition]=.2;p[kChaos]=.3;p[kPanMode]=.5;lfo(0,2,.16,3);p[kReverbOn]=1.;p[kReverbMix]=.18;break;
 case 2:p[kSize]=.12;p[kDensity]=15./31.;p[kChaos]=.6;p[kPanMode]=1.;lfo(128,2,.32,6);break;
 case 3:p[kPitch]=.625;p[kDensity]=9./31.;p[kGrainPan]=.65;p[kReverbOn]=1.;p[kReverbType]=.75;p[kReverbMix]=.25;p[kReverbLength]=(5.-.2)/19.8;break;
 case 4:p[kModuleOrder]=2./6.;p[kGlitchMix]=.85;p[kGlitchReverse]=1.;p[kGlitchVariation]=.7;p[kGlitchDivision]=4./15.;pattern(kGlitchStep0,0x3333);break;
 case 5:p[kRepeatAuto]=1.;p[kRepeatMix]=.9;p[kRepeatDivision]=6./15.;p[kRepeatInterval]=2./7.;p[kRepeatDuration]=2./31.;p[kGrainMix]=.35;break;
 case 6:p[kRepeatSeq]=1.;p[kRepeatDivision]=10./15.;p[kRepeatMix]=.8;pattern(kRepeatStep0,0x4924);p[kGlitchMix]=.3;pattern(kGlitchStep0,0x8080);break;
 case 7:p[kStretchOn]=1.;p[kStretchSpeed]=(.5-.25)/3.75;p[kSize]=.65;p[kPanMode]=.5;p[kReverbOn]=1.;p[kReverbMix]=.3;p[kReverbLength]=(4.-.2)/19.8;break;
 case 8:p[kSize]=.8;p[kDensity]=7./31.;p[kReverbOn]=1.;p[kReverbType]=1.;p[kReverbMix]=.65;p[kReverbLength]=(12.-.2)/19.8;pattern(kReverbStep0,0x0101);p[kReverbGrid]=1./3.;break;
 case 9:p[kMasterFilter]=1.;p[kFilterType]=.5;p[kFilterCutoff]=std::log(80.)/std::log(1000.);p[kFilterResonance]=.4;p[kFilterDrive]=.25;p[kChaos]=.4;p[kGlitchMix]=.4;pattern(kGlitchStep0,0x5555);lfo(16,8,.6,5);break;
 case 19:p[kResliceEnabled]=1.;pattern(kResliceStep0,0xffff);for(int i=0;i<16;++i)p[kResliceIndex0+i]=(i/4*4)/15.;p[kGrainMix]=.3;break;
 case 20:p[kResliceEnabled]=1.;p[kResliceLength]=2./3.;pattern(kResliceStep0,0x7777);for(int i=0;i<16;++i)p[kResliceIndex0+i]=(15-i)/15.;p[kGlitchMix]=.25;break;
 case 21:p[kGaterEnabled]=p[kGaterLatch]=1.;for(int i=0;i<16;++i)p[kGaterState0+i]=0.;p[kGaterState0]=1.;p[kGaterState0+4]=1.;p[kGaterRelease0+8]=1.;p[kGaterState0+12]=1.;p[kGrainMix]=.9;break;
 case 22:p[kMasterFilter]=1.;p[kFilterModel]=1./18.;p[kFilterResonance]=.55;p[kFilterDrive]=.18;lfo(0,8,.6,3);break;
 case 23:p[kMasterFilter]=1.;p[kFilterModel]=2./18.;p[kFilterSlope]=1.;p[kFilterCutoff]=.42;p[kFilterResonance]=.6;p[kFilterDrive]=.3;p[kPitch]=.375;break;
 case 24:p[kMasterFilter]=1.;p[kFilterModel]=3./18.;p[kFilterSlope]=1.;p[kFilterResonance]=.8;p[kFilterDrive]=.35;lfo(16,8,.55,6);p[kResliceEnabled]=1.;pattern(kResliceStep0,0x5555);break;
 case 25:p[kMasterFilter]=1.;p[kFilterModel]=7./18.;p[kFilterCutoff]=.5;p[kFilterResonance]=.6;lfo(0,8,.15,2);p[kReverbOn]=1.;p[kReverbModel]=.75;p[kReverbMix]=.25;break;
 case 26:p[kMasterFilter]=1.;p[kFilterModel]=9./18.;p[kFilterResonance]=.5;lfo(0,8,.16,5);p[kReverbOn]=1.;p[kReverbMix]=.15;break;
 case 27:p[kReverbOn]=1.;p[kReverbModel]=.25;p[kReverbLength]=(3.-.2)/19.8;p[kReverbMix]=.55;pattern(kReverbStep0,0x1111);break;
 case 28:p[kReverbOn]=1.;p[kReverbModel]=.5;p[kReverbLength]=(14.-.2)/19.8;p[kReverbMix]=.6;p[kReverbSource]=1.;p[kReverbRandomRate]=0.;p[kSize]=.8;break;
 case 29:p[kReverbOn]=1.;p[kReverbModel]=1.;p[kReverbLength]=(18.-.2)/19.8;p[kReverbMix]=.65;p[kReverbSource]=1.;p[kReverbRandomRate]=.5;p[kMasterFilter]=1.;p[kFilterModel]=15./18.;p[kFilterResonance]=.35;p[kResliceEnabled]=1.;pattern(kResliceStep0,0x0f0f);lfo(0,8,.3,1);break;
 }
 return p;
}
// Routing bank: eight musical families, sixteen authored variations per family.
constexpr int factoryCategoryCount=8, factoryPresetCount=128;
constexpr const char* factoryCategories[]={"Granular Textures","Pitch & Harmony","Beat Repeat","Slice & Shuffle","Gates & Pulses","Filter Motion","Ambient & Space","Hybrid Routing","Legacy Presets","User Presets"};
constexpr const char* factoryNames[]={
 "V2 001 Velvet Cloud",
 "V2 002 Glass Dust",
 "V2 003 Paper Grains",
 "V2 004 Soft Focus",
 "V2 005 Amber Scatter",
 "V2 006 Silver Thread",
 "V2 007 Frozen Rain",
 "V2 008 Wide Pollen",
 "V2 009 Sand Drift",
 "V2 010 Cloud Loom",
 "V2 011 Tiny Constellations",
 "V2 012 Broken Silk",
 "V2 013 Warm Mist",
 "V2 014 Crystal Swarm",
 "V2 015 Grain Tides",
 "V2 016 Violet Haze",
 "V2 017 Octave Lantern",
 "V2 018 Fifth Horizon",
 "V2 019 Minor Orbit",
 "V2 020 Major Prism",
 "V2 021 Low Gravity",
 "V2 022 Double Halo",
 "V2 023 Falling Fifth",
 "V2 024 Third Bloom",
 "V2 025 Octave Ladder",
 "V2 026 Fifth Cascade",
 "V2 027 Minor Steps",
 "V2 028 Major Steps",
 "V2 029 Deep Choir",
 "V2 030 High Bells",
 "V2 031 Contrary Motion",
 "V2 032 Pitch Mosaic",
 "V2 033 Tight Sixteenths",
 "V2 034 Half Beat Echo",
 "V2 035 Triplet Relay",
 "V2 036 Dotted Bounce",
 "V2 037 Micro Ratchet",
 "V2 038 Pocket Stutter",
 "V2 039 Offbeat Catch",
 "V2 040 Quarter Hold",
 "V2 041 Pitch Tap",
 "V2 042 Triplet Crumble",
 "V2 043 Double Stroke",
 "V2 044 Backbeat Loop",
 "V2 045 Slow Turnaround",
 "V2 046 Dotted Ladder",
 "V2 047 Broken Roll",
 "V2 048 Repeat Carousel",
 "V2 049 Slice Domino",
 "V2 050 Reverse Tiles",
 "V2 051 Quarter Mosaic",
 "V2 052 Shuffle Deck",
 "V2 053 Window Fold",
 "V2 054 Staircase Cut",
 "V2 055 Mirror Slices",
 "V2 056 Dice Garden",
 "V2 057 Half Bar Jigsaw",
 "V2 058 Thirds Shuffle",
 "V2 059 Backwards Map",
 "V2 060 Slice Ladder",
 "V2 061 Window Weave",
 "V2 062 Random Corners",
 "V2 063 Crosscut Motion",
 "V2 064 Slice Origami",
 "V2 065 Pulse Grid",
 "V2 066 Short Circuit",
 "V2 067 Swing Windows",
 "V2 068 Long Breath",
 "V2 069 Lattice Gate",
 "V2 070 Needle Rhythm",
 "V2 071 Open Spaces",
 "V2 072 Latch Islands",
 "V2 073 Broken Pulse",
 "V2 074 Soft Chopper",
 "V2 075 Triplet Shadow",
 "V2 076 Staccato Rain",
 "V2 077 Gate Before Grain",
 "V2 078 Gate After Slice",
 "V2 079 Held Release",
 "V2 080 Pulse Tunnel",
 "V2 081 Acid Footsteps",
 "V2 082 Ladder Bloom",
 "V2 083 Comb Ladder",
 "V2 084 Vowel Drift",
 "V2 085 Northern Lights",
 "V2 086 Resonant Steps",
 "V2 087 Bandpass Relay",
 "V2 088 Soft Formants",
 "V2 089 Lowpass Tide",
 "V2 090 Highpass Sparks",
 "V2 091 Comb Orbit Two",
 "V2 092 Filter Dice",
 "V2 093 Driven Stairway",
 "V2 094 Talking Rhythm",
 "V2 095 Arp Fragments",
 "V2 096 Spectral Loom",
 "V2 097 Plate Mist",
 "V2 098 Dotted Chapel",
 "V2 099 Cosmic Dust",
 "V2 100 Shimmer Room",
 "V2 101 Slow Cathedral",
 "V2 102 Impulse Stars",
 "V2 103 Cloud Chamber",
 "V2 104 Bloom Tail",
 "V2 105 Distant Grain",
 "V2 106 Random Sanctuary",
 "V2 107 Velvet Plate",
 "V2 108 Long Horizon",
 "V2 109 Triplet Space",
 "V2 110 Halo Drops",
 "V2 111 Deep Bloom",
 "V2 112 Afterglow",
 "V2 113 Five Stage Orbit",
 "V2 114 Grain Into Gate",
 "V2 115 Gate Into Grain",
 "V2 116 Repeat Into Slice",
 "V2 117 Slice Into Repeat",
 "V2 118 Chopped Constellation",
 "V2 119 Reverse Cascade",
 "V2 120 Pitch Labyrinth",
 "V2 121 Rhythm Prism",
 "V2 122 Cloud Machine",
 "V2 123 Time Origami",
 "V2 124 Spectral Carousel",
 "V2 125 Dotted Maze",
 "V2 126 Broken Gravity",
 "V2 127 Morphing Circuit",
 "V2 128 Violet Universe",
};
template<class Char> inline int presetBank(const std::basic_string<Char>& name){if(name.size()>6&&name[2]==Char(' ')){if(name[0]==Char('C')&&name[1]==Char('F'))return 1;if(name[0]==Char('F')&&name[1]==Char('E'))return 2;}return 0;}
template<class Char> inline int presetCategory(const std::basic_string<Char>& name){
 if(presetBank(name)>0){int n=0;for(int i=3;i<6;++i){if(name[i]<Char('0')||name[i]>Char('9'))return factoryCategoryCount+1;n=n*10+int(name[i]-Char('0'));}if(n>=1&&n<=128)return (n-1)/16;}
 auto equal=[&](const char* text){size_t n=std::char_traits<char>::length(text);if(name.size()!=n)return false;for(size_t i=0;i<n;++i)if(name[i]!=Char(text[i]))return false;return true;};
 for(int i=0;i<factoryPresetCount;++i)if(equal(factoryNames[i]))return i/16;
 for(int i=0;i<legacyFactoryPresetCount;++i)if(equal(legacyFactoryNames[i]))return factoryCategoryCount;
 return factoryCategoryCount+1;
}
inline std::array<double,kCount> factoryPreset(int index){
 index=std::clamp(index,0,factoryPresetCount-1);const int family=index/16,v=index%16;
 auto p=initialParametersBeforeRemoval();
 // No master dry injection, no automatic freeze, no hidden legacy routes.
 p[kMix]=1.;p[kNormalize]=0.;p[kMasterLimiter]=1.;p[kLimiterCeiling]=1.;
 p[kGrainEnabled]=1.;p[kGlitchEnabled]=p[kRepeatEnabled]=0.;p[kRepeatMix]=0.;
 p[kResliceEnabled]=p[kGaterEnabled]=0.;p[kGrainMix]=1.;p[kGrainBuffer]=.5;
 p[kRoutingOrder]=(1+(index*37)%120)/120.;
 const double sizes[]={.035,.09,.022,.14,.06,.18,.045,.11,.027,.21,.075,.05,.16,.03,.125,.24};
 const int densities[]={4,6,8,3,12,4,10,6,16,3,8,12,4,16,6,8};
 const double positions[]={.08,.18,.06,.32,.12,.5,.24,.4,.1,.75,.28,.16,.6,.2,.45,.9};
 const int divisions[]={4,3,10,13,6,4,5,2,5,11,4,3,2,14,6,10};
 const unsigned masks[]={0xffff,0x5555,0x9249,0x3333,0x1111,0x7777,0x8888,0x0f0f,0x555f,0x4925,0x333f,0x2222,0x00ff,0x9696,0x4445,0x6996};
 const int pitches[]={0,12,7,-12,0,5,-7,12,0,-5,7,0,-12,24,5,0};
 p[kSize]=(sizes[v]-.015)/.235;p[kDensity]=(densities[v]-1)/31.;p[kPosition]=(positions[v]-.015)/1.985;
 p[kChaos]=.025*(v%5);p[kPanMode]=v%4==3?.5:0.;p[kGrainPan]=.5;
 auto mask=[&](int base,unsigned bits){for(int i=0;i<16;++i)p[base+i]=(bits>>i)&1;};
 auto route=[&](int l,int target,double amount,int wave,int grid,int polarity=0){
  p[lfoID(l,lEnabled)]=1.;p[lfoID(l,lWave)]=wave/129.;p[lfoID(l,lGrid)]=grid/20.;p[lfoID(l,lGlide)]=.35;
  p[slotTarget(l,0)]=(target+1)/double(qg::modTargetCount);p[slotAmount(l,0)]=.5+amount*.5;p[slotPolarity(l,0)]=polarity*.5;
 };
 auto repeat=[&](double mix){p[kRepeatEnabled]=p[kRepeatSeq]=1.;p[kRepeatMix]=mix;p[kRepeatDivision]=divisions[v]/15.;mask(kRepeatStep0,masks[v]);
  for(int i=0;i<16;++i){p[kRepeatRate0+i]=(i%4==3?divisions[(v+1)%16]:divisions[v])/15.;p[kRepeatPitch0+i]=.5;}};
 auto slice=[&](double mix){p[kResliceEnabled]=1.;p[kResliceMix]=mix;p[kResliceLength]=(v%3+1)/3.;mask(kResliceStep0,masks[(v+3)%16]);
  for(int i=0;i<16;++i){int source=v%4==0?(i/4)*4:v%4==1?15-i:v%4==2?(i*5+v)%16:(i+4)%16;p[kResliceIndex0+i]=source/15.;}};
 auto gate=[&](double length){p[kGaterEnabled]=1.;p[kGaterGrid]=(v%3+1)/3.;mask(kGaterState0,masks[v]);
  for(int i=0;i<16;++i){p[kGaterLength0+i]=(std::clamp(length+(i%4)*.035,.05,1.)-.05)/.95;p[kGaterSustain0+i]=.012/.5;}};
 auto space=[&](double amount,double seconds){p[kReverbOn]=1.;p[kReverbModel]=(v%5)/4.;p[kReverbMix]=amount;p[kReverbLength]=(seconds-.2)/19.8;
  p[kReverbRateV2]=(v%3==0?5:v%3==1?6:3)/6.;mask(kReverbStep0,masks[(v+4)%16]);p[kReverbSource]=v%4==1?1.:0.;};
 switch(family){
 case 0: // Granular texture with gentle gate/slice contrasts.
  p[kChaos]=.03+.025*(v%7);p[kGrainMix]=.8+.2*(v%3)/2.;
  if(v%2)gate(.75);else slice(.22);route(0,3,.05+.015*(v%4),0,6+v%5);if(v>=8)space(.12,2.+v*.2);break;
 case 1: // Harmonic and melodic grains; pitched repetitions are intentional.
  p[kPitch]=(pitches[v]+48.)/96.;p[kChaos]=0.;repeat(.3+.025*(v%5));
  for(int i=0;i<16;++i)p[kRepeatPitch0+i]=(48.+(i%4==3?7:0))/96.;
  if(v>=8)gate(.7);space(.15,2.5);break;
 case 2: // Clocked repeats with alternating rates and fills.
  p[kGrainMix]=.25;repeat(.85);p[kGlitchEnabled]=v%3==0;p[kGlitchMix]=.35;p[kGlitchChance]=.7;p[kGlitchDivision]=divisions[(v+2)%16]/15.;
  if(v>=8)for(int i=0;i<16;++i)p[kRepeatPitch0+i]=(48.+(i%4==3?12:0))/96.;gate(.8);break;
 case 3: // Re-slicing recorded windows, reverse fragments and controlled randomisation.
  p[kGrainMix]=.3;slice(.9);p[kGlitchEnabled]=1.;p[kGlitchMix]=.45;p[kGlitchDivision]=divisions[v]/15.;p[kGlitchReverse]=v%2;
  p[kGlitchChance]=.65+.05*(v%5);p[kGlitchTriggerRate]=(v%3)/3.;p[kGlitchVariation]=.15;
  p[kResliceRndOn]=(v==3||v==7||v==9||v==13);p[kResliceRndRate]=(v%3)/2.;break;
 case 4: // Clearly articulated gates, including two deliberate latch/release patterns.
  p[kGrainMix]=.65;gate(.18+.04*(v%8));if(v%2)slice(.4);else repeat(.35);
  if(v==7||v==14){p[kGaterLatch]=1.;mask(kGaterState0,0x1111);mask(kGaterRelease0,0x4444);}
  p[kGaterLengthRnd]=v==5||v==11;p[kGaterMinLength]=.1;break;
 case 5: // Filter sequencer drives audible melodies or cutoff movement.
  p[kGrainMix]=.7;p[kMasterFilter]=p[kFilterSeqOn]=1.;
  {const int models[]={3,2,7,9,1,4,5,10,2,6,8,12,3,9,7,15};p[kFilterModel]=models[v]/18.;}
  p[kFilterCutoff]=.42+.025*(v%7);p[kFilterResonance]=.22+.045*(v%6);p[kFilterDrive]=.08;
  p[kFilterSeqMode]=v%3?1.:0.;p[kFilterSeqPattern]=((v*7)%64)/63.;p[kFilterSeqRate]=(v%4)/5.;p[kFilterSeqDepth]=.18+.03*(v%5);p[kFilterSeqGlide]=.12;
  repeat(.3);if(v%2)gate(.7);break;
 case 6: // Longer grains and reverb tails, including both dotted rates.
  p[kSize]=.55+.025*v;p[kDensity]=(5+v%7)/31.;p[kGrainMix]=1.;p[kChaos]=.04;
  p[kPitch]=(48.+(v%4==3?12:0))/96.;space(.3+.025*(v%7),4.+v*.7);
  slice(.2);route(0,3,.04,0,10+v%5);if(v%3==0)gate(.9);break;
 case 7: // All five stages are engaged: every permutation changes the sound.
  p[kGrainMix]=.55+.025*(v%7);p[kPitch]=(pitches[v]+48.)/96.;
  p[kGlitchEnabled]=1.;p[kGlitchMix]=.35;p[kGlitchDivision]=divisions[(v+4)%16]/15.;p[kGlitchChance]=.75;p[kGlitchReverse]=v%2;p[kGlitchVariation]=.15;
  repeat(.55);slice(.6);gate(.6);space(.18,3.+v*.25);
  route(0,3,.07,0,6+v%4);route(1,8,.12,16,3+v%3,1);p[kMasterFilter]=1.;p[kFilterCutoff]=.7;p[kFilterResonance]=.2;break;
 }
 // XY starts neutral; moving the pad varies size and pitch without changing mix.
 p[kXYEnable]=0.;p[kXYX]=p[kXYY]=.5;
 return p;
}

}

