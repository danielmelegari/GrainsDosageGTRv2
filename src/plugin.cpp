#include "engine.h"
#include "declick.h"
#include "master_fx.h"
#include "parameters.h"
#include "factory_presets.h"
#include "editor.h"
#include <cstring>
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "public.sdk/source/vst/vstparameters.h"
#include <cstdio>
#include "public.sdk/source/main/pluginfactory.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include <array>
#include <cmath>
#include <string>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace {
static const FUID processorID(0xB318D832, 0xC27E4F25, 0xBD8E13A5, 0x31470211);
static const FUID controllerID(0xD8FDFEB3, 0xA52848AB, 0x8B8706A4, 0xDA21E077);
using namespace aztec;
double value(const std::array<double, kCount>& p, int id) { return std::clamp(p[id], 0., 1.); }
qg::Settings settings(const std::array<double, kCount>& saved, double tempo, double rate) {
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
  s.reverbKill=value(p,kReverbKill)>=.5;s.reverbRandom=value(p,kReverbSource)>=.5;s.reverbRandomGrid=std::pow(.5,int(std::round(value(p,kReverbRandomRate)*2.)));
  s.reverbOn=value(p,kReverbOn)>=.5;s.reverbType=int(std::round(value(p,kReverbType)*4.));
  s.reverbGrid=std::pow(.5,int(std::round(value(p,kReverbGrid)*3.)));s.reverbMix=value(p,kReverbMix);
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
  s.bypass=value(p,kBypass)>=.5; s.densityFlow=value(p,kDensityFlow)>=.5; s.transpose=value(p,kTranspose)*96.-48.;
  s.stretchOn=value(p,kStretchOn)>=.5; s.stretchSpeed=.25+value(p,kStretchSpeed)*3.75;
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
    for(int slot=0;slot<6;++slot){int target=int(std::round(value(p,slotTarget(i,slot))*qg::modTargetCount))-1;if(target>=0&&target<qg::modTargetCount)l.amount[target]+=value(p,slotAmount(i,slot))*2.-1.;}
  }
  return s;
}
std::array<double,kCount> defaults(){return initialParameters();}
bool loadState(IBStream* stream, std::array<double,kCount>& p) {
  IBStreamer in(stream,kLittleEndian); int32 magic=0;
  if(!in.readInt32(magic) || (magic!=0x51473130 && magic!=0x51473131 && magic!=0x51473132 && magic!=0x51473133 && magic!=0x51473134 && magic!=0x51473135 && magic!=0x51473136 && magic!=0x51473137 && magic!=0x51473138 && magic!=0x51473139 && magic!=0x5147313A && magic!=0x5147313B && magic!=0x5147313C && magic!=0x5147313D && magic!=0x5147313E && magic!=0x5147313F && magic!=0x51473140 && magic!=0x51473141 && magic!=0x51473142 && magic!=0x51473143 && magic!=0x51473144)) return false;
  auto result=defaults();
  const int count = magic==0x51473144 ? int(kCount) : magic==0x51473143 ? int(kResliceRndOn) : magic==0x51473130 ? int(kGrainMix) : (magic==0x51473131 ? int(kLfo0) : (magic==0x51473132 ? int(kLegacyCount) : (magic==0x51473133 ? int(kModuleOrder) : (magic==0x51473134 ? int(kExtraRoutes0) : (magic==0x51473135 ? int(kGlitchMove) : (magic==0x51473136 ? int(kMasterFilter) : (magic==0x51473137 ? int(kNormalize) : (magic==0x51473138 ? int(kReverbLength) : (magic==0x51473139 ? int(kUiWave0) : (magic==0x5147313A ? int(kLfoSlots0) : (magic==0x5147313B ? int(kModWaveRnd0) : (magic==0x5147313C ? int(kReverbSource) : (magic==0x5147313D ? int(kLimiterCeiling) : (magic==0x5147313E ? int(kGaterEnabled) : (magic==0x5147313F ? int(kInputDeclick) : (magic==0x51473140 ? int(kFilterModel) : (magic==0x51473141 ? int(kGaterMinLength) : int(kGlitchTriggerRate))))))))))))))))));
  for(int i=0;i<count;++i) {
    double v=0.; if(!in.readDouble(v) || !std::isfinite(v)) return false;
    result[i]=std::clamp(v,0.,1.);
  }
  if(magic==0x51473133) result[kModuleOrder]=1.; // Preserve version 0.5 parallel routing.
  if(magic<0x51473133) {
    result[kModuleOrder]=1.;
    result[kDensityFlow]=0.; result[kDensity]*=7./31.;
    result[kPitch]=.5+(result[kPitch]-.5)*.25;
    for(int id:{int(kGlitchDivision),int(kRepeatDivision)}) {
      // Missing fields already contain current defaults.
      if(id<count) {
        const double old=divisions[int(std::round(result[id]*8.))];
        for(int j=0;j<16;++j) if(effectDivisions[j]==old) result[id]=j/15.;
      }
    }
    result[kGlitchSeq]=result[kRepeatSeq]=0.;
    for(int i=0;i<4;++i) {
      result[kRandomSteps0+i]=0.;
      if(count>int(kLfo0)) {
        result[lfoID(i,lGrid)]=std::round(result[lfoID(i,lGrid)]*17.)/20.;
        auto& density=result[lfoID(i,lRoute0+1)]; density=.5+(density-.5)*7./31.;
        auto& route=result[lfoID(i,lRoute0+2)]; route=.5+(route-.5)*.25;
      }
    }
  }
  if(magic<0x51473139){const double rt[]={.45,1.25,2.8,5.5,9.};result[kReverbLength]=(rt[int(std::round(result[kReverbType]*4.))]-.2)/19.8;}
  if(magic<0x51473137)result[kMasterLimiter]=0.; // Preserve old saved output levels.
  if(magic<0x5147313B)migrateRoutes(result);else if(magic<0x51473144)migrateModSlots(result);
  if(magic<0x51473140)result[kInputDeclick]=0.;
  if(magic<0x51473144&&count>kResliceLength)result[kResliceLength]=result[kResliceLength]<1./6.?2./3.:1.;
  p=result; return true;
}
class Processor final : public AudioEffect {
  qg::Engine engine_;
  qg::InputDeclick declick_;
  qg::MasterFx master_;
  std::array<double, kCount> p_{};
  int waveCountdown_=0;
  double rate_ = 44100., tempo_ = 120., fallbackBeat_ = 0.;
  // The parameter array covers every legacy + monitor ID; the VST parameter list stops before the new UI controls.
  static constexpr int kParamEnd = int(kUiFilterSeqStep) + 1;
  bool freezePending_ = false; // FREEZE is a momentary button: consumed by the next process() call
public:
  Processor() {
    setControllerClass(controllerID);
    p_=defaults();
    processContextRequirements.needTempo().needProjectTimeMusic().needTransportState();
  }
  static FUnknown* create(void*) { return static_cast<IAudioProcessor*>(new Processor); }
  tresult PLUGIN_API initialize(FUnknown* c) override {
    if(AudioEffect::initialize(c) != kResultOk) return kResultFalse;
    addAudioInput(STR16("Stereo In"), SpeakerArr::kStereo);
    addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);
    return kResultOk;
  }
  tresult PLUGIN_API setupProcessing(ProcessSetup& setup) override {
    rate_ = setup.sampleRate; engine_.prepare(rate_);declick_.prepare(rate_,p_[kInputDeclick]>=.5);
    return AudioEffect::setupProcessing(setup);
  }
  uint32 PLUGIN_API getLatencySamples() override { return qg::InputDeclick::latency; }
  tresult PLUGIN_API setState(IBStream* stream) override {
    return loadState(stream,p_) ? kResultOk : kResultFalse;
  }
  tresult PLUGIN_API getState(IBStream* stream) override {
    IBStreamer out(stream,kLittleEndian);
    if(!out.writeInt32(0x51473145)) return kResultFalse;
    for(int i=0;i<kParamEnd;++i) if(!out.writeDouble(p_[i])) return kResultFalse;
    // v0.14 additions: buffer size, freeze state, then RANDOM/PRESET ids kept for array alignment.
    if(!out.writeDouble(value(p_,kGrainBuffer))) return kResultFalse;
    if(!out.writeDouble(engine_.isFrozen()?1.:0.)) return kResultFalse;
    for(int i=kGrainBuffer+2;i<int(kCount);++i) if(!out.writeDouble(p_[i])) return kResultFalse;
    return kResultOk;
  }
  tresult PLUGIN_API canProcessSampleSize(int32 size) override {
    return size==kSample32 || size==kSample64 ? kResultTrue : kResultFalse;
  }
  tresult PLUGIN_API setBusArrangements(SpeakerArrangement* in,int32 ni,SpeakerArrangement* out,int32 no) override {
    if(ni!=1 || no!=1 || in[0]!=SpeakerArr::kStereo || out[0]!=SpeakerArr::kStereo) return kResultFalse;
    return AudioEffect::setBusArrangements(in,ni,out,no);
  }
  tresult PLUGIN_API setActive(TBool active) override {
    if(active) { engine_.prepare(rate_);declick_.prepare(rate_,p_[kInputDeclick]>=.5);master_.prepare(rate_); fallbackBeat_=0.;waveCountdown_=0; }
    return AudioEffect::setActive(active);
  }
  // NOTE: Processor must NOT override setComponentState — AudioEffect does not
  // declare it virtual (only IComponent via FUnknown), so `override` fails to
  // compile here. The Controller below receives setComponentState and applies
  // the full parameter array through updateStates().
  tresult PLUGIN_API process(ProcessData& data) override {
    // No allocations: fixed queue cursors, updated at the exact sample offset.
    std::array<IParamValueQueue*, kCount> queues{};
    std::array<int32,kCount> cursors{};std::array<int,kCount> queueIDs{};int queueCount=0;
    if(data.inputParameterChanges) {
      for(int32 i=0;i<data.inputParameterChanges->getParameterCount();++i) {
        auto* q=data.inputParameterChanges->getParameterData(i);
        if(q && q->getParameterId()<kCount && !isMonitor(q->getParameterId())) {int id=int(q->getParameterId());if(!queues[id])queueIDs[queueCount++]=id;queues[id]=q;}
      }
    }
    auto applyChanges = [&](int32 sample) {
      bool changed=false;
      for(int qi=0;qi<queueCount;++qi) {int id=queueIDs[qi];
        auto* q=queues[id]; int32 offset=0; ParamValue v=0.;
        while(cursors[id]<q->getPointCount() && q->getPoint(cursors[id],offset,v)==kResultOk && offset<=sample) {
          if(std::isfinite(v)) { p_[id]=std::clamp(v,0.,1.); changed=true; }
          ++cursors[id];
        }
      }
      return changed;
    };
    applyChanges(0);
    if(data.processContext) {
      if(data.processContext->state & ProcessContext::kTempoValid) tempo_ = std::isfinite(data.processContext->tempo) ? std::clamp(data.processContext->tempo,20.,400.) : 120.;
      if((data.processContext->state & ProcessContext::kProjectTimeMusicValid) && std::isfinite(data.processContext->projectTimeMusic)) {
        const double beat=data.processContext->projectTimeMusic;
        if(std::abs(beat-fallbackBeat_)>2.*tempo_/(60.*rate_)) engine_.resetTransport();
        fallbackBeat_=beat;
      }
    }
    if(data.numInputs < 1 || data.numOutputs < 1 || data.inputs[0].numChannels<1 || data.outputs[0].numChannels<1) return kResultOk;
    data.outputs[0].silenceFlags=3;
    auto s = settings(p_, tempo_, rate_);s.playing=!data.processContext||(data.processContext->state & ProcessContext::kPlaying); engine_.set(s);
    auto setMaster=[&](){const double ceilings[]={0.,-6.,-10.};master_.set(false,0,1000.,0.,value(p_,kMasterLimiter)>=.5,0,0.,ceilings[int(std::round(value(p_,kLimiterCeiling)*2.))]);};
    setMaster();
    const double beatIncrement = tempo_ / (60. * rate_);
    double peak=0.;
    for(int32 n=0;n<data.numSamples;++n) {
      if(n>0 && applyChanges(n)) { s=settings(p_,tempo_,rate_);s.playing=!data.processContext||(data.processContext->state & ProcessContext::kPlaying); engine_.set(s);setMaster(); }
      const double beat = fallbackBeat_ + n * beatIncrement;
      if(data.symbolicSampleSize == kSample32) {
        auto& in = data.inputs[0]; auto& out = data.outputs[0];
        float l = (in.silenceFlags & 1) ? 0.f : in.channelBuffers32[0][n], r = in.numChannels > 1 ? ((in.silenceFlags & 2) ? 0.f : in.channelBuffers32[1][n]) : l;
        double dl=l,dr=r;declick_.process(dl,dr,value(p_,kInputDeclick)>=.5,value(p_,kDeclickSensitivity),s.bypass);l=float(dl);r=float(dr);
        float a,b; engine_.process(l,r,beat,a,b);
        if(s.bypass) { a=l; b=r; }else master_.process(a,b);
        peak=std::max({peak,std::abs(double(a)),std::abs(double(b))});
        out.channelBuffers32[0][n] = a;
        if(a!=0.f) out.silenceFlags &= ~uint64(1);
        if(b!=0.f) out.silenceFlags &= ~uint64(2);
        if(out.numChannels > 1) out.channelBuffers32[1][n] = b;
      } else if(data.symbolicSampleSize == kSample64) {
        auto& in = data.inputs[0]; auto& out = data.outputs[0];
        double l = (in.silenceFlags & 1) ? 0. : in.channelBuffers64[0][n];
        double r = in.numChannels>1 ? ((in.silenceFlags & 2) ? 0. : in.channelBuffers64[1][n]) : l;
        declick_.process(l,r,value(p_,kInputDeclick)>=.5,value(p_,kDeclickSensitivity),s.bypass);
        float a,b; engine_.process(float(l),float(r),beat,a,b);
        if(!s.bypass)master_.process(a,b);
        peak=std::max({peak,std::abs(s.bypass?l:double(a)),std::abs(s.bypass?r:double(b))});
        out.channelBuffers64[0][n] = s.bypass ? l : a;
        if(out.numChannels > 1) out.channelBuffers64[1][n] = s.bypass ? r : b;
        if(out.channelBuffers64[0][n]!=0.) out.silenceFlags &= ~uint64(1);
        if(out.numChannels>1 && out.channelBuffers64[1][n]!=0.) out.silenceFlags &= ~uint64(2);
      }
    }
    if(data.outputParameterChanges&&data.numSamples>0){
      // Report our bypass state back to the controller so host-side Bypass toggles
      // (validator "Parameter Bypass persistence" check, DAW bypass button) stay in sync.
      int32 bIdx=0; auto* bq=data.outputParameterChanges->addParameterData(ParamID(kBypass),bIdx);
      if(bq) bq->addPoint(data.numSamples-1, s.bypass?1.:0., bIdx);
      const double beat=fallbackBeat_+(data.numSamples-1)*beatIncrement;
      const int step=int((int64_t(std::floor(beat/.25))%16+16)%16);
      const double meters[]={step/15.,(!s.bypass&&engine_.glitchRunning())?1.:0.,(!s.bypass&&engine_.repeatRunning())?1.:0.,std::clamp(peak,0.,1.),double((int64_t(std::floor(beat/s.reverbGrid))%16+16)%16)/15.};
      for(int i=0;i<5;++i){int32 index=0;auto id=ParamID(i==4?kUiReverb:kUiStep+i);auto* q=data.outputParameterChanges->addParameterData(id,index);if(q)q->addPoint(data.numSamples-1,meters[i],index);}
    }
    waveCountdown_-=data.numSamples;
    if(data.outputParameterChanges&&data.numSamples>0&&waveCountdown_<=0){
      waveCountdown_=std::max(1,int(rate_/30.));auto wave=engine_.grainView();
      auto emit=[&](int id,double v){int32 index=0;auto* q=data.outputParameterChanges->addParameterData(ParamID(id),index);if(q)q->addPoint(data.numSamples-1,v,index);};
      for(int i=0;i<128;++i)emit(kUiWave0+i,wave.amplitude[i]);
      for(int i=0;i<16;++i)emit(kUiResliceSource0+i,engine_.resliceSource(i)/15.);emit(kUiFilterSeqStep,engine_.filterSequenceStep()/31.);emit(kUiResliceStep,engine_.resliceStep()/15.);emit(kUiResliceActive,engine_.resliceActive()?1.:0.);emit(kUiGaterStep,engine_.gaterStep()/15.);for(int i=0;i<16;++i){emit(kUiGaterState0+i,engine_.gaterState(i)?1.:0.);emit(kUiGaterLength0+i,engine_.gaterLength(i));}emit(kUiReverbGate,!s.bypass&&engine_.reverbGate()?1.:0.);emit(kUiGrainStart,wave.start);emit(kUiGrainEnd,wave.end);emit(kUiGrainHead,wave.head);emit(kUiGrainActive,!s.bypass&&wave.active?1.:0.);emit(kUiWaveSeconds,wave.seconds/16.);for(int i=0;i<4;++i){emit(kUiModWave0+i,engine_.lfoWave(i)/129.);emit(kUiLfoPhase0+i,engine_.lfoPhase(i));emit(kUiLfoCycle0+i,std::clamp((double(engine_.lfoCycle(i))+2147483648.)/4294967295.,0.,1.));emit(kUiLfoEpoch0+i,std::clamp((double(engine_.lfoEpoch(i))+2147483648.)/4294967295.,0.,1.));}
    }
    fallbackBeat_ += data.numSamples * beatIncrement;
    return kResultOk;
  }
};
class LogCutoffParameter final : public RangeParameter {
public:
  LogCutoffParameter():RangeParameter(STR16("Master Cutoff"),kFilterCutoff,STR16("Hz"),20.,20000.,1000.){info.defaultNormalizedValue=std::log(50.)/std::log(1000.);setNormalized(info.defaultNormalizedValue);}
  ParamValue toPlain(ParamValue v)const override{return 20.*std::pow(1000.,v);}
  ParamValue toNormalized(ParamValue v)const override{return std::log(std::clamp(v,20.,20000.)/20.)/std::log(1000.);}
};
class Controller final : public EditController {
public:
  static FUnknown* create(void*) { return static_cast<IEditController*>(new Controller); }
  tresult PLUGIN_API initialize(FUnknown* c) override {
    if(EditController::initialize(c) != kResultOk) return kResultFalse;
    auto grid = [&](const TChar* name, ParamID id, double def) {
      auto* param = new StringListParameter(name,id);
      for(auto text : {STR16("1/4"),STR16("1/8"),STR16("1/16"),STR16("1/32"),STR16("1/8 T"),STR16("1/16 T"),STR16("1/8 D"),STR16("1/16 D"),STR16("1/32 D")}) param->appendString(text);
      param->getInfo().defaultNormalizedValue=def; param->setNormalized(def); parameters.addParameter(param);
    };
    auto effectGrid = [&](const TChar* name,ParamID id,double def,bool global=false) {
      auto* param=new StringListParameter(name,id);
      if(global) param->appendString(STR16("Global"));
      for(auto text:{STR16("1/1"),STR16("1/2"),STR16("1/4"),STR16("1/8"),STR16("1/16"),STR16("1/32"),STR16("1/64"),STR16("1/128"),STR16("1/4 T"),STR16("1/8 T"),STR16("1/16 T"),STR16("1/32 T"),STR16("1/4 D"),STR16("1/8 D"),STR16("1/16 D"),STR16("1/32 D")}) param->appendString(text);
      param->getInfo().defaultNormalizedValue=def; param->setNormalized(def); parameters.addParameter(param);
    };
    auto toggle = [&](const TChar* name,ParamID id,double def,int32 extra=0) {
      auto* param=new StringListParameter(name,id,nullptr,ParameterInfo::kCanAutomate|ParameterInfo::kIsList|extra);
      param->appendString(STR16("Off")); param->appendString(STR16("On"));
      param->getInfo().defaultNormalizedValue=def; param->setNormalized(def); parameters.addParameter(param);
    };
    auto range = [&](const TChar* name,ParamID id,const TChar* unit,double low,double high,double def,int32 count=0) {
      parameters.addParameter(new RangeParameter(name,id,unit,low,high,def,count));
    };
    grid(STR16("Gate Grid"),kDivision,.25);
    range(STR16("Grain Size"),kSize,STR16("ms"),15,250,73.75,0);
    range(STR16("Density"),kDensity,STR16("per step"),1,32,4,0);
    range(STR16("Pitch"),kPitch,STR16("st"),-48,48,0,96);
    range(STR16("Lookback"),kPosition,STR16("ms"),15,2000,114.25,0);
    range(STR16("Chaos"),kChaos,STR16("%"),0,100,0,0);
    range(STR16("Dry Wet"),kMix,STR16("%"),0,100,100,0);
    range(STR16("Gate Attack"),kAttack,STR16("ms"),0.2,50.2,5.2,0);
    range(STR16("Gate Release"),kRelease,STR16("ms"),0.2,150.2,22.7,0);
    range(STR16("Feedback"),kFeedback,STR16("%"),0,85,0,0);
    for(int i=0;i<16;++i) {
      String128 name{}; char ascii[32]; std::snprintf(ascii,sizeof(ascii),"Step %02d",i+1); UString(name,128).fromAscii(ascii);
      toggle(name,kStep0+i,1.);
    }
    range(STR16("Granular Mix"),kGrainMix,STR16("%"),0,100,100,0);
    range(STR16("Glitch Mix"),kGlitchMix,STR16("%"),0,100,0,0);
    effectGrid(STR16("Glitch Slice"),kGlitchDivision,5./15.);
    range(STR16("Glitch Probability"),kGlitchChance,STR16("%"),0,100,100,0);
    toggle(STR16("Glitch Reverse"),kGlitchReverse,0.);
    toggle(STR16("Repeat Hold"),kRepeatOn,0.);
    range(STR16("Repeat Mix"),kRepeatMix,STR16("%"),0,100,100,0);
    effectGrid(STR16("Repeat Length"),kRepeatDivision,4./15.);
    toggle(STR16("Bypass"),kBypass,0.,ParameterInfo::kIsBypass);
    for(int i=0;i<qg::lfoCount;++i) {
      String128 name{};
      auto title=[&](const char* label) -> const TChar* {
        char text[100]; std::snprintf(text,sizeof(text),"Mod %d %s",i+1,label);
        UString(name,128).fromAscii(text); return name;
      };
      toggle(title("Enable"),lfoID(i,lEnabled),0.);
      auto* waves=new StringListParameter(title("Wave"),lfoID(i,lWave));
      for(int w=0;w<qg::waveCount;++w) {
        char ascii[100]; String128 label{};
        std::snprintf(ascii,sizeof(ascii),"%03d %s %02d",w+1,waveFamilies[w/16],w%16+1);
        UString(label,128).fromAscii(ascii); waves->appendString(label);
      }
      waves->appendString(STR16("S&H - Sample & Hold"));
      waves->appendString(STR16("S&G - Sample & Glide"));
      parameters.addParameter(waves);
      toggle(title("Tempo Sync"),lfoID(i,lSync),1.);
      range(title("Free Rate"),lfoID(i,lHz),STR16("Hz"),.01,40.,1.);
      auto* rates=new StringListParameter(title("Sync Rate"),lfoID(i,lGrid));
      for(auto text:{STR16("8 x 4 beats"),STR16("4 x 4 beats"),STR16("2 x 4 beats"),STR16("4 beats"),STR16("1/2"),STR16("1/4"),STR16("1/8"),STR16("1/16"),STR16("1/32"),STR16("1/64"),STR16("1/4 T"),STR16("1/8 T"),STR16("1/16 T"),STR16("1/32 T"),STR16("1/4 D"),STR16("1/8 D"),STR16("1/16 D"),STR16("1/32 D")}) rates->appendString(text);
      rates->appendString(STR16("16 x 4 beats")); rates->appendString(STR16("32 x 4 beats")); rates->appendString(STR16("64 x 4 beats"));
      rates->getInfo().defaultNormalizedValue=3./20.; rates->setNormalized(3./20.); parameters.addParameter(rates);
      range(title("Depth"),lfoID(i,lDepth),STR16("%"),0.,100.,100.);
      range(title("Phase"),lfoID(i,lPhase),STR16("deg"),0.,360.,0.);
      toggle(title("Beat Retrigger"),lfoID(i,lReset),0.);
      range(title("Glide"),lfoID(i,lGlide),STR16("% cycle"),1.,100.,100.);
      for(int t=0;t<6;++t) {
        char route[64]; std::snprintf(route,sizeof(route),"To %s",targetNames[t]);
        range(title(route),lfoID(i,lRoute0+t),STR16("%"),-100.,100.,0.);
      }
    }
    toggle(STR16("Glitch Sequence"),kGlitchSeq,1.);
    toggle(STR16("Repeat Sequence"),kRepeatSeq,0.);
    auto stepTitle=[](const char* prefix,int index,String128& name) {
      char text[100]; std::snprintf(text,sizeof(text),"%s %02d",prefix,index+1); UString(name,128).fromAscii(text);
    };
    for(int i=0;i<16;++i) { String128 n{}; stepTitle("Glitch Step",i,n); toggle(n,kGlitchStep0+i,1.); }
    for(int i=0;i<16;++i) { String128 n{}; stepTitle("Repeat Step",i,n); toggle(n,kRepeatStep0+i,1.); }
    // Beat Repeater steps no longer offer "Global"; old presets remap 0->1 (1/1) in syncState and loadState.
    for(int i=0;i<16;++i) { String128 n{}; stepTitle("Repeat Division",i,n); effectGrid(n,kRepeatRate0+i,1./15.); }
    for(int i=0;i<16;++i) { String128 n{}; stepTitle("Repeat Pitch",i,n); range(n,kRepeatPitch0+i,STR16("st"),-48,48,0,96); }
    toggle(STR16("Free Stretch"),kStretchOn,0.);
    range(STR16("Stretch Speed"),kStretchSpeed,STR16("%"),25,400,100);
    for(int i=0;i<4;++i) { String128 n{}; stepTitle("Mod Random Points",i,n); range(n,kRandomSteps0+i,STR16("points"),1,64,16,63); }
    toggle(STR16("Density Flow"),kDensityFlow,1.);
    range(STR16("Final Transpose"),kTranspose,STR16("st"),-48,48,0,96);
    range(STR16("XY X"),kXYX,STR16("%"),0,100,50);
    range(STR16("XY Y"),kXYY,STR16("%"),0,100,50);
    toggle(STR16("XY Enable"),kXYEnable,0.);
    for(int axis=0;axis<2;++axis) {
      auto* route=new StringListParameter(axis?STR16("Y Destination"):STR16("X Destination"),axis?kYTarget:kXTarget);
      route->appendString(STR16("None"));
      for(int id=0;id<int(kXYX);++id) { ParameterInfo info{}; getParameterInfo(id,info); route->appendString(info.title); }
      const double def=defaults()[axis?kYTarget:kXTarget]; route->getInfo().defaultNormalizedValue=def; route->setNormalized(def); parameters.addParameter(route);
    }
    range(STR16("X Amount"),kXAmount,STR16("%"),-100,100,50);
    range(STR16("Y Amount"),kYAmount,STR16("%"),-100,100,50);
    auto* order=new StringListParameter(STR16("Module Order"),kModuleOrder);
    for(auto name:{STR16("Grain → Glitch → Repeat"),STR16("Grain → Repeat → Glitch"),
                   STR16("Glitch → Grain → Repeat"),STR16("Glitch → Repeat → Grain"),
                   STR16("Repeat → Grain → Glitch"),STR16("Repeat → Glitch → Grain"),
                   STR16("Legacy Parallel")}) order->appendString(name);
    parameters.addParameter(order);
    for(int i=0;i<4;++i) for(int t=6;t<8;++t) {
      char ascii[64]; String128 name{};
      std::snprintf(ascii,sizeof(ascii),"Mod %d To %s",i+1,targetNames[t]);
      UString(name,128).fromAscii(ascii);
      range(name,routeID(i,t),STR16("%"),-100.,100.,0.);
    }
    range(STR16("Glitch Move"),kGlitchMove,STR16("%"),0.,100.,50.);
    range(STR16("Glitch Variation"),kGlitchVariation,STR16("%"),0.,100.,30.);
    auto interval=[&](const TChar* name,ParamID id,double def){auto* p=new StringListParameter(name,id);for(auto text:{STR16("1/16"),STR16("1/8"),STR16("1/4"),STR16("1/2"),STR16("1 Bar"),STR16("2 Bars"),STR16("4 Bars"),STR16("8 Bars")})p->appendString(text);p->getInfo().defaultNormalizedValue=def;p->setNormalized(def);parameters.addParameter(p);};
    interval(STR16("Glitch Refresh"),kGlitchRefresh,2./7.);
    toggle(STR16("Repeat Auto"),kRepeatAuto,0.);
    interval(STR16("Repeat Interval"),kRepeatInterval,4./7.);
    range(STR16("Repeat Duration"),kRepeatDuration,STR16("16ths"),1,32,4,31);
    range(STR16("Repeat Chance"),kRepeatChance,STR16("%"),0,100,100);
    for(int i=0;i<4;++i){auto* p=new RangeParameter(i==0?STR16("Step Monitor"):(i==1?STR16("Glitch Monitor"):(i==2?STR16("Repeat Monitor"):STR16("Output Monitor"))),kUiStep+i,nullptr,0,1,0);p->getInfo().flags=ParameterInfo::kIsReadOnly|ParameterInfo::kIsHidden;parameters.addParameter(p);}
    toggle(STR16("Master Filter"),kMasterFilter,0.);
    auto* filterType=new StringListParameter(STR16("Master Filter Type"),kFilterType);filterType->appendString(STR16("LP"));filterType->appendString(STR16("HP"));filterType->appendString(STR16("BP"));parameters.addParameter(filterType);
    parameters.addParameter(new LogCutoffParameter);
    range(STR16("Master Resonance"),kFilterResonance,STR16("%"),0,100,0);
    toggle(STR16("Master Limiter"),kMasterLimiter,1.);
    toggle(STR16("Normalize Wet"),kNormalize,0.);
    auto* slope=new StringListParameter(STR16("Filter Slope"),kFilterSlope);slope->appendString(STR16("12 dB"));slope->appendString(STR16("24 dB"));parameters.addParameter(slope);
    range(STR16("Filter Drive"),kFilterDrive,STR16("dB"),0,24,0);
    toggle(STR16("Grain Enabled"),kGrainEnabled,1.);toggle(STR16("Glitch Enabled"),kGlitchEnabled,1.);toggle(STR16("Repeater Enabled"),kRepeatEnabled,1.);
    range(STR16("Grain Pan"),kGrainPan,STR16("L/R"),-100,100,0);
    auto* pan=new StringListParameter(STR16("Grain Pan Mode"),kPanMode);for(auto name:{STR16("Manual"),STR16("Alternate L/R"),STR16("Random L/R")})pan->appendString(name);parameters.addParameter(pan);
    toggle(STR16("Reverb On"),kReverbOn,0.);
    auto* reverb=new StringListParameter(STR16("Reverb Type"),kReverbType);for(auto name:{STR16("Room"),STR16("Chamber"),STR16("Hall"),STR16("Ambient"),STR16("Space")})reverb->appendString(name);parameters.addParameter(reverb);
    auto* revGrid=new StringListParameter(STR16("Reverb Grid"),kReverbGrid);for(auto name:{STR16("1/4"),STR16("1/8"),STR16("1/16"),STR16("1/32")})revGrid->appendString(name);revGrid->getInfo().defaultNormalizedValue=2./3.;revGrid->setNormalized(2./3.);parameters.addParameter(revGrid);
    range(STR16("Reverb Amount"),kReverbMix,STR16("%"),0,100,25);
    for(int i=0;i<16;++i){String128 name{};char ascii[32];std::snprintf(ascii,sizeof(ascii),"Reverb Step %02d",i+1);UString(name,128).fromAscii(ascii);toggle(name,kReverbStep0+i,i%4==0?1.:0.);}
    auto* revMeter=new RangeParameter(STR16("Reverb Step Monitor"),kUiReverb,nullptr,0,1,0);revMeter->getInfo().flags=ParameterInfo::kIsReadOnly|ParameterInfo::kIsHidden;parameters.addParameter(revMeter);
    range(STR16("Reverb Length"),kReverbLength,STR16("s"),.2,20.,2.8);
    for(int id=kUiWave0;id<=kUiWaveSeconds;++id){char ascii[40];std::snprintf(ascii,sizeof(ascii),"Grain Display %d",id-kUiWave0);String128 name{};UString(name,128).fromAscii(ascii);auto* monitor=new RangeParameter(name,id,nullptr,0,1,0);monitor->getInfo().flags=ParameterInfo::kIsReadOnly|ParameterInfo::kIsHidden;parameters.addParameter(monitor);}
    for(int l=0;l<4;++l)for(int slot=0;slot<6;++slot){
      char ascii[64];String128 name{};std::snprintf(ascii,sizeof(ascii),"Mod %d Destination %d",l+1,slot+1);UString(name,128).fromAscii(ascii);auto* destination=new StringListParameter(name,slotTarget(l,slot));destination->appendString(STR16("None"));for(auto title:modNames){String128 text{};UString(text,128).fromAscii(title);destination->appendString(text);}parameters.addParameter(destination);
      std::snprintf(ascii,sizeof(ascii),"Mod %d Amount %d",l+1,slot+1);UString(name,128).fromAscii(ascii);range(name,slotAmount(l,slot),STR16("%"),-100,100,0.);
    }
    for(int l=0;l<4;++l){char ascii[32];String128 name{};std::snprintf(ascii,sizeof(ascii),"Mod %d Speed",l+1);UString(name,128).fromAscii(ascii);auto* speed=new StringListParameter(name,kLfoSpeed0+l);for(auto title:{STR16("0.25x"),STR16("0.5x"),STR16("1x"),STR16("2x")})speed->appendString(title);speed->getInfo().defaultNormalizedValue=2./3.;speed->setNormalized(2./3.);parameters.addParameter(speed);}
    toggle(STR16("Reverb Kill Dry"),kReverbKill,0.);
    for(int l=0;l<12;++l){auto* phase=new RangeParameter(STR16("Mod Display"),kUiLfoPhase0+l,nullptr,0,1,l<4?0.:2147483648./4294967295.);phase->getInfo().flags=ParameterInfo::kIsReadOnly|ParameterInfo::kIsHidden;parameters.addParameter(phase);}
    for(int l=0;l<4;++l){char ascii[48];String128 name{};std::snprintf(ascii,sizeof(ascii),"Mod %d Wave RND",l+1);UString(name,128).fromAscii(ascii);auto* rnd=new StringListParameter(name,kModWaveRnd0+l);for(auto label:{STR16("Off"),STR16("1/1"),STR16("1/2"),STR16("1/4"),STR16("1/8")})rnd->appendString(label);parameters.addParameter(rnd);}
    for(int l=0;l<4;++l){auto* monitor=new RangeParameter(STR16("Mod Active Wave"),kUiModWave0+l,nullptr,0,1,0);monitor->getInfo().flags=ParameterInfo::kIsReadOnly|ParameterInfo::kIsHidden;parameters.addParameter(monitor);}
    auto* source=new StringListParameter(STR16("Reverb Trigger Source"),kReverbSource);source->appendString(STR16("Step Sequencer"));source->appendString(STR16("Random Impulse"));parameters.addParameter(source);
    auto* randomRate=new StringListParameter(STR16("Reverb Random Rate"),kReverbRandomRate);for(auto label:{STR16("1/4"),STR16("1/8"),STR16("1/16")})randomRate->appendString(label);randomRate->getInfo().defaultNormalizedValue=1.;randomRate->setNormalized(1.);parameters.addParameter(randomRate);
    auto* sendGate=new RangeParameter(STR16("Reverb Send Monitor"),kUiReverbGate,nullptr,0,1,0);sendGate->getInfo().flags=ParameterInfo::kIsReadOnly|ParameterInfo::kIsHidden;parameters.addParameter(sendGate);
    auto* ceiling=new StringListParameter(STR16("Limiter Ceiling"),kLimiterCeiling);for(auto label:{STR16("0 dBFS"),STR16("-6 dBFS"),STR16("-10 dBFS")})ceiling->appendString(label);parameters.addParameter(ceiling);
    toggle(STR16("Gater On"),kGaterEnabled,0.);
    auto* gateGrid=new StringListParameter(STR16("Gater Rate"),kGaterGrid);for(auto label:{STR16("1/4"),STR16("1/8"),STR16("1/16"),STR16("1/32")})gateGrid->appendString(label);gateGrid->getInfo().defaultNormalizedValue=2./3.;gateGrid->setNormalized(2./3.);parameters.addParameter(gateGrid);
    toggle(STR16("Gater Length RND"),kGaterLengthRnd,0.);toggle(STR16("Gater Step RND"),kGaterStepRnd,0.);range(STR16("Gater Chance"),kGaterChance,STR16("%"),0,100,50);
    for(int i=0;i<16;++i){String128 name{};stepTitle("Gater Step",i,name);auto* state=new StringListParameter(name,kGaterState0+i);for(auto label:{STR16("Off"),STR16("Wet")})state->appendString(label);state->getInfo().defaultNormalizedValue=1.;state->setNormalized(1.);parameters.addParameter(state);}
    for(int i=0;i<16;++i){String128 name{};stepTitle("Gater Length",i,name);range(name,kGaterLength0+i,STR16("%"),5,100,75);}
    for(int i=kUiGaterStep;i<kInputDeclick;++i){auto* monitor=new RangeParameter(STR16("Gater Display"),i,nullptr,0,1,0);monitor->getInfo().flags=ParameterInfo::kIsReadOnly|ParameterInfo::kIsHidden;parameters.addParameter(monitor);}
    toggle(STR16("Input De-click"),kInputDeclick,1.);range(STR16("De-click Sensitivity"),kDeclickSensitivity,STR16("%"),0,100,50);
    auto* model=new StringListParameter(STR16("Filter Model"),kFilterModel);for(auto label:qg::filterModels){String128 name{};UString(name,128).fromAscii(label);model->appendString(name);}parameters.addParameter(model);
    toggle(STR16("Reslice On"),kResliceEnabled,0.);
    auto* length=new StringListParameter(STR16("Reslice Window"),kResliceLength);for(auto label:{STR16("4/1"),STR16("2/1"),STR16("1/1"),STR16("1/2")})length->appendString(label);length->getInfo().defaultNormalizedValue=2./3.;length->setNormalized(2./3.);parameters.addParameter(length);
    range(STR16("Reslice Mix"),kResliceMix,STR16("%"),0,100,100);
    // Reslice steps default to ON; the module itself stays OFF (kResliceEnabled default 0).
    for(int i=0;i<16;++i){String128 name{};stepTitle("Reslice Step",i,name);toggle(name,kResliceStep0+i,1.);}
    for(int i=0;i<16;++i){String128 name{};stepTitle("Reslice Source",i,name);range(name,kResliceIndex0+i,STR16("slice"),1,16,i+1,15);}
    toggle(STR16("Gater Latch"),kGaterLatch,0.);
    for(int i=0;i<16;++i){String128 name{};stepTitle("Gater Release",i,name);toggle(name,kGaterRelease0+i,0.);}
    for(int i=kUiResliceStep;i<kReverbModel;++i){auto* monitor=new RangeParameter(STR16("Reslice Display"),i,nullptr,0,1,0);monitor->getInfo().flags=ParameterInfo::kIsReadOnly|ParameterInfo::kIsHidden;parameters.addParameter(monitor);}
    auto* reverbModel=new StringListParameter(STR16("Reverb Model"),kReverbModel);for(auto label:{STR16("Classic (legacy)"),STR16("Plate"),STR16("Cosmic Space"),STR16("Dark Space"),STR16("Bloom Space")})reverbModel->appendString(label);parameters.addParameter(reverbModel);
    range(STR16("Gater Minimum Length"),kGaterMinLength,STR16("%"),5,95,5);
    // Gater "Tie" removed from the UI; parameter kept (hidden) for old-state compatibility, forced off.
    {auto* tie=new StringListParameter(STR16("Gater Tie"),kGaterTie,nullptr,ParameterInfo::kIsHidden);tie->appendString(STR16("Off"));tie->appendString(STR16("On"));tie->getInfo().defaultNormalizedValue=0.;tie->setNormalized(0.);parameters.addParameter(tie);}
    for(int i=0;i<16;++i){String128 name{};stepTitle("Gater Sustain",i,name);range(name,kGaterSustain0+i,STR16("ms"),0,500,50);}

    auto* glitchRate=new StringListParameter(STR16("Glitch Random Interval"),kGlitchTriggerRate);
    for(auto label:{STR16("1/4"),STR16("1/2"),STR16("1/1"),STR16("2/1")})glitchRate->appendString(label);
    parameters.addParameter(glitchRate);
    for(int id:{int(kGlitchSeq),int(kGlitchRefresh)})getParameterObject(id)->getInfo().flags=ParameterInfo::kIsHidden;
    for(int i=0;i<16;++i)getParameterObject(kGlitchStep0+i)->getInfo().flags=ParameterInfo::kIsHidden;
    toggle(STR16("Reslice Step Random"),kResliceRndOn,0.);
    auto* rndRate=new StringListParameter(STR16("Reslice Random Rate"),kResliceRndRate);for(auto label:{STR16("1/2"),STR16("1/1"),STR16("2/1")})rndRate->appendString(label);parameters.addParameter(rndRate);
    toggle(STR16("Filter Sequencer"),kFilterSeqOn,0.);
    auto list=[&](const TChar* name,ParamID id,std::initializer_list<const TChar*> labels,double def=0.){auto* param=new StringListParameter(name,id);for(auto title:labels)param->appendString(title);param->getInfo().defaultNormalizedValue=def;param->setNormalized(def);parameters.addParameter(param);};
    list(STR16("Filter Sequencer Mode"),kFilterSeqMode,{STR16("Arp - Original"),STR16("Sample & Glide")});
    auto* arp=new StringListParameter(STR16("Filter Pattern"),kFilterSeqPattern);const char* families[]={"Rise","Fall","Triangle","Bounce","Stairs","Sine","Scatter","Pulse"};
    for(int i=0;i<64;++i){char label[64];String128 title{};std::snprintf(label,sizeof(label),"%02d %s %d",i+1,families[i/8],i%8+1);UString(title,128).fromAscii(label);arp->appendString(title);}parameters.addParameter(arp);
    list(STR16("Filter Sequence Rate"),kFilterSeqRate,{STR16("1/4"),STR16("1/8"),STR16("1/16"),STR16("1/32"),STR16("1/2"),STR16("1/1")},2./5.);
    range(STR16("Filter Sequence Depth"),kFilterSeqDepth,STR16("%"),0,100,50);
    range(STR16("Filter Sequence Glide"),kFilterSeqGlide,STR16("%"),0,100,20);
    list(STR16("Comb Root"),kCombRoot,{STR16("C"),STR16("C#"),STR16("D"),STR16("D#"),STR16("E"),STR16("F"),STR16("F#"),STR16("G"),STR16("G#"),STR16("A"),STR16("A#"),STR16("B")});
    range(STR16("Comb Octave"),kCombOctave,nullptr,0,6,2,6);
    list(STR16("Comb Notes"),kCombScale,{STR16("Minor chord"),STR16("Natural minor")});
    for(int id=kUiResliceSource0;id<=kUiFilterSeqStep;++id){auto* monitor=new RangeParameter(STR16("Sequencer Display"),id,nullptr,0,1,0);monitor->getInfo().flags=ParameterInfo::kIsReadOnly|ParameterInfo::kIsHidden;parameters.addParameter(monitor);}
    for(int l=0;l<4;++l)for(int t=0;t<8;++t)getParameterObject(routeID(l,t))->getInfo().flags=ParameterInfo::kIsHidden;
    getParameterObject(kFeedback)->getInfo().flags=ParameterInfo::kIsHidden;

    for(int id:{int(kDivision),int(kAttack),int(kRelease)})getParameterObject(id)->getInfo().flags=ParameterInfo::kIsHidden;
    for(int i=0;i<16;++i)getParameterObject(kStep0+i)->getInfo().flags=ParameterInfo::kIsHidden;
    return kResultOk;
  }
  IPlugView* PLUGIN_API createView(FIDString name) override {
#if defined(__APPLE__) || defined(_WIN32)
    if(name && std::strcmp(name,ViewType::kEditor)==0) return aztec::createEditor(this);
#endif
    return nullptr;
  }
  tresult PLUGIN_API setComponentState(IBStream* stream) override {
    auto p=defaults(); if(!loadState(stream,p)) return kResultFalse;
    for(int i=0;i<kCount;++i) setParamNormalized(i,p[i]);
    return kResultOk;
  }
};
}

BEGIN_FACTORY_DEF("Daniel Melegari", "", "")
DEF_CLASS2(INLINE_UID_FROM_FUID(processorID), PClassInfo::kManyInstances, kVstAudioEffectClass,
           "GrainsDosage", Vst::kDistributable, "Fx|Granular", "0.13.0", kVstVersionString, Processor::create)
DEF_CLASS2(INLINE_UID_FROM_FUID(controllerID), PClassInfo::kManyInstances, kVstComponentControllerClass,
           "GrainsDosage Controller", 0, "", "0.13.0", kVstVersionString, Controller::create)
END_FACTORY
