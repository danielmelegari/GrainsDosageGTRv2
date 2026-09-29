#pragma once
#include "modulation.h"
#include "stretch.h"
#include "transpose.h"
#include "rhythmic_loop.h"
#include "level_match.h"
#include "step_reverb.h"
#include "master_fx.h"
#include "gater.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace qg {
constexpr int steps = 16;
constexpr int maxVoices = 128;
constexpr double pi = 3.14159265358979323846;

struct Settings {
  GaterSettings gater;
  double tempo = 120., sampleRate = 44100.;
  double division = .25; // beats per step: 1/16 note
  double size = .075, density = 1., pitch = 0., position = .10;
  double chaos = 0., mix = 1., attack = .005, release = .02;
  double feedback = 0.;
  bool normalize=false;std::array<bool,3> moduleOn{{true,true,true}};
  double grainPan=0.;int panMode=0;
  bool filterOn=false;int filterType=0,filterSlope=0;double filterCutoff=.5663233347786729,filterResonance=0.,filterDrive=0.;
  bool reverbRandom=false;double reverbRandomGrid=.25;bool reverbKill=false;bool reverbOn=false;int reverbType=0;double reverbLength=0.;double reverbGrid=.25,reverbMix=.25;uint16_t reverbPattern=0x1111;
  double grainMix = 1., glitchMix = 0., repeatMix = 0.;
  double glitchBeats = .125, repeatBeats = .25, glitchChance = 1.;
  bool glitchReverse = false, repeatOn = false;
  bool glitchSequence=false, repeatSequence=false, stretchOn=false;
  uint16_t glitchPattern=0xFFFF, repeatPattern=0xFFFF;
  std::array<double,16> repeatRates{}, repeatPitches{};
  double stretchSpeed=1., transpose=0.;
  double glitchMove=0.,glitchVariation=0.,glitchRefresh=0.;
  bool repeatAuto=false;double repeatInterval=4.,repeatDuration=1.,repeatChance=1.;
  bool densityFlow=false, bypass=false;
  int order=6;
  uint16_t pattern = 0xAAAA;
  std::array<LfoSettings,lfoCount> lfos{};
};

class Engine {
  struct Voice { double read = 0., increment = 1.; int age = 0, length = 0; double pan = 0.; bool active = false; };
  std::array<Voice, maxVoices> voices_{};
  std::array<std::vector<float>, 2> buffer_;
  std::array<std::vector<float>, 2> repeatBuffer_;
  std::array<std::vector<float>, 2> glitchBuffer_;
  RhythmicLoop serialRepeat_,serialGlitch_;
  bool glitchBlock_=false,glitchChosen_=false;
  double glitchOffset_=0.,lastGlitchChance_=-1.,glitchSlice_=0.,nextGlitch_=-1e30;
  double repeatUntil_=-1e30;int64_t repeatCycle_=INT64_MIN;
  bool previousHold_=false;
  Settings settings_{};
  Modulation modulation_;
  Gater gater_;MasterFx filter_;LevelMatch match_;StepReverb reverb_;double moduleBlend_[3]={1.,1.,1.};bool panRight_=false;
  FreeStretch stretch_;
  Transpose transpose_;
  double grainClock_=0.;
  ModValues modBase_{};
  double grainSize_=.075, grainDensity_=1., grainPitch_=0., grainPosition_=.1, grainChaos_=0., grainLevel_=1.;
  double levelSmoothing_=0.;
  bool mixModulated_=false;
  uint32_t rng_ = 0x1234ABCD;
  int64_t write_ = 0, lastStep_ = INT64_MIN;
  double previous_[2] = {0., 0.}, gate_ = 0.;
  int64_t repeatWrite_ = 0, glitchWrite_ = 0;
  int lastOrder_=0, orderFade_=0; float lastOutput_[2]={};
  double glitchStart_ = 0., glitchLength_ = 16., repeatStart_ = 0., repeatLength_ = 16.;
  double glitchBeat_ = 0., repeatBeat_ = 0., heldRepeatBeats_ = .25;
  bool glitchActive_ = false, repeating_ = false;
  int64_t lastRhythm_=INT64_MIN;
  double repeatPhase_=0., repeatLastBeat_=0., repeatPitch_=0.;
  double repeatGate_=0., glitchGate_=0.;
  double lastRepeat_[2]={}, transitionFrom_[2]={}; int transition_=0;
  double gateAttack_ = 0., gateRelease_ = 0.;
  static double loopRead(const std::vector<float>& b, double start, double length, double phase, bool reverse) {
    phase -= std::floor(phase);
    double position = phase * length;
    if(reverse) position = length - 1. - position;
    position = std::fmod(position + length, length);
    const double base = std::floor(position), fraction = position-base;
    auto sample = [&](double p) {
      auto idx = int64_t(std::floor(start+p)) % int64_t(b.size());
      if(idx<0) idx += int64_t(b.size());
      return b[size_t(idx)];
    };
    // A short edge envelope suppresses clicks when the slice loops.
    const double fade = std::min(64., length*.25);
    const double envelope = std::min(1., std::min(position, length-position) / fade);
    return (sample(base)*(1.-fraction) + sample(std::fmod(base+1.,length))*fraction)*envelope;
  }
  double random() { rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5; return (rng_ & 0xFFFFFF) / double(0x1000000); }
  double read(int ch, double pos) const {
    const auto& b = buffer_[ch];
    const double n = double(b.size());
    pos = std::fmod(pos, n); if(pos < 0) pos += n;
    const auto a = size_t(pos), c = (a + 1) % b.size();
    return b[a] + (b[c] - b[a]) * (pos - a);
  }
  void trigger() {
    Voice* v = nullptr;
    for(auto& candidate : voices_) if(!candidate.active) { v = &candidate; break; }
    if(!v) v = &*std::max_element(voices_.begin(),voices_.end(),[](const Voice& a,const Voice& b) { return double(a.age)/a.length < double(b.age)/b.length; });
    const double c = grainChaos_;
    const double length = grainSize_ * settings_.sampleRate * (1. + c * (random() - .5));
    v->length = std::max(16, int(length)); v->age = 0;
    const double lookback = (grainPosition_ + c * random() * .2) * settings_.sampleRate;
    
    v->increment = std::pow(2., (grainPitch_ + c * (random() - .5) * 12.) / 12.);
    v->read=double(write_)-std::max({2.,lookback,(v->increment-1.)*v->length+2.});
    v->pan=std::clamp(settings_.grainPan,-1.,1.);
    if(settings_.panMode==1){panRight_=!panRight_;v->pan=panRight_?1.:-1.;}
    if(settings_.panMode==2)v->pan=random()>.5?1.:-1.; v->active = true;
  }
public:
  struct GrainView{std::array<double,128> amplitude{};double start=0.,end=0.,head=0.,seconds=2.5;bool active=false;};
  GrainView grainView()const{
    GrainView view;if(buffer_[0].empty())return view;
    const Voice* voice=nullptr;for(const auto& v:voices_)if(v.active&&(!voice||v.age<voice->age))voice=&v;
    double origin=voice?voice->read-voice->age*voice->increment:double(write_);
    // Follow the audible read position: the head stays centered while source audio scrolls.
    // Keep unavailable future/overwritten samples empty rather than wrapping stale audio.
    double span=std::clamp(std::max(settings_.sampleRate*2.5,voice?2.*voice->length*voice->increment:0.),settings_.sampleRate*2.5,double(buffer_[0].size()-2));
    double center=voice?voice->read:double(write_);
    double left=center-span*.5;view.seconds=span/settings_.sampleRate;
    const double oldest=std::max(0.,double(write_)-double(buffer_[0].size())+1.);
    for(int i=0;i<128;++i){double peak=0.;for(int j=0;j<8;++j){double pos=left+span*(i+(j+.5)/8.)/128.;if(pos>=oldest&&pos<double(write_)-1.)peak=std::max({peak,std::abs(read(0,pos)),std::abs(read(1,pos))});}view.amplitude[i]=std::min(1.,peak);}
    if(voice&&settings_.moduleOn[0]&&grainLevel_>.0001){view.active=true;view.start=std::clamp((origin-left)/span,0.,1.);view.end=std::clamp((origin+voice->length*voice->increment-left)/span,0.,1.);view.head=std::clamp((voice->read-left)/span,0.,1.);}
    return view;
  }
  void prepare(double sampleRate) {
    settings_.sampleRate = std::max(8000., sampleRate);
    gater_.prepare(settings_.sampleRate);filter_.prepare(settings_.sampleRate);match_.prepare(settings_.sampleRate);reverb_.prepare(settings_.sampleRate);panRight_=false;for(int i=0;i<3;++i)moduleBlend_[i]=settings_.moduleOn[i]?1.:0.;
    const size_t capacity = size_t(settings_.sampleRate * 16.);
    for(auto& b : buffer_) b.assign(capacity, 0.f);
    for(auto& b : repeatBuffer_) b.assign(capacity, 0.f);
    for(auto& b : glitchBuffer_) b.assign(capacity,0.f);
    write_ = 0; lastStep_ = INT64_MIN; gate_ = 0.; previous_[0] = previous_[1] = 0.;
    for(auto& v : voices_) v = {};
    repeatWrite_=glitchWrite_=0; lastOrder_=settings_.order; orderFade_=0; lastOutput_[0]=lastOutput_[1]=0.; glitchActive_ = repeating_ = false;
    stretch_.prepare(settings_.sampleRate); transpose_.prepare(settings_.sampleRate); grainClock_=0.; lastRhythm_=INT64_MIN; repeatPhase_=0.; repeatGate_=glitchGate_=0.; transition_=0;
    serialRepeat_.prepare(settings_.sampleRate);serialGlitch_.prepare(settings_.sampleRate);glitchBlock_=false;nextGlitch_=-1e30;repeatCycle_=INT64_MIN;repeatUntil_=-1e30;previousHold_=false;
    modulation_.prepare(); grainLevel_=settings_.grainMix;
    levelSmoothing_=std::exp(-1./(.003*settings_.sampleRate));
  }
  void set(const Settings& s) {
    if(s.order!=settings_.order) { orderFade_=128; lastOrder_=settings_.order; }
    if(s.order!=settings_.order){serialRepeat_.reset();serialGlitch_.reset();glitchBlock_=false;}
    settings_ = s;gater_.set(s.gater);
    reverb_.set(s.reverbOn,s.reverbType,s.reverbGrid,s.reverbPattern,s.reverbMix,s.reverbLength,s.reverbKill,s.reverbRandom,s.reverbRandomGrid);
    modBase_ = {(s.size-.015)/.235,(s.density-1.)/31.,(s.pitch+48.)/96.,(s.position-.015)/1.985,s.chaos,s.grainMix,(s.stretchSpeed-.25)/3.75,(s.transpose+48.)/96.,s.filterCutoff,s.filterResonance,s.filterDrive};
    mixModulated_=false;
    for(const auto& lfo:s.lfos) if(lfo.enabled && lfo.depth>0. && lfo.amount[5]!=0.) mixModulated_=true;
    gateAttack_ = std::exp(-1./(s.sampleRate*std::max(.0001,s.attack)));
    gateRelease_ = std::exp(-1./(s.sampleRate*std::max(.0001,s.release)));
  }
  void resetTransport() {
    // Preserve reverb tails and level matching across host seeks/loops.
    serialRepeat_.reset();serialGlitch_.reset();glitchBlock_=false;nextGlitch_=-1e30;repeatCycle_=INT64_MIN;repeatUntil_=-1e30;previousHold_=false;
    lastStep_ = INT64_MIN; glitchActive_ = repeating_ = false;
    modulation_.reset(); stretch_.reset(); transpose_.reset(); grainClock_=0.; lastRhythm_=INT64_MIN; repeatGate_=glitchGate_=0.; transition_=orderFade_=0;lastOutput_[0]=lastOutput_[1]=0.;
    for(auto& v : voices_) v.active = false;
  }
  int gaterStep()const{return gater_.step();}int gaterState(int i)const{return gater_.state(i);}double gaterLength(int i)const{return gater_.length(i);}
  bool reverbGate()const{return reverb_.gateOpen();}
  int lfoWave(int i)const{return modulation_.wave(i);}
  double lfoPhase(int i)const{return modulation_.phase(i);}
  int64_t lfoCycle(int i)const{return modulation_.cycle(i);}
  int64_t lfoEpoch(int i)const{return modulation_.epoch(i);}
  bool glitchRunning()const{return settings_.moduleOn[1]&&(settings_.order>=6?glitchActive_:serialGlitch_.active());}
  bool repeatRunning()const{return settings_.moduleOn[2]&&(settings_.order>=6?repeating_:serialRepeat_.active());}
  // beat is the host PPQ beat position for this sample, or a running fallback beat.
  void processParallel(float inL, float inR, double beat, float& outL, float& outR) {
    if(buffer_[0].empty()) { outL = inL; outR = inR; return; }
    const int64_t step = int64_t(std::floor(beat / std::max(.015625, settings_.division) + 1e-8));
    const bool activeStep = (settings_.pattern & (1u << ((step % steps + steps) % steps))) != 0;
    const auto values=modulation_.process(settings_.lfos,modBase_,beat,settings_.sampleRate,
                                          step!=lastStep_ && activeStep,step);
    grainSize_=.015+values[0]*.235; grainDensity_=1.+values[1]*31.;
    grainPitch_=-48.+values[2]*96.; grainPosition_=.015+values[3]*1.985; grainChaos_=values[4];
    grainLevel_=mixModulated_ ? values[5]+levelSmoothing_*(grainLevel_-values[5]) : settings_.grainMix;
    const bool gateTrigger=step!=lastStep_;
    const int64_t rhythm=int64_t(std::floor(beat/.25+1e-8));
    const int slot=int((rhythm%16+16)%16);
    const bool rhythmTrigger=rhythm!=lastRhythm_;
    const bool glitchOpen=settings_.glitchSequence ? (settings_.glitchPattern&(1u<<slot))!=0 : activeStep;
    const bool repeatOpen=settings_.repeatSequence ? (settings_.repeatPattern&(1u<<slot))!=0 : activeStep;
    const double samplesPerBeat=settings_.sampleRate*60./std::max(20.,settings_.tempo);
    if(settings_.densityFlow) {
      if(gateTrigger) grainClock_=0.;
      if(activeStep) {
        if(grainClock_<=0.) { trigger(); grainClock_+=1.; }
        grainClock_-=grainDensity_/(samplesPerBeat*settings_.division);
      }
    } else if(gateTrigger && activeStep) {
      int count=std::clamp(int(std::round(grainDensity_)),1,8);
      for(int i=0;i<count;++i) trigger();
    }
    if(settings_.glitchSequence ? rhythmTrigger : gateTrigger) {
      glitchActive_=false;
      if(glitchOpen) {
        glitchLength_=std::clamp(std::round(settings_.glitchBeats*samplesPerBeat),16.,double(buffer_[0].size()-2));
        const double extra=std::min(settings_.chaos*random()*samplesPerBeat,double(buffer_[0].size())-glitchLength_-2.);
        glitchStart_=double(write_)-glitchLength_-extra;
        glitchBeat_=beat;
        glitchActive_=settings_.glitchMix>0. && random()<settings_.glitchChance;
      }
    }
    const double repeatBeats=settings_.repeatSequence && settings_.repeatRates[slot]>0. ? settings_.repeatRates[slot] : settings_.repeatBeats;
    const double pitch=settings_.repeatSequence ? settings_.repeatPitches[slot] : 0.;
    if(settings_.repeatOn && !repeating_ && repeatOpen && repeatWrite_>=std::round(repeatBeats*samplesPerBeat)) {
      repeating_=true; heldRepeatBeats_=repeatBeats; repeatPhase_=0.; repeatLastBeat_=beat; repeatPitch_=pitch;
      repeatLength_=std::clamp(std::round(repeatBeats*samplesPerBeat),16.,double(repeatBuffer_[0].size()-2));
      repeatStart_=double(repeatWrite_)-repeatLength_;
    }
    if(repeating_) {
      repeatPhase_+=std::max(0.,beat-repeatLastBeat_)/heldRepeatBeats_*std::pow(2.,repeatPitch_/12.);
      repeatPhase_-=std::floor(repeatPhase_); repeatLastBeat_=beat;
      const double newLength=std::clamp(std::round(repeatBeats*samplesPerBeat),16.,double(repeatBuffer_[0].size()-2));
      if(newLength!=repeatLength_ || pitch!=repeatPitch_) {
        transitionFrom_[0]=lastRepeat_[0]; transitionFrom_[1]=lastRepeat_[1]; transition_=128;
        repeatLength_=newLength; repeatStart_=double(repeatWrite_)-repeatLength_;
      }
      heldRepeatBeats_=repeatBeats; repeatPitch_=pitch;
    }
    if(!settings_.repeatOn) repeating_=false;
    lastStep_=step; lastRhythm_=rhythm;
    const auto smoothGate=[&](double& g,bool enabled) {
      g=(enabled?1.:0.)+(enabled?gateAttack_:gateRelease_)*(g-(enabled?1.:0.));
    };
    smoothGate(repeatGate_,repeatOpen); smoothGate(glitchGate_,glitchOpen);
    const bool open = (settings_.pattern & (1u << ((step % steps + steps) % steps))) != 0;
    const double coefficient = open ? gateAttack_ : gateRelease_;
    gate_ = (open ? 1. : 0.) + coefficient * (gate_ - (open ? 1. : 0.));
    const size_t index = size_t(write_ % int64_t(buffer_[0].size()));
    buffer_[0][index] = float(std::clamp(double(inL) + previous_[0] * settings_.feedback, -4., 4.));
    buffer_[1][index] = float(std::clamp(double(inR) + previous_[1] * settings_.feedback, -4., 4.));
    if(!repeating_) {
      const auto ri = size_t(repeatWrite_ % int64_t(repeatBuffer_[0].size()));
      repeatBuffer_[0][ri] = inL; repeatBuffer_[1][ri] = inR; ++repeatWrite_;
    }
    double wetL = 0., wetR = 0.; int playing = 0; double weight=0.;
    for(auto& v : voices_) {
      if(!v.active) continue;
      const double phase = double(v.age) / v.length;
      const double window = .5 - .5 * std::cos(2. * pi * phase);
      double vl=read(0,v.read),vr=read(1,v.read);
      wetL+=(vl*(1.-std::max(0.,v.pan))+vr*std::max(0.,-v.pan))*window;
      wetR+=(vr*(1.+std::min(0.,v.pan))+vl*std::max(0.,v.pan))*window;
      weight+=window*window; ++playing; v.read += v.increment;
      if(++v.age >= v.length) v.active = false;
    }
    const double scale = settings_.densityFlow ? 1./std::max({1.,std::sqrt(weight),std::abs(wetL),std::abs(wetR)}) : (playing ? 1. / std::sqrt(double(playing)) : 0.);
    double sumL = settings_.moduleOn[0]?wetL*scale*grainLevel_*gate_:0., sumR=settings_.moduleOn[0]?wetR*scale*grainLevel_*gate_:0.;
    if(glitchActive_ && settings_.moduleOn[1]) {
      const double phase = (beat-glitchBeat_)/settings_.glitchBeats;
      sumL += settings_.glitchMix*glitchGate_*loopRead(buffer_[0],glitchStart_,glitchLength_,phase,settings_.glitchReverse);
      sumR += settings_.glitchMix*glitchGate_*loopRead(buffer_[1],glitchStart_,glitchLength_,phase,settings_.glitchReverse);
    }
    if(repeating_ && settings_.moduleOn[2]) {
      double repeated[2];
      for(int ch=0;ch<2;++ch) {
        repeated[ch]=loopRead(repeatBuffer_[ch],repeatStart_,repeatLength_,repeatPhase_,false);
        if(transition_>0) { const double t=1.-transition_/128.; repeated[ch]=transitionFrom_[ch]*(1.-t)+repeated[ch]*t; }
        lastRepeat_[ch]=repeated[ch];
      }
      if(transition_>0) --transition_;
      sumL+=settings_.repeatMix*repeatGate_*repeated[0]; sumR+=settings_.repeatMix*repeatGate_*repeated[1];
    }
    const double gain = 1./std::max(1., (settings_.moduleOn[0]?grainLevel_:0.) + (glitchActive_&&settings_.moduleOn[1] ? settings_.glitchMix : 0.) + (repeating_&&settings_.moduleOn[2] ? settings_.repeatMix : 0.));
    previous_[0] = sumL * gain; previous_[1] = sumR * gain;
    if(!settings_.moduleOn[0]&&!settings_.moduleOn[1]&&!settings_.moduleOn[2]){previous_[0]=inL;previous_[1]=inR;}
    float wl=float(previous_[0]),wr=float(previous_[1]);stretch_.process(wl,wr,settings_.stretchOn,.25+values[6]*3.75);transpose_.process(wl,wr,-48.+values[7]*96.);gater_.process(wl,wr,beat,settings_.tempo);filter_.set(settings_.filterOn,settings_.filterType,20.*std::pow(1000.,values[8]),values[9],false,settings_.filterSlope,values[10]*24.);filter_.process(wl,wr);reverb_.process(wl,wr,beat);previous_[0]=wl;previous_[1]=wr;
    match_.process(inL,inR,previous_[0],previous_[1],settings_.normalize);
    double finalMix=settings_.mix+(1.-settings_.mix)*reverb_.killAmount();
    outL = float(inL*(1.-finalMix)+(previous_[0]+inL*gater_.dryGain()*(1.-reverb_.killAmount()))*finalMix);outR=float(inR*(1.-finalMix)+(previous_[1]+inR*gater_.dryGain()*(1.-reverb_.killAmount()))*finalMix);
    ++write_;
  }
  void process(float inL,float inR,double beat,float& outL,float& outR) {
    if(settings_.order>=6) {
      processParallel(inL,inR,beat,outL,outR);
      if(orderFade_>0 && settings_.mix>0.) {
        double t=1.-orderFade_/128.;outL=float(lastOutput_[0]*(1.-t)+outL*t);
        outR=float(lastOutput_[1]*(1.-t)+outR*t);--orderFade_;
      }
      lastOutput_[0]=outL;lastOutput_[1]=outR;
      return;
    }
    if(buffer_[0].empty()) { outL=inL; outR=inR; return; }
    const int64_t step=int64_t(std::floor(beat/std::max(.015625,settings_.division)+1e-8));
    const int gateIndex=int((step%steps+steps)%steps);
    const bool activeStep=(settings_.pattern&(1u<<gateIndex))!=0;
    const int64_t rhythm=int64_t(std::floor(beat/.25+1e-8));
    const int slot=int((rhythm%16+16)%16);
    const bool gateTrigger=step!=lastStep_,rhythmTrigger=rhythm!=lastRhythm_;
    const bool glitchOpen=settings_.glitchSequence ? (settings_.glitchPattern&(1u<<slot))!=0 : activeStep;
    const bool repeatOpen=settings_.repeatSequence ? (settings_.repeatPattern&(1u<<slot))!=0 : activeStep;
    const auto values=modulation_.process(settings_.lfos,modBase_,beat,settings_.sampleRate,gateTrigger&&activeStep,step);
    grainSize_=.015+values[0]*.235; grainDensity_=1.+values[1]*31.;
    grainPitch_=-48.+values[2]*96.; grainPosition_=.015+values[3]*1.985; grainChaos_=values[4];
    grainLevel_=mixModulated_ ? values[5]+levelSmoothing_*(grainLevel_-values[5]) : settings_.grainMix;
    const double samplesPerBeat=settings_.sampleRate*60./std::max(20.,settings_.tempo);
    if(settings_.densityFlow) {
      if(gateTrigger) grainClock_=0.;
      if(activeStep) { if(grainClock_<=0.) { trigger(); grainClock_+=1.; }
        grainClock_-=grainDensity_/(samplesPerBeat*settings_.division); }
    } else if(gateTrigger&&activeStep) for(int i=0,n=std::clamp(int(std::round(grainDensity_)),1,8);i<n;++i) trigger();
    const bool glitchRequest=glitchOpen&&settings_.glitchMix>0.;
    bool refresh=glitchRequest&&(!glitchBlock_||settings_.glitchChance!=lastGlitchChance_||
      (settings_.glitchRefresh>0.&&beat+1e-8>=nextGlitch_));
    if(refresh){
      glitchChosen_=random()<settings_.glitchChance;
      glitchOffset_=settings_.glitchMove*random()*samplesPerBeat*4.;
      int shift=int(std::round((random()*2.-1.)*settings_.glitchVariation*3.));
      glitchSlice_=std::clamp(settings_.glitchBeats*std::pow(2.,shift),.03125,4.);
      nextGlitch_=beat+std::max(settings_.glitchRefresh,glitchSlice_*2.);
    }
    if(!glitchRequest)nextGlitch_=-1e30;
    glitchBlock_=glitchRequest;lastGlitchChance_=settings_.glitchChance;
    if(settings_.glitchVariation==0.)glitchSlice_=settings_.glitchBeats;
    const double repeatBeats=settings_.repeatSequence&&settings_.repeatRates[slot]>0. ? settings_.repeatRates[slot] : settings_.repeatBeats;
    const double pitch=settings_.repeatSequence ? settings_.repeatPitches[slot] : 0.;
    int64_t cycle=int64_t(std::floor(beat/std::max(.03125,settings_.repeatInterval)+1e-8));
    bool autoTrigger=settings_.repeatAuto&&cycle!=repeatCycle_&&!settings_.repeatOn;
    if(autoTrigger)repeatUntil_=random()<settings_.repeatChance?beat+settings_.repeatDuration:beat;
    repeatCycle_=cycle;
    bool autoOpen=settings_.repeatAuto&&beat<repeatUntil_;
    bool repeatRequest=settings_.repeatOn||autoOpen||(settings_.repeatSequence&&repeatOpen&&settings_.repeatMix>0.);
    bool repeatCapture=autoTrigger||(settings_.repeatOn&&!previousHold_);
    previousHold_=settings_.repeatOn;
    lastStep_=step;lastRhythm_=rhythm;
    auto smoothGate=[&](double& g,bool enabled) {
      g=(enabled?1.:0.)+(enabled?gateAttack_:gateRelease_)*(g-(enabled?1.:0.));
    };
    smoothGate(gate_,activeStep);smoothGate(glitchGate_,glitchOpen);smoothGate(repeatGate_,repeatOpen);
    double signal[2]={inL,inR};
    const int order=std::clamp(settings_.order,0,5);
    for(int stage:aztecOrder(order)) {
      double before[2]={signal[0],signal[1]};
      double target=settings_.moduleOn[stage]?1.:0.;moduleBlend_[stage]=target+levelSmoothing_*(moduleBlend_[stage]-target);
      if(stage==0) {
        size_t idx=size_t(write_%int64_t(buffer_[0].size()));
        for(int ch=0;ch<2;++ch) buffer_[ch][idx]=float(std::clamp(signal[ch]+previous_[ch]*settings_.feedback,-4.,4.));
        double wet[2]={};int playing=0;double weight=0.;
        for(auto& v:voices_) if(v.active) {
          double phase=double(v.age)/v.length;
          double window=.5-.5*std::cos(2.*pi*phase);
          double vl=read(0,v.read),vr=read(1,v.read);
          wet[0]+=(vl*(1.-std::max(0.,v.pan))+vr*std::max(0.,-v.pan))*window;
          wet[1]+=(vr*(1.+std::min(0.,v.pan))+vl*std::max(0.,v.pan))*window;
          weight+=window;++playing;v.read+=v.increment;
          if(++v.age>=v.length) v.active=false;
        }
        double scale=settings_.densityFlow ? 1./std::max(.25,weight) : (playing ? 1./std::sqrt(double(playing)) : 0.);
        for(int ch=0;ch<2;++ch) signal[ch]=signal[ch]*(1.-grainLevel_)+wet[ch]*scale*grainLevel_*gate_;
      } else if(stage==1) {
        float l=float(signal[0]),r=float(signal[1]);
        serialGlitch_.process(l,r,glitchRequest&&glitchChosen_,glitchRequest,
          glitchSlice_*samplesPerBeat,0.,settings_.glitchReverse,settings_.glitchMix,glitchOffset_,refresh);
        signal[0]=l;signal[1]=r;
      } else {
        float l=float(signal[0]),r=float(signal[1]);
        serialRepeat_.process(l,r,repeatRequest,repeatRequest,
          repeatBeats*samplesPerBeat,pitch,false,settings_.repeatMix,0.,repeatCapture);
        signal[0]=l;signal[1]=r;
      }
      for(int ch=0;ch<2;++ch)signal[ch]=before[ch]+moduleBlend_[stage]*(signal[ch]-before[ch]);
    }
    ++write_;
    // Feedback follows the selected chain without an extra dry injection.
    previous_[0]=signal[0]-inL;previous_[1]=signal[1]-inR;
    float wl=float(signal[0]),wr=float(signal[1]);stretch_.process(wl,wr,settings_.stretchOn,.25+values[6]*3.75);transpose_.process(wl,wr,-48.+values[7]*96.);gater_.process(wl,wr,beat,settings_.tempo);filter_.set(settings_.filterOn,settings_.filterType,20.*std::pow(1000.,values[8]),values[9],false,settings_.filterSlope,values[10]*24.);filter_.process(wl,wr);reverb_.process(wl,wr,beat);signal[0]=wl;signal[1]=wr;
    match_.process(inL,inR,signal[0],signal[1],settings_.normalize);
    double finalMix=settings_.mix+(1.-settings_.mix)*reverb_.killAmount();
    outL=float(inL*(1.-finalMix)+(signal[0]+inL*gater_.dryGain()*(1.-reverb_.killAmount()))*finalMix);outR=float(inR*(1.-finalMix)+(signal[1]+inR*gater_.dryGain()*(1.-reverb_.killAmount()))*finalMix);
    if(orderFade_>0 && settings_.mix>0.) {
      double t=1.-orderFade_/128.;outL=float(lastOutput_[0]*(1.-t)+outL*t);
      outR=float(lastOutput_[1]*(1.-t)+outR*t);--orderFade_;
    }
    lastOutput_[0]=outL;lastOutput_[1]=outR;
  }
private:
  static const std::array<int,3>& aztecOrder(int index) {
    static constexpr std::array<std::array<int,3>,6> orders={{{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}}};
    return orders[size_t(index)];
  }

};
}
