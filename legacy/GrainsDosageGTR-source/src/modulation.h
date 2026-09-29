#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace qg {
constexpr int lfoCount = 4;
constexpr int modTargetCount = 11;
constexpr int waveCount = 128;
constexpr int tableSize = 1024;
constexpr double tau = 6.28318530717958647692;
// Destination order is persistent: Size, Density, Pitch, Lookback, Chaos, Granular Mix, Speed, Transpose.
struct LfoSettings {
  bool enabled = false, sync = true, gateReset = false;
  int randomSteps = 1;
  int wave = 0; // 0..127 periodic waves, 128 S&H, 129 S&G
  double speed=1.;
  int waveRandom=0; // Off, 1/1, 1/2, 1/4, 1/8 (host quarter-note beats)
  double hz = 1., beats = 4., depth = 1., phase = 0., glide = 1.;
  std::array<double, modTargetCount> amount{}; // bipolar normalized destination range
};
using ModValues = std::array<double, modTargetCount>;
inline double cycleNoise(int64_t cycle, uint64_t seed) {
  uint64_t h = uint64_t(cycle) + seed + 0x9e3779b97f4a7c15ULL;
  h = (h ^ (h >> 30)) * 0xbf58476d1ce4e5b9ULL;
  h = (h ^ (h >> 27)) * 0x94d049bb133111ebULL;
  h ^= h >> 31;
  return double(h >> 11) * (2. / 9007199254740992.) - 1.;
}
inline double smooth(double t) { return t*t*(3.-2.*t); }
using WaveBank = std::array<std::array<float,tableSize+1>,waveCount>;
inline const WaveBank& waveBank() {
  // Constructed explicitly by prepare(), never lazily from the audio callback.
  static const WaveBank bank = [] {
    WaveBank result{};
    for(int wave=0;wave<waveCount;++wave) {
      const int family=wave/16, variant=wave%16;
      const double t=variant/15.;
      for(int i=0;i<tableSize;++i) {
        const double p=double(i)/tableSize;
        double y=0.;
        switch(family) {
          case 0: y=std::sin(tau*p + t*1.5*std::sin(tau*p)); break;
          case 1: { const double peak=.05+.9*t; y=p<peak ? -1.+2.*p/peak : 1.-2.*(p-peak)/(1.-peak); break; }
          case 2: y=p<(.05+.9*t) ? 1. : -1.; break;
          case 3: y=2.*std::pow(p,.25+3.75*t)-1.; break;
          case 4: y=1.-2.*std::pow(p,.25+3.75*t); break;
          case 5: y=(std::sin(tau*p)+.5*std::sin(tau*(variant+2)*p))/1.5; break;
          case 6: { const int n=variant+2; y=2.*std::floor(p*n)/(n-1)-1.; break; }
          case 7: {
            const int n=variant+4; const double position=p*n;
            const int index=int(position); const double fraction=position-index;
            const auto seed=uint64_t(variant+1)*7919;
            const double a=cycleNoise(index,seed), b=cycleNoise((index+1)%n,seed);
            y=a+(b-a)*smooth(fraction); break;
          }
        }
        result[wave][i]=float(std::clamp(y,-1.,1.));
      }
      result[wave][tableSize]=result[wave][0];
    }
    return result;
  }();
  return bank;
}

class Lfo {
  const WaveBank* bank_ = nullptr;
  double freePhase_=0., gateBeat_=0.,position_=0.;
  int64_t freeCycle_=0, gateEpoch_=0,displayCycle_=0;
  uint64_t seed_=1;
  int activeWave_=0,previousWave_=-1; double transition_=1.,lastOutput_=0.,transitionFrom_=0.;
public:
  void prepare(uint64_t seed) { bank_=&waveBank(); seed_=seed; reset(); }
  void reset() { position_=freePhase_=gateBeat_=0.;previousWave_=-1;activeWave_=0;transition_=1.;lastOutput_=transitionFrom_=0.; displayCycle_=freeCycle_=gateEpoch_=0; }
  int wave()const{return activeWave_;}
  double phase()const{return position_;}
  int64_t cycle()const{return displayCycle_;}
  int64_t epoch()const{return gateEpoch_;}
  double process(const LfoSettings& s,double beat,double sampleRate,bool gateTrigger,int64_t gateStep) {
    if(!s.enabled){position_=0.;return 0.;}
    if(s.gateReset && gateTrigger) {
      gateBeat_=beat; freePhase_=0.; freeCycle_=0; gateEpoch_=gateStep;
    }
    double position=0.; int64_t cycle=0;
    if(s.sync) {
      const double cycles=(beat-(s.gateReset ? gateBeat_ : 0.))/std::max(.03125,s.beats)*s.speed+s.phase;
      cycle=int64_t(std::floor(cycles)); position=cycles-double(cycle);
    } else {
      const double shifted=freePhase_+s.phase;
      const int extra=int(std::floor(shifted)); cycle=freeCycle_+extra; position=shifted-extra;
      freePhase_+=std::clamp(s.hz,.01,40.)*s.speed/sampleRate;
      const int wraps=int(std::floor(freePhase_)); freePhase_-=wraps; freeCycle_+=wraps;
    }
    position_=position;displayCycle_=cycle;
    const double intervals[]={4.,2.,1.,.5};
    int wave=std::clamp(s.wave,0,129);
    if(s.waveRandom>0){int64_t step=int64_t(std::floor(beat/intervals[std::clamp(s.waveRandom-1,0,3)]+1e-9));wave=std::clamp(int((cycleNoise(step,seed_+0x57a9ULL)+1.)*.5*waveCount),0,waveCount-1);}
    activeWave_=wave;
    if(previousWave_>=0&&previousWave_!=wave){transition_=s.waveRandom>0?0.:1.;transitionFrom_=lastOutput_;}
    previousWave_=wave;
    double output=0.;
    if(wave>=waveCount) {
      const int points=std::clamp(s.randomSteps,1,64);
      const double point=position*points;
      cycle=cycle*points+int64_t(std::floor(point)); position=point-std::floor(point);
      const uint64_t seed=seed_+(s.gateReset ? uint64_t(gateEpoch_)*0x9e3779b97f4a7c15ULL : 0);
      const double next=cycleNoise(cycle,seed);
      if(wave==waveCount) output=next;
      else {
        const double previous=cycleNoise(cycle-1,seed);
        const double fraction=std::clamp(position/std::max(.01,s.glide),0.,1.);
        output=previous+(next-previous)*smooth(fraction);
      }
    } else if(bank_) {
      const auto& table=(*bank_)[std::clamp(wave,0,waveCount-1)];
      const double point=position*tableSize;
      const int a=std::clamp(int(point),0,tableSize-1);
      output=table[a]+(table[a+1]-table[a])*(point-a);
    }
    if(transition_<1.){transition_=std::min(1.,transition_+1./((.010+.240*std::clamp(s.glide,0.,1.))*sampleRate));output=transitionFrom_+(output-transitionFrom_)*smooth(transition_);}
    lastOutput_=output;return output*std::clamp(s.depth,0.,1.);
  }
};

class Modulation {
  std::array<Lfo,lfoCount> lfos_{};
public:
  void prepare() { for(int i=0;i<lfoCount;++i) lfos_[i].prepare(0x13579BDFULL+uint64_t(i)*104729); }
  void reset() { for(auto& lfo:lfos_) lfo.reset(); }
  int wave(int i)const{return lfos_[i].wave();}
  double phase(int i)const{return lfos_[i].phase();}
  int64_t cycle(int i)const{return lfos_[i].cycle();}
  int64_t epoch(int i)const{return lfos_[i].epoch();}
  ModValues process(const std::array<LfoSettings,lfoCount>& settings,const ModValues& base,
                    double beat,double sampleRate,bool gateTrigger,int64_t gateStep) {
    ModValues result=base;
    for(int i=0;i<lfoCount;++i) {
      const double value=lfos_[i].process(settings[i],beat,sampleRate,gateTrigger,gateStep);
      for(int target=0;target<modTargetCount;++target) result[target]+=value*settings[i].amount[target];
    }
    for(auto& value:result) value=std::clamp(value,0.,1.);
    return result;
  }
};
}
