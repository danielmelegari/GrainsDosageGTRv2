#pragma once
#include "parameters.h"
#include "modulation.h"
#include "stretch.h"
#include "transpose.h"
#include "rhythmic_loop.h"
#include "level_match.h"
#include "step_reverb.h"
#include "master_fx.h"
#include "gater.h"
#include "quantized_trigger.h"
#include "reslice.h"
#include "filter_bank.h"
#include "filter_sequencer.h"
#include "space_reverb.h"
#include <atomic>
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
  FilterSequenceSettings filterSequence;
  GaterSettings gater;ResliceSettings reslice;bool playing=true;int filterModel=0,reverbModel=0;
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
  bool glitchRandom=false; double glitchTriggerBeats=1.;
  bool glitchSequence=false, repeatSequence=false, stretchOn=false;
  uint16_t glitchPattern=0xFFFF, repeatPattern=0xFFFF;
  std::array<double,16> repeatRates{}, repeatPitches{};
  double stretchSpeed=1., transpose=0.;
  double glitchMove=0.,glitchVariation=0.,glitchRefresh=0.;
  bool repeatAuto=false;double repeatInterval=4.,repeatDuration=1.,repeatChance=1.;
  bool densityFlow=false, bypass=false;
  double bufferSeconds=16.; // selectable grain/recording buffer size (seconds)
  bool freeze=false;        // hold the input stream: grains keep reading the frozen audio
  int freezeBucket=-1;      // BUFFER SIZE bucket captured when FREEZE engaged (-1 = none): a change re-freezes the new selection
  int order=6;
  int routing=-1; // -1 keeps the legacy chain; 0..119 select all five stages
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
  QuantizedTrigger glitchClock_;double glitchUntil_=-1e30,glitchLastBeat_=-1e30,glitchInterval_=0.;bool randomGlitchOpen_=false;
  bool glitchBlock_=false,glitchChosen_=false;
  double glitchOffset_=0.,lastGlitchChance_=-1.,glitchSlice_=0.,nextGlitch_=-1e30;
  double repeatUntil_=-1e30;int64_t repeatCycle_=INT64_MIN;
  bool previousHold_=false;
  Settings settings_{},baseSettings_{};
  std::array<bool,modTargetCount> modRouted_{};bool extendedMod_=false;
  Modulation modulation_;
  FilterSequencer filterSequencer_;
  Gater gater_;Reslice reslice_;FilterBank filter_;LevelMatch match_;SpaceReverb reverb_;double moduleBlend_[3]={1.,1.,1.};bool panRight_=false;
  FreeStretch stretch_;
  Transpose transpose_;
  double grainClock_=0.;
  ModValues modBase_{};
  double grainSize_=.075, grainDensity_=1., grainPitch_=0., grainPosition_=.1, grainChaos_=0., grainLevel_=1.;
  double bufferSeconds_=16.; // last applied selectable buffer size (see setBufferSeconds)
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
  std::atomic<bool> freezeRequest_{false}, freezeTarget_{false}; // UI->audio freeze restore (see requestFrozen)
  // FREEZE crossfade + snapshot state (see setFrozen/applyFreezeSelection):
  // while frozen, grains read from the captured span instead of the live ring.
  double freezeStart_=0., freezeLength_=0.;   // snapshot span inside buffer_[]
  int64_t freezeRamp_=0;                       // samples remaining in the current fade
  double freezeBlend_=0.;                      // 0 = live input, 1 = fully frozen
  bool freezeFadingOut_=false;                 // true while the 100 ms release fade runs
  double freezeRead(int ch, double pos) const { // wrapped read limited to the frozen span
    if(freezeLength_<=0.) return read(ch,pos);
    double p=std::fmod(pos-freezeStart_,freezeLength_); if(p<0) p+=freezeLength_;
    return read(ch,freezeStart_+p);
  }
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
  inline static constexpr double effectTimes[]={4.,2.,1.,.5,.25,.125,.0625,.03125,2./3.,1./3.,1./6.,1./12.,1.5,.75,.375,.1875};
  inline static constexpr double triggerTimes[]={1.,2.,4.,8.},captureTimes[]={.25,.5,1.,2.,4.,8.,16.,32.},gateTimes[]={1.,.5,.25,.125},randomReverbTimes[]={1.,.5,.25},windowTimes[]={16.,8.,4.,2.},resliceTimes[]={2.,4.,8.},filterTimes[]={1.,.5,.25,.125,2.,4.};
  template<size_t N> static double normalizedTime(double v,const double(&table)[N]){size_t best=0;for(size_t i=1;i<N;++i)if(std::abs(table[i]-v)<std::abs(table[best]-v))best=i;return best/double(N-1);}
  template<size_t N> static double selectedTime(double v,const double(&table)[N]){return table[size_t(std::round(v*(N-1)))];}
  void applyExtended(const ModValues& values){
    if(!extendedMod_)return;
    if(modRouted_[11]){double v=values[11];settings_.grainPan=v*2.-1.;}
    if(modRouted_[12]){double v=values[12];settings_.glitchMix=v;}
    if(modRouted_[13]){double v=values[13];settings_.glitchChance=v;}
    // MOVE control removed from the GUI (PRESLICER); target stays fixed at centre.
    if(modRouted_[15]){double v=values[15];settings_.glitchVariation=v;}
    if(modRouted_[16]){double v=values[16];settings_.repeatMix=v;}
    if(modRouted_[17]){double v=values[17];settings_.repeatChance=v;}
    if(modRouted_[18]){double v=values[18];settings_.repeatDuration=.25+v*7.75;}
    if(modRouted_[19]){double v=values[19];settings_.reverbMix=v;}
    if(modRouted_[20]){double v=values[20];settings_.reverbLength=.2+v*19.8;}
    if(modRouted_[21]){double v=values[21];settings_.reslice.mix=v;}
    if(modRouted_[22]){double v=values[22];settings_.gater.chance=v;}
    if(modRouted_[23]){double v=values[23];settings_.gater.minimumLength=.05+v*.90;}
    if(modRouted_[24]){double v=values[24];settings_.mix=v;}
    if(modRouted_[25]){double v=values[25];settings_.filterSequence.depth=v;}
    if(modRouted_[26]){double v=values[26];settings_.filterSequence.glide=v;}
    if(modRouted_[27]){double v=values[27];settings_.glitchBeats=selectedTime(v,effectTimes);}
    if(modRouted_[28]){double v=values[28];settings_.repeatBeats=selectedTime(v,effectTimes);}
    if(modRouted_[29]){double v=values[29];settings_.glitchTriggerBeats=selectedTime(v,triggerTimes);}
    if(modRouted_[30]){double v=values[30];settings_.repeatInterval=selectedTime(v,captureTimes);}
    if(modRouted_[31]){double v=values[31];settings_.reverbGrid=selectedTime(v,gateTimes);}
    if(modRouted_[32]){double v=values[32];settings_.reverbRandomGrid=selectedTime(v,randomReverbTimes);}
    if(modRouted_[33]){double v=values[33];settings_.gater.grid=selectedTime(v,gateTimes);}
    if(modRouted_[34]){double v=values[34];settings_.reslice.beats=selectedTime(v,windowTimes);}
    if(modRouted_[35]){double v=values[35];settings_.reslice.randomBeats=selectedTime(v,resliceTimes);}
    if(modRouted_[36]){double v=values[36];settings_.filterSequence.rate=selectedTime(v,filterTimes);}
    if(modRouted_[37]){double v=values[37];settings_.repeatPitches[0]=v*96.-48.;}
    if(modRouted_[38]){double v=values[38];settings_.repeatPitches[1]=v*96.-48.;}
    if(modRouted_[39]){double v=values[39];settings_.repeatPitches[2]=v*96.-48.;}
    if(modRouted_[40]){double v=values[40];settings_.repeatPitches[3]=v*96.-48.;}
    if(modRouted_[41]){double v=values[41];settings_.repeatPitches[4]=v*96.-48.;}
    if(modRouted_[42]){double v=values[42];settings_.repeatPitches[5]=v*96.-48.;}
    if(modRouted_[43]){double v=values[43];settings_.repeatPitches[6]=v*96.-48.;}
    if(modRouted_[44]){double v=values[44];settings_.repeatPitches[7]=v*96.-48.;}
    if(modRouted_[45]){double v=values[45];settings_.repeatPitches[8]=v*96.-48.;}
    if(modRouted_[46]){double v=values[46];settings_.repeatPitches[9]=v*96.-48.;}
    if(modRouted_[47]){double v=values[47];settings_.repeatPitches[10]=v*96.-48.;}
    if(modRouted_[48]){double v=values[48];settings_.repeatPitches[11]=v*96.-48.;}
    if(modRouted_[49]){double v=values[49];settings_.repeatPitches[12]=v*96.-48.;}
    if(modRouted_[50]){double v=values[50];settings_.repeatPitches[13]=v*96.-48.;}
    if(modRouted_[51]){double v=values[51];settings_.repeatPitches[14]=v*96.-48.;}
    if(modRouted_[52]){double v=values[52];settings_.repeatPitches[15]=v*96.-48.;}
    if(modRouted_[53]){double v=values[53];settings_.gater.length[0]=.05+v*.95;}
    if(modRouted_[54]){double v=values[54];settings_.gater.length[1]=.05+v*.95;}
    if(modRouted_[55]){double v=values[55];settings_.gater.length[2]=.05+v*.95;}
    if(modRouted_[56]){double v=values[56];settings_.gater.length[3]=.05+v*.95;}
    if(modRouted_[57]){double v=values[57];settings_.gater.length[4]=.05+v*.95;}
    if(modRouted_[58]){double v=values[58];settings_.gater.length[5]=.05+v*.95;}
    if(modRouted_[59]){double v=values[59];settings_.gater.length[6]=.05+v*.95;}
    if(modRouted_[60]){double v=values[60];settings_.gater.length[7]=.05+v*.95;}
    if(modRouted_[61]){double v=values[61];settings_.gater.length[8]=.05+v*.95;}
    if(modRouted_[62]){double v=values[62];settings_.gater.length[9]=.05+v*.95;}
    if(modRouted_[63]){double v=values[63];settings_.gater.length[10]=.05+v*.95;}
    if(modRouted_[64]){double v=values[64];settings_.gater.length[11]=.05+v*.95;}
    if(modRouted_[65]){double v=values[65];settings_.gater.length[12]=.05+v*.95;}
    if(modRouted_[66]){double v=values[66];settings_.gater.length[13]=.05+v*.95;}
    if(modRouted_[67]){double v=values[67];settings_.gater.length[14]=.05+v*.95;}
    if(modRouted_[68]){double v=values[68];settings_.gater.length[15]=.05+v*.95;}
    if(modRouted_[69]){double v=values[69];settings_.gater.sustain[0]=v*.5;}
    if(modRouted_[70]){double v=values[70];settings_.gater.sustain[1]=v*.5;}
    if(modRouted_[71]){double v=values[71];settings_.gater.sustain[2]=v*.5;}
    if(modRouted_[72]){double v=values[72];settings_.gater.sustain[3]=v*.5;}
    if(modRouted_[73]){double v=values[73];settings_.gater.sustain[4]=v*.5;}
    if(modRouted_[74]){double v=values[74];settings_.gater.sustain[5]=v*.5;}
    if(modRouted_[75]){double v=values[75];settings_.gater.sustain[6]=v*.5;}
    if(modRouted_[76]){double v=values[76];settings_.gater.sustain[7]=v*.5;}
    if(modRouted_[77]){double v=values[77];settings_.gater.sustain[8]=v*.5;}
    if(modRouted_[78]){double v=values[78];settings_.gater.sustain[9]=v*.5;}
    if(modRouted_[79]){double v=values[79];settings_.gater.sustain[10]=v*.5;}
    if(modRouted_[80]){double v=values[80];settings_.gater.sustain[11]=v*.5;}
    if(modRouted_[81]){double v=values[81];settings_.gater.sustain[12]=v*.5;}
    if(modRouted_[82]){double v=values[82];settings_.gater.sustain[13]=v*.5;}
    if(modRouted_[83]){double v=values[83];settings_.gater.sustain[14]=v*.5;}
    if(modRouted_[84]){double v=values[84];settings_.gater.sustain[15]=v*.5;}
    if(modRouted_[85]){double v=values[85];settings_.reslice.slice[0]=int(std::round(v*15.));}
    if(modRouted_[86]){double v=values[86];settings_.reslice.slice[1]=int(std::round(v*15.));}
    if(modRouted_[87]){double v=values[87];settings_.reslice.slice[2]=int(std::round(v*15.));}
    if(modRouted_[88]){double v=values[88];settings_.reslice.slice[3]=int(std::round(v*15.));}
    if(modRouted_[89]){double v=values[89];settings_.reslice.slice[4]=int(std::round(v*15.));}
    if(modRouted_[90]){double v=values[90];settings_.reslice.slice[5]=int(std::round(v*15.));}
    if(modRouted_[91]){double v=values[91];settings_.reslice.slice[6]=int(std::round(v*15.));}
    if(modRouted_[92]){double v=values[92];settings_.reslice.slice[7]=int(std::round(v*15.));}
    if(modRouted_[93]){double v=values[93];settings_.reslice.slice[8]=int(std::round(v*15.));}
    if(modRouted_[94]){double v=values[94];settings_.reslice.slice[9]=int(std::round(v*15.));}
    if(modRouted_[95]){double v=values[95];settings_.reslice.slice[10]=int(std::round(v*15.));}
    if(modRouted_[96]){double v=values[96];settings_.reslice.slice[11]=int(std::round(v*15.));}
    if(modRouted_[97]){double v=values[97];settings_.reslice.slice[12]=int(std::round(v*15.));}
    if(modRouted_[98]){double v=values[98];settings_.reslice.slice[13]=int(std::round(v*15.));}
    if(modRouted_[99]){double v=values[99];settings_.reslice.slice[14]=int(std::round(v*15.));}
    if(modRouted_[100]){double v=values[100];settings_.reslice.slice[15]=int(std::round(v*15.));}
    reslice_.set(settings_.reslice);gater_.set(settings_.gater);
    reverb_.set(settings_.reverbOn,settings_.reverbType,settings_.reverbGrid,settings_.reverbPattern,settings_.reverbMix,settings_.reverbLength,settings_.reverbKill,settings_.reverbRandom,settings_.reverbRandomGrid,settings_.reverbModel);
  }
  bool scheduleGlitch(double beat,double samplesPerBeat) {
    const bool enabled=settings_.playing&&settings_.moduleOn[1]&&settings_.glitchMix>0.;
    if(glitchInterval_!=settings_.glitchTriggerBeats||beat<glitchLastBeat_||beat-glitchLastBeat_>settings_.glitchTriggerBeats){
      glitchClock_.reset();glitchUntil_=-1e30;
    }
    glitchInterval_=settings_.glitchTriggerBeats;glitchLastBeat_=beat;
    const bool fire=glitchClock_.tick(beat,settings_.glitchTriggerBeats,enabled);
    if(!enabled)glitchUntil_=-1e30;
    if(fire){
      glitchChosen_=random()<settings_.glitchChance;
      glitchOffset_=settings_.glitchMove*random()*samplesPerBeat*4.;
      const int shift=int(std::round((random()*2.-1.)*settings_.glitchVariation*3.));
      glitchSlice_=std::clamp(settings_.glitchBeats*std::pow(2.,shift),.03125,4.);
      // A burst repeats two cuts, capped at half the trigger interval.
      glitchUntil_=beat+std::min(glitchSlice_*2.,settings_.glitchTriggerBeats*.5);
    }
    randomGlitchOpen_=enabled&&glitchChosen_&&beat<glitchUntil_;
    return fire;
  }
public:
  struct GrainView{std::array<double,aztec::waveformBins> low{},high{};std::array<double,128> amplitude{};double start=0.,end=0.,head=0.,seconds=2.5;bool active=false;};
  GrainView grainView()const{
    GrainView view;if(buffer_[0].empty())return view;
    const Voice* voice=nullptr;for(const auto& v:voices_)if(v.active&&(!voice||std::abs(double(v.age)/v.length-.5)<std::abs(double(voice->age)/voice->length-.5)))voice=&v;
    double origin=voice?voice->read-voice->age*voice->increment:double(write_);
    // Fixed trailing window: spawning a grain must never recenter or zoom the
    // entire display. Frozen audio uses its own fixed source window.
    double span=std::min(settings_.sampleRate*4.,double(buffer_[0].size()-2));
    bool frozen=freezeLength_>0.&&freezeBlend_>.5;
    double left=double(write_)-span;if(frozen){left=freezeStart_;span=freezeLength_;}
    view.seconds=span/settings_.sampleRate;
    const double oldest=std::max(0.,double(write_)-double(buffer_[0].size())+1.);
    // A bounded min/max envelope preserves waveform shape and polarity. Sample
    // more positions than the old 128-bin absolute-peak display at the same 30 Hz.
    for(int i=0;i<aztec::waveformBins;++i){double lo=0.,hi=0.;
      for(int j=0;j<32;++j){double pos=left+span*(i+(j+.5)/32.)/aztec::waveformBins;
        if(frozen||(pos>=oldest&&pos<double(write_)-1.)){double l=frozen?freezeRead(0,pos):read(0,pos),r=frozen?freezeRead(1,pos):read(1,pos);lo=std::min({lo,l,r});hi=std::max({hi,l,r});}}
      view.low[i]=std::max(-1.,lo);view.high[i]=std::min(1.,hi);
    }
    for(int i=0;i<128;++i)view.amplitude[i]=std::max({-view.low[i*2],view.high[i*2],-view.low[i*2+1],view.high[i*2+1]});
    double head=voice?voice->read:0.;if(voice&&frozen){head=left+std::fmod(std::fmod(head-left,span)+span,span);origin=head-voice->age*voice->increment;}
    if(voice&&settings_.moduleOn[0]&&grainLevel_>.0001){view.active=true;view.start=std::clamp((origin-left)/span,0.,1.);view.end=std::clamp((origin+voice->length*voice->increment-left)/span,0.,1.);view.head=std::clamp((head-left)/span,0.,1.);}
    return view;
  }
  // Resize the recording buffers live (2/4/8/16 s), preserving the audible tail.
  void setBufferSeconds(double seconds) {
    if(buffer_[0].empty() || settings_.sampleRate <= 0.) return;
    const size_t capacity = size_t(std::clamp(seconds, 1., 64.) * settings_.sampleRate);
    if(capacity == buffer_[0].size()) { bufferSeconds_ = seconds; return; }
    const int64_t oldSize = int64_t(buffer_[0].size());
    const int64_t keep = std::min<int64_t>(int64_t(capacity), oldSize);
    for(int ch = 0; ch < 2; ++ch) {
      std::vector<float> rebuilt(capacity, 0.f);
      const int64_t head = write_ % oldSize; // newest sample lives at head-1
      const int64_t srcStart = head - keep;  // may be negative: wrap modulo the old ring
      for(int64_t i = 0; i < keep; ++i) {
        int64_t s = (srcStart + i) % oldSize; if(s < 0) s += oldSize;
        rebuilt[size_t(i)] = buffer_[ch][size_t(s)];
      }
      buffer_[ch].swap(rebuilt);
    }
    write_ = keep; // oldest kept sample sits at index 0, newest at keep-1
    repeatWrite_ = std::min(write_, repeatWrite_); glitchWrite_ = std::min(write_, glitchWrite_);
    for(auto& v : voices_) if(v.active) v.read = std::fmod(v.read, double(capacity));
    // The frozen snapshot lives inside the ring: re-anchor it to the rebuilt
    // copy (newest sample now sits at write_-1) so FREEZE keeps sounding after
    // a BUFFER SIZE switch. applyFreezeSelection() then re-captures the span
    // around the newly selected size with its 50 ms fade-in.
    if(freezeLength_>0.) { double a=double(write_)-freezeLength_; const double n=double(buffer_[0].size()); a=std::fmod(a,n); if(a<0) a+=n; freezeStart_=a; }
    bufferSeconds_ = seconds;
  }
  bool isFrozen() const { return settings_.freeze; }
  // FREEZE engages/disengages with a live crossfade between the incoming
  // stream and the frozen snapshot: 50 ms fade-in when freezing (the held
  // audio eases in over the input), 100 ms fade-out when unfreezing (the
  // snapshot eases away as the input returns). The frozen span always covers
  // the current BUFFER SIZE selection (see applyFreezeSelection): entering
  // FREEZE captures that many seconds of history; changing BUFFER SIZE while
  // frozen re-freezes around the new selection.
  void toggleFreeze() { setFrozen(!settings_.freeze); }
  void requestToggleFreeze() { requestFrozen(!settings_.freeze); } // cross-thread variant (automation-safe)
  void setFrozen(bool on) {
    if(on==settings_.freeze && !freezeFadingOut_) return;
    const double sr = std::max(8000., settings_.sampleRate);
    if(on) {
      // Re-freezing while a release fade is still running: fold the remaining
      // snapshot tail into the new 50 ms fade-in instead of stacking ramps.
      freezeFadingOut_=false;
      settings_.freeze = baseSettings_.freeze = true;
      freezeRamp_ = int64_t(std::llround(0.050*sr));   // 50 ms fade-IN of the frozen span
      freezeBlend_ = 0.;                               // start from the live stream
      // Snapshot the last bufferSeconds of ring history: grains keep looping it.
      const size_t cap = buffer_[0].empty()?0:buffer_[0].size();
      if(cap) {
        const int64_t keep = std::min<int64_t>(int64_t(cap), std::max(int64_t(1), int64_t(std::llround(std::clamp(bufferSeconds_,1.,64.)*sr))));
        // Newest sample sits at write_%cap-1 in the ring; read() wraps modulo
        // cap, so anchor the span to the wrapped head (not the raw counter).
        freezeStart_ = double((write_-keep)%int64_t(cap)); freezeLength_ = double(keep);
        settings_.freezeBucket = baseSettings_.freezeBucket = bucketForSeconds(bufferSeconds_);
      }
    } else {
      settings_.freeze = baseSettings_.freeze = false;
      settings_.freezeBucket = baseSettings_.freezeBucket = -1;
      if(freezeLength_>0.) {
        // Keep whatever blend was reached so far and ride the rest of the
        // snapshot down to silence over a full 100 ms fade-OUT.
        const double remain = std::clamp(freezeBlend_,0.,1.);
        freezeRamp_ = std::max<int64_t>(1, int64_t(std::llround(0.100*sr)));
        freezeBlend_ = remain; freezeFadingOut_ = remain>0.;
        if(!freezeFadingOut_) freezeLength_=0.;
      } else freezeRamp_=0;
    }
  }
  // Cross-thread freeze request: the UI thread (setComponentState / host edit)
  // only raises this flag; process() applies it on the audio thread, where
  // touching settings_ is safe. A restored session that saved frozen=1 therefore
  // actually freezes instead of being silently unfrozen by the next engine.set().
  void requestFrozen(bool on) { freezeRequest_.store(true, std::memory_order_release); freezeTarget_.store(on, std::memory_order_release); }
  bool takeFreezeRestore(bool& on) { if(!freezeRequest_.exchange(false, std::memory_order_acq_rel)) return false; on = freezeTarget_.load(std::memory_order_acquire); return true; }
  // BUFFER SELECTION drives FREEZE: map a duration to its 1/2/4/8/16 s bucket.
  static int bucketForSeconds(double seconds) {
    const double sizes[]={1.,2.,4.,8.,16.}; int best=0;
    for(int i=1;i<5;++i) if(std::abs(sizes[i]-seconds)<std::abs(sizes[best]-seconds)) best=i;
    return best;
  }
  // Called after every live buffer-size switch: while frozen, FREEZE follows
  // the selection — switching buckets re-captures the frozen span around the
  // new BUFFER SIZE (with the same 50 ms fade-in).
  void applyFreezeSelection(int bucket) {
    if(!settings_.freeze) { settings_.freezeBucket = baseSettings_.freezeBucket = -1; return; }
    if(bucket==settings_.freezeBucket) return;
    const double sizes[]={1.,2.,4.,8.,16.};
    const double sr = std::max(8000., settings_.sampleRate);
    const size_t cap = buffer_[0].empty()?0:buffer_[0].size();
    if(!cap) { settings_.freezeBucket = baseSettings_.freezeBucket = bucket; return; }
    const int64_t keep = std::min<int64_t>(int64_t(cap), std::max(int64_t(1), int64_t(std::llround(sizes[std::clamp(bucket,0,4)]*sr))));
    freezeStart_ = double((write_-keep)%int64_t(cap)); freezeLength_ = double(keep);
    freezeRamp_ = int64_t(std::llround(0.050*sr)); freezeBlend_ = 0.; freezeFadingOut_=false;
    settings_.freezeBucket = baseSettings_.freezeBucket = bucket;
  }
  void prepare(double sampleRate) {
    settings_.sampleRate = std::max(8000., sampleRate);
    filterSequencer_.reset();gater_.prepare(settings_.sampleRate);reslice_.prepare(settings_.sampleRate);filter_.prepare(settings_.sampleRate);match_.prepare(settings_.sampleRate);reverb_.prepare(settings_.sampleRate);panRight_=false;for(int i=0;i<3;++i)moduleBlend_[i]=settings_.moduleOn[i]?1.:0.;
    const size_t capacity = size_t(settings_.sampleRate * std::clamp(bufferSeconds_, 1., 64.));
    for(auto& b : buffer_) b.assign(capacity, 0.f);
    for(auto& b : repeatBuffer_) b.assign(capacity, 0.f);
    for(auto& b : glitchBuffer_) b.assign(capacity,0.f);
    write_ = 0; lastStep_ = INT64_MIN; gate_ = 0.; previous_[0] = previous_[1] = 0.;
    for(auto& v : voices_) v = {};
    repeatWrite_=glitchWrite_=0; lastOrder_=settings_.order; orderFade_=0; lastOutput_[0]=lastOutput_[1]=0.; glitchActive_ = repeating_ = false;
    stretch_.prepare(settings_.sampleRate); transpose_.prepare(settings_.sampleRate); grainClock_=0.; lastRhythm_=INT64_MIN; repeatPhase_=0.; repeatGate_=glitchGate_=0.; transition_=0;
    serialRepeat_.prepare(settings_.sampleRate);serialGlitch_.prepare(settings_.sampleRate);glitchBlock_=false;glitchClock_.reset();glitchUntil_=-1e30;randomGlitchOpen_=false;nextGlitch_=-1e30;repeatCycle_=INT64_MIN;repeatUntil_=-1e30;previousHold_=false;
    modulation_.prepare(); grainLevel_=settings_.grainMix;
    // Reset the FREEZE crossfade engine with the buffers (no stale snapshot).
    freezeStart_=0.; freezeLength_=0.; freezeRamp_=0; freezeBlend_=0.;
    settings_.freezeBucket = baseSettings_.freezeBucket = settings_.freeze ? bucketForSeconds(bufferSeconds_) : -1;
    if(settings_.freeze) { const int64_t keep=std::min<int64_t>(int64_t(capacity),std::max(int64_t(1),int64_t(std::llround(std::clamp(bufferSeconds_,1.,64.)*settings_.sampleRate)))); freezeStart_=0.; freezeLength_=double(keep); freezeBlend_=1.; }
    levelSmoothing_=std::exp(-1./(.003*settings_.sampleRate));
  }
  void set(Settings s) {
    if(s.order!=settings_.order||s.routing!=settings_.routing) { orderFade_=128; lastOrder_=settings_.order; }
    if(s.order!=settings_.order||s.routing!=settings_.routing){serialRepeat_.reset();serialGlitch_.reset();glitchBlock_=false;glitchClock_.reset();glitchUntil_=-1e30;}
    const size_t capacity = size_t(settings_.sampleRate * std::clamp(s.bufferSeconds,1.,64.));
    if(capacity != buffer_[0].size()) setBufferSeconds(std::clamp(s.bufferSeconds,1.,64.)); // live buffer-size switch
    // FREEZE is engine-owned sustained state: settings() always carries
    // freeze=false (toggled directly on the engine), so preserve it across the
    // per-block copy instead of silently unfreezing mid-hold. (set() takes the
    // Settings by value precisely so these two lines stay local to this frame.)
    s.freeze=settings_.freeze; s.freezeBucket=settings_.freezeBucket;
    settings_ = baseSettings_ = s;modRouted_.fill(false);extendedMod_=false;
    // BUFFER SELECTION drives FREEZE: switching buckets while frozen re-freezes
    // around the newly selected span (50 ms fade-in).
    applyFreezeSelection(bucketForSeconds(std::clamp(s.bufferSeconds,1.,64.)));
    for(const auto& mod:s.lfos)if(mod.enabled&&mod.depth>0.)for(int t=0;t<modTargetCount;++t)if(mod.amount[t]!=0.||mod.positive[t]!=0.||mod.negative[t]!=0.){modRouted_[t]=true;if(t>=11)extendedMod_=true;}
    gater_.set(s.gater);reslice_.set(s.reslice);
    reverb_.set(s.reverbOn,s.reverbType,s.reverbGrid,s.reverbPattern,s.reverbMix,s.reverbLength,s.reverbKill,s.reverbRandom,s.reverbRandomGrid,s.reverbModel);
    modBase_ = {(s.size-.015)/.235,(s.density-1.)/31.,(s.pitch+48.)/96.,(s.position-.015)/1.985,s.chaos,s.grainMix,(s.stretchSpeed-.25)/3.75,(s.transpose+48.)/96.,s.filterCutoff,s.filterResonance,s.filterDrive};
    modBase_[11]=std::clamp((s.grainPan+1.)/2.,0.,1.);
    modBase_[12]=std::clamp(s.glitchMix,0.,1.);
    modBase_[13]=std::clamp(s.glitchChance,0.,1.);
    modBase_[14]=.5; // MOVE removed (PRESLICER): fixed centre base.
    modBase_[15]=std::clamp(s.glitchVariation,0.,1.);
    modBase_[16]=std::clamp(s.repeatMix,0.,1.);
    modBase_[17]=std::clamp(s.repeatChance,0.,1.);
    modBase_[18]=std::clamp((s.repeatDuration-.25)/7.75,0.,1.);
    modBase_[19]=std::clamp(s.reverbMix,0.,1.);
    modBase_[20]=std::clamp((s.reverbLength-.2)/19.8,0.,1.);
    modBase_[21]=std::clamp(s.reslice.mix,0.,1.);
    modBase_[22]=std::clamp(s.gater.chance,0.,1.);
    modBase_[23]=std::clamp((s.gater.minimumLength-.05)/.90,0.,1.);
    modBase_[24]=std::clamp(s.mix,0.,1.);
    modBase_[25]=std::clamp(s.filterSequence.depth,0.,1.);
    modBase_[26]=std::clamp(s.filterSequence.glide,0.,1.);
    modBase_[27]=std::clamp(normalizedTime(s.glitchBeats,effectTimes),0.,1.);
    modBase_[28]=std::clamp(normalizedTime(s.repeatBeats,effectTimes),0.,1.);
    modBase_[29]=std::clamp(normalizedTime(s.glitchTriggerBeats,triggerTimes),0.,1.);
    modBase_[30]=std::clamp(normalizedTime(s.repeatInterval,captureTimes),0.,1.);
    modBase_[31]=std::clamp(normalizedTime(s.reverbGrid,gateTimes),0.,1.);
    modBase_[32]=std::clamp(normalizedTime(s.reverbRandomGrid,randomReverbTimes),0.,1.);
    modBase_[33]=std::clamp(normalizedTime(s.gater.grid,gateTimes),0.,1.);
    modBase_[34]=std::clamp(normalizedTime(s.reslice.beats,windowTimes),0.,1.);
    modBase_[35]=std::clamp(normalizedTime(s.reslice.randomBeats,resliceTimes),0.,1.);
    modBase_[36]=std::clamp(normalizedTime(s.filterSequence.rate,filterTimes),0.,1.);
    modBase_[37]=std::clamp((s.repeatPitches[0]+48.)/96.,0.,1.);
    modBase_[38]=std::clamp((s.repeatPitches[1]+48.)/96.,0.,1.);
    modBase_[39]=std::clamp((s.repeatPitches[2]+48.)/96.,0.,1.);
    modBase_[40]=std::clamp((s.repeatPitches[3]+48.)/96.,0.,1.);
    modBase_[41]=std::clamp((s.repeatPitches[4]+48.)/96.,0.,1.);
    modBase_[42]=std::clamp((s.repeatPitches[5]+48.)/96.,0.,1.);
    modBase_[43]=std::clamp((s.repeatPitches[6]+48.)/96.,0.,1.);
    modBase_[44]=std::clamp((s.repeatPitches[7]+48.)/96.,0.,1.);
    modBase_[45]=std::clamp((s.repeatPitches[8]+48.)/96.,0.,1.);
    modBase_[46]=std::clamp((s.repeatPitches[9]+48.)/96.,0.,1.);
    modBase_[47]=std::clamp((s.repeatPitches[10]+48.)/96.,0.,1.);
    modBase_[48]=std::clamp((s.repeatPitches[11]+48.)/96.,0.,1.);
    modBase_[49]=std::clamp((s.repeatPitches[12]+48.)/96.,0.,1.);
    modBase_[50]=std::clamp((s.repeatPitches[13]+48.)/96.,0.,1.);
    modBase_[51]=std::clamp((s.repeatPitches[14]+48.)/96.,0.,1.);
    modBase_[52]=std::clamp((s.repeatPitches[15]+48.)/96.,0.,1.);
    modBase_[53]=std::clamp((s.gater.length[0]-.05)/.95,0.,1.);
    modBase_[54]=std::clamp((s.gater.length[1]-.05)/.95,0.,1.);
    modBase_[55]=std::clamp((s.gater.length[2]-.05)/.95,0.,1.);
    modBase_[56]=std::clamp((s.gater.length[3]-.05)/.95,0.,1.);
    modBase_[57]=std::clamp((s.gater.length[4]-.05)/.95,0.,1.);
    modBase_[58]=std::clamp((s.gater.length[5]-.05)/.95,0.,1.);
    modBase_[59]=std::clamp((s.gater.length[6]-.05)/.95,0.,1.);
    modBase_[60]=std::clamp((s.gater.length[7]-.05)/.95,0.,1.);
    modBase_[61]=std::clamp((s.gater.length[8]-.05)/.95,0.,1.);
    modBase_[62]=std::clamp((s.gater.length[9]-.05)/.95,0.,1.);
    modBase_[63]=std::clamp((s.gater.length[10]-.05)/.95,0.,1.);
    modBase_[64]=std::clamp((s.gater.length[11]-.05)/.95,0.,1.);
    modBase_[65]=std::clamp((s.gater.length[12]-.05)/.95,0.,1.);
    modBase_[66]=std::clamp((s.gater.length[13]-.05)/.95,0.,1.);
    modBase_[67]=std::clamp((s.gater.length[14]-.05)/.95,0.,1.);
    modBase_[68]=std::clamp((s.gater.length[15]-.05)/.95,0.,1.);
    modBase_[69]=std::clamp(s.gater.sustain[0]/.5,0.,1.);
    modBase_[70]=std::clamp(s.gater.sustain[1]/.5,0.,1.);
    modBase_[71]=std::clamp(s.gater.sustain[2]/.5,0.,1.);
    modBase_[72]=std::clamp(s.gater.sustain[3]/.5,0.,1.);
    modBase_[73]=std::clamp(s.gater.sustain[4]/.5,0.,1.);
    modBase_[74]=std::clamp(s.gater.sustain[5]/.5,0.,1.);
    modBase_[75]=std::clamp(s.gater.sustain[6]/.5,0.,1.);
    modBase_[76]=std::clamp(s.gater.sustain[7]/.5,0.,1.);
    modBase_[77]=std::clamp(s.gater.sustain[8]/.5,0.,1.);
    modBase_[78]=std::clamp(s.gater.sustain[9]/.5,0.,1.);
    modBase_[79]=std::clamp(s.gater.sustain[10]/.5,0.,1.);
    modBase_[80]=std::clamp(s.gater.sustain[11]/.5,0.,1.);
    modBase_[81]=std::clamp(s.gater.sustain[12]/.5,0.,1.);
    modBase_[82]=std::clamp(s.gater.sustain[13]/.5,0.,1.);
    modBase_[83]=std::clamp(s.gater.sustain[14]/.5,0.,1.);
    modBase_[84]=std::clamp(s.gater.sustain[15]/.5,0.,1.);
    modBase_[85]=std::clamp(s.reslice.slice[0]/15.,0.,1.);
    modBase_[86]=std::clamp(s.reslice.slice[1]/15.,0.,1.);
    modBase_[87]=std::clamp(s.reslice.slice[2]/15.,0.,1.);
    modBase_[88]=std::clamp(s.reslice.slice[3]/15.,0.,1.);
    modBase_[89]=std::clamp(s.reslice.slice[4]/15.,0.,1.);
    modBase_[90]=std::clamp(s.reslice.slice[5]/15.,0.,1.);
    modBase_[91]=std::clamp(s.reslice.slice[6]/15.,0.,1.);
    modBase_[92]=std::clamp(s.reslice.slice[7]/15.,0.,1.);
    modBase_[93]=std::clamp(s.reslice.slice[8]/15.,0.,1.);
    modBase_[94]=std::clamp(s.reslice.slice[9]/15.,0.,1.);
    modBase_[95]=std::clamp(s.reslice.slice[10]/15.,0.,1.);
    modBase_[96]=std::clamp(s.reslice.slice[11]/15.,0.,1.);
    modBase_[97]=std::clamp(s.reslice.slice[12]/15.,0.,1.);
    modBase_[98]=std::clamp(s.reslice.slice[13]/15.,0.,1.);
    modBase_[99]=std::clamp(s.reslice.slice[14]/15.,0.,1.);
    modBase_[100]=std::clamp(s.reslice.slice[15]/15.,0.,1.);
    mixModulated_=false;
    for(const auto& lfo:s.lfos) if(lfo.enabled && lfo.depth>0. && (lfo.amount[5]!=0.||lfo.positive[5]!=0.||lfo.negative[5]!=0.)) mixModulated_=true;
    gateAttack_ = std::exp(-1./(s.sampleRate*std::max(.0001,s.attack)));
    gateRelease_ = std::exp(-1./(s.sampleRate*std::max(.0001,s.release)));
  }
  void resetTransport() {
    // Preserve reverb tails and level matching across host seeks/loops.
    filterSequencer_.reset();reslice_.reset();gater_.resetLatch();
    serialRepeat_.reset();serialGlitch_.reset();glitchBlock_=false;glitchClock_.reset();glitchUntil_=-1e30;randomGlitchOpen_=false;nextGlitch_=-1e30;repeatCycle_=INT64_MIN;repeatUntil_=-1e30;previousHold_=false;
    lastStep_ = INT64_MIN; glitchActive_ = repeating_ = false;
    modulation_.reset(); stretch_.reset(); transpose_.reset(); grainClock_=0.; lastRhythm_=INT64_MIN; repeatGate_=glitchGate_=0.; transition_=orderFade_=0;lastOutput_[0]=lastOutput_[1]=0.;
    for(auto& v : voices_) v.active = false;
  }
  int filterSequenceStep()const{return filterSequencer_.step();}
  int resliceSource(int i)const{return reslice_.source(i);}
  int resliceStep()const{return reslice_.step();}bool resliceActive()const{return reslice_.active();}
  int gaterStep()const{return gater_.step();}int gaterState(int i)const{return gater_.state(i);}double gaterLength(int i)const{return gater_.length(i);}
  bool reverbGate()const{return reverb_.gateOpen();}
  std::atomic<uint64_t> lastModPacked_{0}; // live Size/Density/Pitch/Chaos magnitudes (aztec-packed) for the UI knob bars
public:
  int lfoWave(int i)const{return modulation_.wave(i);}
  double lfoPhase(int i)const{return modulation_.phase(i);}
  int64_t lfoCycle(int i)const{return modulation_.cycle(i);}
  int64_t lfoEpoch(int i)const{return modulation_.epoch(i);}
  bool glitchRunning()const{return settings_.moduleOn[1]&&((settings_.routing<0&&settings_.order>=6)?(glitchActive_&&(!settings_.glitchRandom||glitchGate_>.01)):serialGlitch_.active());}
  bool repeatRunning()const{return settings_.moduleOn[2]&&((settings_.routing<0&&settings_.order>=6)?repeating_:serialRepeat_.active());}
  // beat is the host PPQ beat position for this sample, or a running fallback beat.
  void processParallel(float inL, float inR, double beat, float& outL, float& outR) {
    if(buffer_[0].empty()) { outL = inL; outR = inR; return; }
    const int64_t step = int64_t(std::floor(beat / std::max(.015625, settings_.division) + 1e-8));
    const bool activeStep = (settings_.pattern & (1u << ((step % steps + steps) % steps))) != 0;
    ModValues values;values=modulation_.process(settings_.lfos,modBase_,beat,settings_.sampleRate,
                                          step!=lastStep_ && activeStep,step);
    lastModPacked_.store(aztec::packModMagnitudes(values),std::memory_order_relaxed);
    applyExtended(values);
    grainSize_=.015+values[0]*.235; grainDensity_=1.+values[1]*31.;
    grainPitch_=-48.+values[2]*96.; grainPosition_=.015+values[3]*1.985; grainChaos_=values[4];
    grainLevel_=mixModulated_ ? values[5]+levelSmoothing_*(grainLevel_-values[5]) : settings_.grainMix;
    const bool gateTrigger=step!=lastStep_;
    const int64_t rhythm=int64_t(std::floor(beat/.25+1e-8));
    const int slot=int((rhythm%16+16)%16);
    const bool rhythmTrigger=rhythm!=lastRhythm_;
    bool glitchOpen=settings_.glitchSequence ? (settings_.glitchPattern&(1u<<slot))!=0 : activeStep;
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
    if(settings_.glitchRandom){
      const bool fire=scheduleGlitch(beat,samplesPerBeat);glitchOpen=randomGlitchOpen_;
      if(fire&&glitchChosen_){
        glitchLength_=std::clamp(std::round(glitchSlice_*samplesPerBeat),16.,double(buffer_[0].size()-2));
        glitchStart_=double(write_)-glitchLength_-std::min(glitchOffset_,double(buffer_[0].size())-glitchLength_-2.);
        glitchBeat_=beat;glitchActive_=true;
      }
      if(!glitchOpen&&glitchGate_<1e-6)glitchActive_=false;
    } else if(settings_.glitchSequence ? rhythmTrigger : gateTrigger) {
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
    if(!settings_.freeze) { // FREEZE: hold the recording head so grains loop the frozen audio
    const size_t index = size_t(write_ % int64_t(buffer_[0].size()));
    buffer_[0][index] = float(std::clamp(double(inL) + previous_[0] * settings_.feedback, -4., 4.));
    buffer_[1][index] = float(std::clamp(double(inR) + previous_[1] * settings_.feedback, -4., 4.));
    }
    if(!repeating_ && !settings_.freeze) { // FREEZE: stop capturing new input into the loop pool
      const auto ri = size_t(repeatWrite_ % int64_t(repeatBuffer_[0].size()));
      repeatBuffer_[0][ri] = inL; repeatBuffer_[1][ri] = inR; ++repeatWrite_;
    }
    double wetL = 0., wetR = 0.; int playing = 0; double weight=0.;
    for(auto& v : voices_) {
      if(!v.active) continue;
      const double phase = double(v.age) / v.length;
      const double window = .5 - .5 * std::cos(2. * pi * phase);
      // FREEZE crossfade: blend the live ring read with the frozen-snapshot
      // read (50 ms fade-in on freeze, 100 ms fade-out on unfreeze).
      double vl=read(0,v.read),vr=read(1,v.read);
      if(freezeBlend_>0.) { vl+=(freezeRead(0,v.read)-vl)*freezeBlend_; vr+=(freezeRead(1,v.read)-vr)*freezeBlend_; }
      wetL+=(vl*(1.-std::max(0.,v.pan))+vr*std::max(0.,-v.pan))*window;
      wetR+=(vr*(1.+std::min(0.,v.pan))+vl*std::max(0.,v.pan))*window;
      weight+=window*window; ++playing; v.read += v.increment;
      if(++v.age >= v.length) v.active = false;
    }
    const double scale = settings_.densityFlow ? 1./std::max({1.,std::sqrt(weight),std::abs(wetL),std::abs(wetR)}) : (playing ? 1. / std::sqrt(double(playing)) : 0.);
    double sumL = settings_.moduleOn[0]?wetL*scale*grainLevel_*gate_:0., sumR=settings_.moduleOn[0]?wetR*scale*grainLevel_*gate_:0.;
    if(glitchActive_ && settings_.moduleOn[1]) {
      const double phase = (beat-glitchBeat_)/(settings_.glitchRandom?glitchSlice_:settings_.glitchBeats);
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
    // Loudness fix: weight each contribution by its ACTIVE gate so the anti-clip gain never
    // over-divides while the repeater holds (previously repeatMix counted at full value even
    // when repeatGate_ was ducked, making repeats noticeably quieter than the dry signal).
    const double grainWeight=settings_.moduleOn[0]?grainLevel_:0.;
    const double glitchWeight=(glitchActive_&&settings_.moduleOn[1])?settings_.glitchMix*(settings_.glitchRandom?glitchGate_:1.):0.;
    const double repeatWeight=(repeating_&&settings_.moduleOn[2])?settings_.repeatMix*repeatGate_:0.;
    const double gain = 1./std::max(1., grainWeight + glitchWeight + repeatWeight);
    previous_[0] = sumL * gain; previous_[1] = sumR * gain;
    if(!settings_.moduleOn[0]&&!settings_.moduleOn[1]&&!settings_.moduleOn[2]){previous_[0]=inL;previous_[1]=inR;}
    float wl=float(previous_[0]),wr=float(previous_[1]);stretch_.process(wl,wr,settings_.stretchOn,.25+values[6]*3.75);transpose_.process(wl,wr,-48.+values[7]*96.);reslice_.process(wl,wr,beat,settings_.tempo,settings_.playing);gater_.process(wl,wr,beat,settings_.tempo,settings_.playing);filter_.set(settings_.filterOn,settings_.filterType,filterSequencer_.process(settings_.filterSequence,values[8],beat,settings_.sampleRate,settings_.playing,settings_.filterModel==7||settings_.filterModel==8),values[9],false,settings_.filterSlope,values[10]*24.,0.,settings_.filterModel,settings_.filterModel==7||settings_.filterModel==8);filter_.process(wl,wr);reverb_.process(wl,wr,beat);previous_[0]=wl;previous_[1]=wr;
    match_.process(inL,inR,previous_[0],previous_[1],settings_.normalize);
    double finalMix=settings_.mix+(1.-settings_.mix)*reverb_.killAmount();
    outL = float(inL*(1.-finalMix)+previous_[0]*finalMix);outR=float(inR*(1.-finalMix)+previous_[1]*finalMix);
    ++write_;
  }
  void process(float inL,float inR,double beat,float& outL,float& outR) {
    if(extendedMod_)settings_=baseSettings_;
    // Advance the FREEZE crossfade ramp (50 ms fade-in / 100 ms fade-out).
    if(freezeRamp_>0) {
      const double sr = std::max(8000., settings_.sampleRate);
      const int64_t total = settings_.freeze ? int64_t(std::llround(0.050*sr)) : int64_t(std::llround(0.100*sr));
      if(settings_.freeze) {
        // Equal-power-ish linear rise toward the frozen snapshot over 50 ms.
        freezeBlend_ = 1. - double(freezeRamp_-1)/double(std::max<int64_t>(1,total));
        if(--freezeRamp_<=0) { freezeRamp_=0; freezeBlend_=1.; }
      } else if(freezeFadingOut_) {
        // Linear fall back to the live stream over a full 100 ms.
        freezeBlend_ = double(freezeRamp_-1)/double(std::max<int64_t>(1,total));
        if(--freezeRamp_<=0) { freezeRamp_=0; freezeBlend_=0.; freezeFadingOut_=false; freezeLength_=0.; } // snapshot released once faded out
      } else { freezeRamp_=0; }
    }
    if(settings_.routing<0&&settings_.order>=6) {
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
    bool glitchOpen=settings_.glitchSequence ? (settings_.glitchPattern&(1u<<slot))!=0 : activeStep;
    const bool repeatOpen=settings_.repeatSequence ? (settings_.repeatPattern&(1u<<slot))!=0 : activeStep;
    ModValues values;values=modulation_.process(settings_.lfos,modBase_,beat,settings_.sampleRate,gateTrigger&&activeStep,step);
    lastModPacked_.store(aztec::packModMagnitudes(values),std::memory_order_relaxed);
    applyExtended(values);
    grainSize_=.015+values[0]*.235; grainDensity_=1.+values[1]*31.;
    grainPitch_=-48.+values[2]*96.; grainPosition_=.015+values[3]*1.985; grainChaos_=values[4];
    grainLevel_=mixModulated_ ? values[5]+levelSmoothing_*(grainLevel_-values[5]) : settings_.grainMix;
    const double samplesPerBeat=settings_.sampleRate*60./std::max(20.,settings_.tempo);
    if(settings_.densityFlow) {
      if(gateTrigger) grainClock_=0.;
      if(activeStep) { if(grainClock_<=0.) { trigger(); grainClock_+=1.; }
        grainClock_-=grainDensity_/(samplesPerBeat*settings_.division); }
    } else if(gateTrigger&&activeStep) for(int i=0,n=std::clamp(int(std::round(grainDensity_)),1,8);i<n;++i) trigger();
    bool randomFire=false;
    if(settings_.glitchRandom){randomFire=scheduleGlitch(beat,samplesPerBeat);glitchOpen=randomGlitchOpen_;}
    const bool glitchRequest=glitchOpen&&settings_.glitchMix>0.;
    bool refresh=!settings_.glitchRandom&&glitchRequest&&(!glitchBlock_||settings_.glitchChance!=lastGlitchChance_||
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
    if(!settings_.glitchRandom&&settings_.glitchVariation==0.)glitchSlice_=settings_.glitchBeats;
    if(settings_.glitchRandom)refresh=randomFire&&randomGlitchOpen_;
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
    auto legacy=aztecOrder(order);
    std::array<int,5> chain=settings_.routing>=0?aztec::fiveModuleOrder(settings_.routing):std::array<int,5>{{legacy[0],legacy[1],legacy[2],-1,-1}};
    for(int stage:chain) {
      if(stage<0)continue;
      if(stage>=3){float l=float(signal[0]),r=float(signal[1]);if(stage==3)reslice_.process(l,r,beat,settings_.tempo,settings_.playing);else gater_.process(l,r,beat,settings_.tempo,settings_.playing);signal[0]=l;signal[1]=r;continue;}
      double before[2]={signal[0],signal[1]};
      double target=settings_.moduleOn[stage]?1.:0.;moduleBlend_[stage]=target+levelSmoothing_*(moduleBlend_[stage]-target);
      if(stage==0) {
        if(!settings_.freeze) { // FREEZE: hold the recording head so grains loop the frozen audio
        size_t idx=size_t(write_%int64_t(buffer_[0].size()));
        for(int ch=0;ch<2;++ch) buffer_[ch][idx]=float(std::clamp(signal[ch]+previous_[ch]*settings_.feedback,-4.,4.));
        }
        double wet[2]={};int playing=0;double weight=0.;
        for(auto& v:voices_) if(v.active) {
          double phase=double(v.age)/v.length;
          double window=.5-.5*std::cos(2.*pi*phase);
          // FREEZE crossfade (parallel path): blend live ring with frozen snapshot.
          double vl=read(0,v.read),vr=read(1,v.read);
          if(freezeBlend_>0.) { vl+=(freezeRead(0,v.read)-vl)*freezeBlend_; vr+=(freezeRead(1,v.read)-vr)*freezeBlend_; }
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
    float wl=float(signal[0]),wr=float(signal[1]);stretch_.process(wl,wr,settings_.stretchOn,.25+values[6]*3.75);transpose_.process(wl,wr,-48.+values[7]*96.);if(settings_.routing<0){reslice_.process(wl,wr,beat,settings_.tempo,settings_.playing);gater_.process(wl,wr,beat,settings_.tempo,settings_.playing);}filter_.set(settings_.filterOn,settings_.filterType,filterSequencer_.process(settings_.filterSequence,values[8],beat,settings_.sampleRate,settings_.playing,settings_.filterModel==7||settings_.filterModel==8),values[9],false,settings_.filterSlope,values[10]*24.,0.,settings_.filterModel,settings_.filterModel==7||settings_.filterModel==8);filter_.process(wl,wr);reverb_.process(wl,wr,beat);signal[0]=wl;signal[1]=wr;
    match_.process(inL,inR,signal[0],signal[1],settings_.normalize);
    double finalMix=settings_.mix+(1.-settings_.mix)*reverb_.killAmount();
    outL=float(inL*(1.-finalMix)+signal[0]*finalMix);outR=float(inR*(1.-finalMix)+signal[1]*finalMix);
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

