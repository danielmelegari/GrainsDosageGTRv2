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
enum Param : ParamID { kDivision, kSize, kDensity, kPitch, kPosition, kChaos, kMix, kAttack, kRelease, kFeedback, kStep0, kGrainMix = kStep0 + 16, kGlitchMix, kGlitchDivision, kGlitchChance, kGlitchReverse, kRepeatOn, kRepeatMix, kRepeatDivision, kBypassReserved /* global BYPASS button removed; slot kept for legacy state IDs */, kLfo0, kLegacyCount = kLfo0 + qg::lfoCount * (9 + 6), kGlitchSeq = kLegacyCount, kRepeatSeq, kGlitchStep0, kRepeatStep0 = kGlitchStep0+16, kRepeatRate0 = kRepeatStep0+16, kRepeatPitch0 = kRepeatRate0+16, kStretchOn = kRepeatPitch0+16, kStretchSpeed, kRandomSteps0, kDensityFlow = kRandomSteps0+4, kTranspose, kXYX, kXYY, kXYEnable, kXTarget, kYTarget, kXAmount, kYAmount, kModuleOrder, kExtraRoutes0, kGlitchMove = kExtraRoutes0 + 8, kGlitchVariation, kGlitchRefresh, kRepeatAuto, kRepeatInterval, kRepeatDuration, kRepeatChance, kUiStep, kUiGlitch, kUiRepeat, kUiLevel, kMasterFilter, kFilterType, kFilterCutoff, kFilterResonance, kMasterLimiter, kNormalize, kFilterSlope, kFilterDrive, kGrainEnabled, kGlitchEnabled, kRepeatEnabled, kGrainPan, kPanMode, kReverbOn, kReverbType, kReverbGrid, kReverbMix, kReverbStep0, kUiReverb=kReverbStep0+16, kReverbLength, kUiWave0, kUiGrainStart=kUiWave0+128, kUiGrainEnd, kUiGrainHead, kUiGrainActive, kUiWaveSeconds, kLfoSlots0, kLfoSpeed0=kLfoSlots0+48, kReverbKill=kLfoSpeed0+4, kUiLfoPhase0, kUiLfoCycle0=kUiLfoPhase0+4, kUiLfoEpoch0=kUiLfoCycle0+4, kModWaveRnd0=kUiLfoEpoch0+4, kUiModWave0=kModWaveRnd0+4, kReverbSource=kUiModWave0+4, kReverbRandomRate, kUiReverbGate, kLimiterCeiling, kGaterEnabled, kGaterGrid, kGaterLengthRnd, kGaterStepRnd, kGaterChance, kGaterState0, kGaterLength0=kGaterState0+16, kUiGaterStep=kGaterLength0+16, kUiGaterState0, kUiGaterLength0=kUiGaterState0+16, kInputDeclick=kUiGaterLength0+16, kDeclickSensitivity, kFilterModel, kResliceEnabled, kResliceLength, kResliceMix, kResliceStep0, kResliceIndex0=kResliceStep0+16, kGaterLatch=kResliceIndex0+16, kGaterRelease0, kUiResliceStep=kGaterRelease0+16, kUiResliceActive, kReverbModel, kGaterMinLength, kGaterTie, kGaterSustain0, kGlitchTriggerRate=kGaterSustain0+16, kResliceRndOn, kResliceRndRate, kFilterSeqOn, kFilterSeqMode, kFilterSeqPattern, kFilterSeqRate, kFilterSeqDepth, kFilterSeqGlide, kCombRoot, kCombOctave, kCombScale, kUiResliceSource0, kUiFilterSeqStep=kUiResliceSource0+16, kGrainBuffer, kFreeze = kGrainBuffer+4, kRandomAll, kPresetPrev, kPresetNext, kCount };
enum LfoParam { lEnabled, lWave, lSync, lHz, lGrid, lDepth, lPhase, lReset, lGlide, lRoute0, lStride = lRoute0 + 6 };
constexpr std::array<std::array<int,3>,6> moduleOrders={{{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}}};
constexpr std::array<double,21> lfoBeats={32.,16.,8.,4.,2.,1.,.5,.25,.125,.0625,2./3.,1./3.,1./6.,1./12.,1.5,.75,.375,.1875,64.,128.,256.};
constexpr std::array<double,16> effectDivisions={4.,2.,1.,.5,.25,.125,.0625,.03125,2./3.,1./3.,1./6.,1./12.,1.5,.75,.375,.1875};
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

namespace aztec { constexpr int presetCount=kCount; constexpr bool isMonitor(int id){return (id>=kUiStep&&id<=kUiLevel)||id==kUiReverb||(id>=kUiWave0&&id<=kUiWaveSeconds)||(id>=kUiLfoPhase0&&id<kModWaveRnd0)||(id>=kUiModWave0&&id<kReverbSource)||id==kUiReverbGate||(id>=kUiGaterStep&&id<kInputDeclick)||id==kUiResliceStep||id==kUiResliceActive||(id>=kUiResliceSource0&&id<=kUiFilterSeqStep);} }

namespace aztec {
constexpr int slotTarget(int l,int slot){return kLfoSlots0+l*12+slot*2;}
constexpr int slotAmount(int l,int slot){return slotTarget(l,slot)+1;}
inline constexpr auto& modNames=qg::modulationNames;
inline void migrateRoutes(std::array<double,kCount>& p){for(int l=0;l<4;++l){int slot=0;for(int t=0;t<8;++t){int old=routeID(l,t);if(std::abs(p[old]-.5)>1e-12&&slot<6){p[slotTarget(l,slot)]=(t+1)/double(qg::modTargetCount);p[slotAmount(l,slot)]=p[old];p[old]=.5;++slot;}}}}
}

namespace aztec { inline void migrateModSlots(std::array<double,kCount>& p){for(int l=0;l<4;++l)for(int s=0;s<6;++s)p[slotTarget(l,s)]=std::round(p[slotTarget(l,s)]*11.)/qg::modTargetCount;} }
