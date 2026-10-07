#pragma once
#include "modulation.h"
// The SDK only defines ParamID when pluginterfaces is on the include path (the
// VST3 plugin and the SDK-linked state test define GRAINS_HAVE_VST3_SDK).
// Standalone DSP tests include this header without the SDK: fall back to a
// binary-identical typedef (Steinberg::vst2paramid == uint32) so the parameter
// IDs, enum storage and all aztec code stay exactly the same either way.
#if defined(GRAINS_HAVE_VST3_SDK)
#include "pluginterfaces/vst/vsttypes.h"
#endif
#include <array>
#include <cstdint>
namespace aztec {
#if defined(GRAINS_HAVE_VST3_SDK)
using Steinberg::Vst::ParamID;
#else
using ParamID = std::uint32_t;
#endif
enum Param : ParamID { kDivision, kSize, kDensity, kPitch, kPosition, kChaos, kMix, kAttack, kRelease, kFeedback, kStep0, kGrainMix = kStep0 + 16, kGlitchMix, kGlitchDivision, kGlitchChance, kGlitchReverse, kRepeatOn, kRepeatMix, kRepeatDivision, kBypassReserved /* global BYPASS button removed; slot kept for legacy state IDs */, kLfo0, kLegacyCount = kLfo0 + qg::lfoCount * (9 + 6), kGlitchSeq = kLegacyCount, kRepeatSeq, kGlitchStep0, kRepeatStep0 = kGlitchStep0+16, kRepeatRate0 = kRepeatStep0+16, kRepeatPitch0 = kRepeatRate0+16, kStretchOn = kRepeatPitch0+16, kStretchSpeed, kRandomSteps0, kDensityFlow = kRandomSteps0+4, kTranspose, kXYX, kXYY, kXYEnable, kXTarget, kYTarget, kXAmount, kYAmount, kModuleOrder, kExtraRoutes0, kGlitchMove = kExtraRoutes0 + 8, kGlitchVariation, kGlitchRefresh, kRepeatAuto, kRepeatInterval, kRepeatDuration, kRepeatChance, kUiStep, kUiGlitch, kUiRepeat, kUiLevel, kMasterFilter, kFilterType, kFilterCutoff, kFilterResonance, kMasterLimiter, kNormalize, kFilterSlope, kFilterDrive, kGrainEnabled, kGlitchEnabled, kRepeatEnabled, kGrainPan, kPanMode, kReverbOn, kReverbType, kReverbGrid, kReverbMix, kReverbStep0, kUiReverb=kReverbStep0+16, kReverbLength, kUiWave0, kUiGrainStart=kUiWave0+128, kUiGrainEnd, kUiGrainHead, kUiGrainActive, kUiWaveSeconds, kLfoSlots0, kLfoSpeed0=kLfoSlots0+48, kReverbKill=kLfoSpeed0+4, kUiLfoPhase0, kUiLfoCycle0=kUiLfoPhase0+4, kUiLfoEpoch0=kUiLfoCycle0+4, kModWaveRnd0=kUiLfoEpoch0+4, kUiModWave0=kModWaveRnd0+4, kReverbSource=kUiModWave0+4, kReverbRandomRate, kUiReverbGate, kLimiterCeiling, kGaterEnabled, kGaterGrid, kGaterLengthRnd, kGaterStepRnd, kGaterChance, kGaterState0, kGaterLength0=kGaterState0+16, kUiGaterStep=kGaterLength0+16, kUiGaterState0, kUiGaterLength0=kUiGaterState0+16, kInputDeclick=kUiGaterLength0+16, kDeclickSensitivity, kFilterModel, kResliceEnabled, kResliceLength, kResliceMix, kResliceStep0, kResliceIndex0=kResliceStep0+16, kGaterLatch=kResliceIndex0+16, kGaterRelease0, kUiResliceStep=kGaterRelease0+16, kUiResliceActive, kReverbModel, kGaterMinLength, kGaterTie, kGaterSustain0, kGlitchTriggerRate=kGaterSustain0+16, kResliceRndOn, kResliceRndRate, kFilterSeqOn, kFilterSeqMode, kFilterSeqPattern, kFilterSeqRate, kFilterSeqDepth, kFilterSeqGlide, kCombRoot, kCombOctave, kCombScale, kUiResliceSource0, kUiFilterSeqStep=kUiResliceSource0+16, kGrainBuffer, kFreeze = kGrainBuffer+4, kRandomAll, kPresetPrev, kPresetNext, kUiTab, kSlotPolarity0, kRoutingOrder=kSlotPolarity0+24, kUiWaveLow0, kUiWaveHigh0=kUiWaveLow0+256, kCount=kUiWaveHigh0+256 };
// Tab strip: the five top-row modules — Granulizer / PreSlicer / BeatRepeater /
// Reslice / Gater — each own their own CARD: one shared panel area spanning the
// canvas from extreme left to extreme right (x=16..1304, y=98..502), switched
// with FIVE buttons placed on top (no solo buttons anywhere). Under the cards
// sit Modulation / Morph / Filter / Reverb, always visible. kUiTab is a UI-only
// monitor parameter (like kUiStep/kUiGlitch: never touched by DSP or preset
// files) carrying the active card:
//   0 GRANULIZER   1 PRESLICER   2 BEAT REPEATER   3 RESLICE   4 GATER
// Switching cards flips between the five processors instantly, which is how you
// morph between different artefacts. The audio chain itself keeps following the
// existing AUDIO ORDER control (kModuleOrder -> aztec::moduleOrders), so no DSP
// changes are involved.
constexpr int tabCount=5;
constexpr int kLegacySkinCount=int(kUiTab)+1;
constexpr int waveformBins=256;
constexpr int slotPolarity(int l,int slot){return kSlotPolarity0+l*6+slot;}
// 120 permutations without changing the six legacy order values.
inline std::array<int,5> fiveModuleOrder(int index){
 std::array<int,5> pool{{0,1,2,3,4}},out{};const int factorial[]={24,6,2,1,1};
 index=std::clamp(index,0,119);for(int i=0;i<5;++i){int n=index/factorial[i];index%=factorial[i];out[i]=pool[n];for(int j=n;j<4-i;++j)pool[j]=pool[j+1];}return out;
}
// Tab values are stored normalized over (tabCount-1) so the full 0..1 range
// maps exactly onto the five cards.
inline double tabToValue(int tab){return double(std::clamp(tab,0,tabCount-1))/double(tabCount-1);}
inline int tabFromValue(double v){return std::clamp(int(std::round(v*(tabCount-1))),0,tabCount-1);}
constexpr int tabPermutation(int){return -1;}          // legacy shim: no pinned permutations
inline bool tabIsArtifact(int){return false;}          // legacy shim: no stacked artefact tabs
// Which top-row modules are VISIBLE for a card (controls are registered/hit-
// tested/drawn only for the visible stage — hidden stages leave no hit areas).
inline bool tabShowsGranular(int tab){return tab==0;}
inline bool tabShowsPreslicer(int tab){return tab==1;}
inline bool tabShowsRepeater(int tab){return tab==2;}
inline bool tabShowsReslice(int tab){return tab==3;}
inline bool tabShowsGater(int tab){return tab==4;}
inline bool tabShowsStagePanel(int tab,int stage){
  switch(stage){
    case 0:return tabShowsGranular(tab);
    case 1:return tabShowsPreslicer(tab);
    case 2:return tabShowsRepeater(tab);
    case 3:return tabShowsReslice(tab);
    default:return tabShowsGater(tab);
  }
}
// Card rect (x,y,w,h): every card paints its ONE visible module panel at the
// same full-width place (x=16, y=98, 1288x404), from extreme left to extreme
// right of the canvas.
inline void tabSlotRect(int,int,double& x,double& y,double& w,double& h){
  x=16.;y=98;w=1288;h=404;
}
enum LfoParam { lEnabled, lWave, lSync, lHz, lGrid, lDepth, lPhase, lReset, lGlide, lRoute0, lStride = lRoute0 + 6 };
constexpr std::array<std::array<int,3>,6> moduleOrders={{{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}}};
constexpr std::array<double,21> lfoBeats={32.,16.,8.,4.,2.,1.,.5,.25,.125,.0625,2./3.,1./3.,1./6.,1./12.,1.5,.75,.375,.1875,64.,128.,256.};
constexpr std::array<double,16> effectDivisions={4.,2.,1.,.5,.25,.125,.0625,.03125,2./3.,1./3.,1./6.,1./12.,1.5,.75,.375,.1875};
// Modulation depth for the accent-coloured bar drawn around each knob: the
// engine's live per-target modulation magnitudes (LFOs + XY morph already baked
// in) at the four primary destinations, mapped onto their parameter IDs.
// Editors refresh it at ~30fps via aztec::updateModDepths(engine.unpackMods()).
inline constexpr int modDestCount=qg::modTargetCount;
inline std::array<double,kCount> modulationDepths(const std::array<double,qg::modTargetCount>& mods) {
  std::array<double,kCount> d{};
  // Engine destination order: 0 Size, 1 Density, 2 Pitch, 4 Chaos.
  const int ids[4]={kSize,kDensity,kPitch,kChaos};
  for(int i=0;i<4;++i)d[ids[i]]+=std::abs(mods[i]);
  for(auto& v:d)v=std::clamp(v,0.,1.);
  return d;
}

constexpr const char* waveFamilies[]={"Sine Warp","Skew Triangle","Pulse","Rise Curve","Fall Curve","Harmonic","Staircase","Random Curve"};
constexpr const char* targetNames[]={"Size","Density","Pitch","Lookback","Chaos","Granular Mix","Speed","Transpose"};
constexpr int lfoID(int index,int offset) { return kLfo0+index*lStride+offset; }
constexpr std::array<double, 9> divisions = {1., .5, .25, .125, 1./3., 1./6., .75, .375, .1875};
}

namespace aztec {
// Append new routes without changing any existing host automation IDs.
constexpr int routeID(int lfo,int target) { return target<6 ? lfoID(lfo,lRoute0+target) : kExtraRoutes0+lfo*2+(target-6); }
}

namespace aztec { constexpr std::array<double,8> captureIntervals={.25,.5,1.,2.,4.,8.,16.,32.}; }

namespace aztec { constexpr int presetCount=kCount; constexpr bool isMonitor(int id){return (id>=kUiStep&&id<=kUiLevel)||id==kUiReverb||(id>=kUiWave0&&id<=kUiWaveSeconds)||(id>=kUiLfoPhase0&&id<kModWaveRnd0)||(id>=kUiModWave0&&id<kReverbSource)||id==kUiReverbGate||(id>=kUiGaterStep&&id<kInputDeclick)||id==kUiResliceStep||id==kUiResliceActive||(id>=kUiResliceSource0&&id<=kUiFilterSeqStep)||id==kUiTab||(id>=kUiWaveLow0&&id<kCount);} }

namespace aztec {
constexpr int slotTarget(int l,int slot){return kLfoSlots0+l*12+slot*2;}
constexpr int slotAmount(int l,int slot){return slotTarget(l,slot)+1;}
inline constexpr auto& modNames=qg::modulationNames;
inline void migrateRoutes(std::array<double,kCount>& p){for(int l=0;l<4;++l){int slot=0;for(int t=0;t<8;++t){int old=routeID(l,t);if(std::abs(p[old]-.5)>1e-12&&slot<6){p[slotTarget(l,slot)]=(t+1)/double(qg::modTargetCount);p[slotAmount(l,slot)]=p[old];p[old]=.5;++slot;}}}}
}

namespace aztec { inline void migrateModSlots(std::array<double,kCount>& p){for(int l=0;l<4;++l)for(int s=0;s<6;++s)p[slotTarget(l,s)]=std::round(p[slotTarget(l,s)]*11.)/qg::modTargetCount;} }

namespace aztec {
// Live modulation-depth map consumed by both editors' knob bars. The editor's
// 30fps tick refreshes it from its own controller snapshot before drawing.
// 8.8 fixed-point packing of the four primary modulation magnitudes
// (Size, Density, Pitch, Chaos) into one atomic 64-bit word for lock-free
// audio->UI hand-off.
inline uint64_t packModMagnitudes(const std::array<double,qg::modTargetCount>& m) {
  auto q=[](double x)->uint64_t{return uint64_t(uint16_t(int16_t(std::clamp(x,-128.,127.)*256.)))&0xFFFFu;};
  // Engine destination order: 0 Size, 1 Density, 2 Pitch, 4 Chaos (see modulationDepths).
  return q(m[0])|(q(m[1])<<16)|(q(m[2])<<32)|(q(m[3])<<48);
}
inline std::array<double,qg::modTargetCount> unpackModMagnitudes(uint64_t p) {
  std::array<double,qg::modTargetCount> m{};
  auto d=[p](int shift)->double{int16_t raw=int16_t((p>>shift)&0xFFFFu);return raw/256.;};
  m[0]=d(0);m[1]=d(16);m[2]=d(32);m[3]=d(48);return m;
}
inline std::array<double,kCount>& modDepthMap() { static std::array<double,kCount> m{}; return m; }
inline void updateModDepths(const std::array<double,qg::modTargetCount>& mods) { modDepthMap()=modulationDepths(mods); }
inline double modDepth(int id) { return id>=0&&id<int(kCount)?modDepthMap()[id]:0.; }
}

