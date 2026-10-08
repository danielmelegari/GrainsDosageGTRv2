#pragma once
#include "engine.h"
namespace aztec {
inline double value(const std::array<double, kCount>& p, int id) { return std::clamp(p[id], 0., 1.); }
inline qg::Settings settings(const std::array<double, kCount>& saved, double tempo, double rate) {
  auto p=saved;
  if(value(p,kXYEnable)>=.5) {
    for(int axis=0;axis<2;++axis) {
      const int target=int(std::round(value(saved,axis?kYTarget:kXTarget)*int(kXYX)))-1;
      if(target>=0 && target<int(kXYX)) p[target]=std::clamp(p[target]+(value(saved,axis?kXYY:kXYX)*2.-1.)*(value(saved,axis?kYAmount:kXAmount)*2.-1.),0.,1.);
    }
  }
  qg::Settings s; s.tempo = tempo; s.sampleRate = rate;
  s.division = .25; // Fixed sixteenth-note density clock; no grain gate.
  s.size = .015 + value(p, kSize) * .235;
  s.density = 1. + value(p, kDensity) * 31.;
  s.pitch = std::round((value(p, kPitch) - .5) * 96.);
  s.position = .015 + value(p, kPosition) * 1.985;
  s.chaos = value(p, kChaos); s.mix = value(p, kMix);
  s.attack = .002;
  s.release = .004;
  s.feedback = 0.; s.pattern = 0;
  s.normalize=value(p,kNormalize)>=.5;
  s.moduleOn={{value(p,kGrainEnabled)>=.5,value(p,kGlitchEnabled)>=.5,value(p,kRepeatEnabled)>=.5}};
  s.grainPan=value(p,kGrainPan)*2.-1.;s.panMode=int(std::round(value(p,kPanMode)*2.));
  s.filterSequence.enabled=value(p,kFilterSeqOn)>=.5;s.filterSequence.mode=int(std::round(value(p,kFilterSeqMode)));s.filterSequence.pattern=int(std::round(value(p,kFilterSeqPattern)*63.));
  const double filterRates[]={1.,.5,.25,.125,2.,4.};s.filterSequence.rate=filterRates[int(std::round(value(p,kFilterSeqRate)*5.))];s.filterSequence.depth=value(p,kFilterSeqDepth);s.filterSequence.glide=value(p,kFilterSeqGlide);
  s.filterSequence.root=int(std::round(value(p,kCombRoot)*11.));s.filterSequence.octave=int(std::round(value(p,kCombOctave)*6.));s.filterSequence.scale=int(std::round(value(p,kCombScale)));
  s.filterOn=value(p,kMasterFilter)>=.5;s.filterType=int(std::round(value(p,kFilterType)*2.));s.filterSlope=value(p,kFilterSlope)>=.5?1:0;s.filterCutoff=value(p,kFilterCutoff);s.filterResonance=value(p,kFilterResonance);s.filterDrive=value(p,kFilterDrive);
  s.reverbModel=int(std::round(value(p,kReverbModel)*4.));s.filterModel=int(std::round(value(p,kFilterModel)*(qg::filterModelCount-1)));s.reslice.enabled=value(p,kResliceEnabled)>=.5;const double resliceLengths[]={16.,8.,4.,2.};s.reslice.beats=resliceLengths[int(std::round(value(p,kResliceLength)*3.))];s.reslice.mix=value(p,kResliceMix);s.reslice.random=value(p,kResliceRndOn)>=.5;s.reslice.randomBeats=2.*std::pow(2.,std::round(value(p,kResliceRndRate)*2.));s.gater.latch=value(p,kGaterLatch)>=.5;s.gater.tie=false; /* Tie removed from the module (LATCH kept). */ s.gater.minimumLength=.05+.90*value(p,kGaterMinLength);for(int i=0;i<16;++i){s.reslice.on[i]=value(p,kResliceStep0+i)>=.5;s.reslice.slice[i]=int(std::round(value(p,kResliceIndex0+i)*15.));if(value(p,kGaterRelease0+i)>=.5)s.gater.release|=uint16_t(1)<<i;}
  s.gater.enabled=value(p,kGaterEnabled)>=.5;s.gater.grid=std::pow(.5,int(std::round(value(p,kGaterGrid)*3.)));s.gater.lengthRandom=value(p,kGaterLengthRnd)>=.5;s.gater.stepRandom=value(p,kGaterStepRnd)>=.5;s.gater.chance=value(p,kGaterChance);for(int i=0;i<16;++i){s.gater.state[i]=value(p,kGaterState0+i)>=.25?1:0;s.gater.length[i]=.05+.95*value(p,kGaterLength0+i);s.gater.sustain[i]=.5*value(p,kGaterSustain0+i);}
  s.reverbKill=value(p,kReverbKill)>=.5;s.reverbRandom=value(p,kReverbSource)>=.5;s.reverbRandomGrid=reverbRate(value(p,kReverbRateV2),value(p,kReverbRandomRate),true);
  s.reverbOn=value(p,kReverbOn)>=.5;s.reverbType=int(std::round(value(p,kReverbType)*4.));
  s.reverbGrid=reverbRate(value(p,kReverbRateV2),value(p,kReverbGrid),false);s.reverbMix=value(p,kReverbMix);
  s.reverbLength=.2+value(p,kReverbLength)*19.8;
  s.reverbPattern=0;for(int i=0;i<16;++i)if(value(p,kReverbStep0+i)>=.5)s.reverbPattern|=uint16_t(1u<<i);
  s.pattern=0xffff; // Grain gate removed; retained IDs only for old preset compatibility.
  s.grainMix = value(p,kGrainMix); s.glitchMix = value(p,kGlitchMix);
  s.glitchBeats = effectDivisions[std::min(15,int(std::round(value(p,kGlitchDivision)*15.)))];
  s.glitchChance = value(p,kGlitchChance); s.glitchReverse = value(p,kGlitchReverse) >= .5;
  s.repeatOn = value(p,kRepeatOn) >= .5; s.repeatMix = value(p,kRepeatMix);
  s.repeatBeats = effectDivisions[std::min(15,int(std::round(value(p,kRepeatDivision)*15.)))];
  s.glitchRandom=true;s.glitchTriggerBeats=std::pow(2.,std::round(value(p,kGlitchTriggerRate)*3.));s.glitchSequence=false; s.repeatSequence=value(p,kRepeatSeq)>=.5;
  s.glitchPattern=s.repeatPattern=0;
  for(int i=0;i<16;++i) {
    if(value(p,kGlitchStep0+i)>=.5) s.glitchPattern|=uint16_t(1u<<i);
    if(value(p,kRepeatStep0+i)>=.5) s.repeatPattern|=uint16_t(1u<<i);
    const int rate=int(std::round(value(p,kRepeatRate0+i)*15.)); // "Global" removed; 16-entry table now.
    s.repeatRates[i]=effectDivisions[std::clamp(rate,0,15)];
    s.repeatPitches[i]=std::round(value(p,kRepeatPitch0+i)*96.-48.);
  }
  // PRESLICER: the MOVE slider was removed from the GUI; keep a fixed centred offset for old presets.
  s.glitchMove=.5;s.glitchVariation=value(p,kGlitchVariation);
  s.glitchRefresh=captureIntervals[int(std::round(value(p,kGlitchRefresh)*7.))];
  s.repeatAuto=value(p,kRepeatAuto)>=.5;s.repeatInterval=captureIntervals[int(std::round(value(p,kRepeatInterval)*7.))];
  s.repeatDuration=.25+value(p,kRepeatDuration)*7.75;s.repeatChance=value(p,kRepeatChance);
  s.order=std::min(6,int(std::round(value(p,kModuleOrder)*6.)));
  s.routing=int(std::round(value(p,kRoutingOrder)*120.))-1;
  // Global BYPASS button removed from the plugin: kBypassReserved stays in the enum only so
  // legacy state/preset IDs keep their positions, but it is never read anymore.
  s.bypass=false; s.densityFlow=value(p,kDensityFlow)>=.5; s.transpose=value(p,kTranspose)*96.-48.;
  s.stretchOn=value(p,kStretchOn)>=.5; s.stretchSpeed=.25+value(p,kStretchSpeed)*3.75;
  // BUFFER SIZE: kGrainBuffer selects one of 1/2/4/8/16 s (live resize keeps the audible tail).
  {const double bufferSeconds[]={1.,2.,4.,8.,16.};int bucket=int(std::clamp(value(p,kGrainBuffer)*4.,0.,4.));s.bufferSeconds=bufferSeconds[bucket];}
  // FREEZE is momentary on screen: p_[kFreeze] carries a one-shot request that
  // process() consumes below; the sustained state lives in the engine and is
  // mirrored back into p_ so getState/save always capture the true frozen flag.
  s.freeze=false; // never carried through settings(): toggled directly on the engine
  for(int i=0;i<qg::lfoCount;++i) {
    auto& l=s.lfos[i]; const int base=lfoID(i,0);
    l.enabled=value(p,base+lEnabled)>=.5; l.wave=int(std::round(value(p,base+lWave)*129.));
    l.sync=value(p,base+lSync)>=.5; l.hz=.01+value(p,base+lHz)*39.99;
    l.beats=lfoBeats[std::clamp(int(std::round(value(p,base+lGrid)*20.)),0,20)];
    l.randomSteps=1+int(std::round(value(p,kRandomSteps0+i)*63.));
    l.depth=value(p,base+lDepth); l.phase=value(p,base+lPhase);
    l.gateReset=value(p,base+lReset)>=.5; l.glide=.01+value(p,base+lGlide)*.99;
    for(int t=0;t<8;++t) l.amount[t]=value(p,routeID(i,t))*2.-1.;
    const double speeds[]={.25,.5,1.,2.};l.speed=speeds[int(std::round(value(p,kLfoSpeed0+i)*3.))];l.waveRandom=int(std::round(value(p,kModWaveRnd0+i)*4.));
    for(int slot=0;slot<6;++slot){int target=int(std::round(value(p,slotTarget(i,slot))*qg::modTargetCount))-1;if(target>=0&&target<qg::modTargetCount){double amount=value(p,slotAmount(i,slot))*2.-1.;int polarity=int(std::round(value(p,slotPolarity(i,slot))*2.));if(polarity==1)l.positive[target]+=std::abs(amount);else if(polarity==2)l.negative[target]+=std::abs(amount);else l.amount[target]+=amount;}}
  }
  return s;
}
}
