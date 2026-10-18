#pragma once
#include "mod_destinations.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <memory>

namespace qg {
constexpr int lfoCount = 4;

constexpr int waveCount = 128; // Legacy periodic shapes; persistent IDs stay unchanged.
constexpr int extraWaveCount=200, tableWaveCount=waveCount+extraWaveCount, waveChoiceCount=tableWaveCount+2, maxWave=waveChoiceCount-1;
inline constexpr const char* extraWaveFamilies[]={"FM Orbit","Phase Fold","Ripple Ramp","Pulse Train","Bouncing","Exponential Steps","Harmonic Bloom","Pendulum","Smooth Scatter","Rhythmic Gates"};
inline std::string extraWaveName(int id){int n=id-130;return std::to_string(id+1)+" "+extraWaveFamilies[n/20]+" "+std::to_string(n%20+1);}
constexpr int tableSize = 1024;
constexpr double tau = 6.28318530717958647692;
// Destination order is persistent: Size, Density, Pitch, Lookback, Chaos, Granular Mix, Speed, Transpose.
struct LfoSettings {
  bool enabled = false, sync = true, gateReset = false;
  int randomSteps = 1;
  int wave = 0; // 0..127 legacy, 128/129 S&H/S&G, 130..329 new shapes
  int randomWaveCount=waveCount;
  int randomFirst=-1,randomLast=maxWave; // Inclusive wave IDs; -1 retains legacy DSP API selection.
  double speed=1.;
  int waveRandom=0; // Off, 1/1, 1/2, 1/4, 1/8, 2/1, 4/1 (host quarter-note beats)
  double hz = 1., beats = 4., depth = 1., phase = 0., glide = 1.;
  std::array<double, modTargetCount> positive{},negative{};
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
using WaveBank = std::array<std::array<float,tableSize+1>,tableWaveCount>;
inline const WaveBank& waveBank() {
  // Constructed explicitly by prepare(), never lazily from the audio callback.
  static const auto bank = [] {
    auto storage=std::make_unique<WaveBank>();auto& result=*storage;
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
    // Precompute new families once; audio processing remains one table lookup.
    for(int n=0;n<extraWaveCount;++n){int family=n/20,v=n%20;double amount=(v+1.)/20.;auto& table=result[waveCount+n];
      for(int i=0;i<tableSize;++i){double p=double(i)/tableSize,angle=tau*p,y=0.;int harmonics=2+v%7;
        switch(family){
        case 0:y=std::sin(angle+(.25+2.75*amount)*std::sin((2+v%5)*angle));break;
        case 1:y=std::asin(std::sin(angle+(.2+1.8*amount)*std::sin(harmonics*angle)))/(tau*.25);break;
        case 2:y=(2*p-1.+amount*.8*std::sin(harmonics*angle))/(1.+amount*.8);break;
        case 3:{double duty=.08+.84*amount;double pulse=std::tanh((std::sin(harmonics*angle)-std::cos(tau*duty*.5))*(2.+v));y=.8*pulse+.2*std::sin(angle+.13*v);break;}
        case 4:y=2*std::pow(std::abs(std::sin((1+v%5)*angle*.5)),.35+3.65*amount)-1.;break;
        case 5:{int steps=3+v%10;double at=p*steps;double a=std::floor(at)/steps,b=(std::floor(at)+1)/steps;double f=std::pow(at-std::floor(at),.27+3*amount);y=2*(a+(b-a)*f)-1.;break;}
        case 6:{double sum=0,norm=0;for(int h=1;h<=harmonics;++h){double gain=1./std::pow(double(h),.4+2*amount);sum+=gain*std::sin(h*angle+.17*v*(h-1));norm+=gain;}y=sum/norm;break;}
        case 7:y=std::sin(angle)*(1.-amount*.75)+amount*.75*std::cos((2+v%6)*angle+.11*v);break;
        case 8:{int points=3+v%9;double at=p*points;int j=int(at);double f=smooth(at-j);double a=cycleNoise(j,0xa135+v*991),b=cycleNoise((j+1)%points,0xa135+v*991);y=a+(b-a)*f;break;}
        case 9:{int steps=4+v%13;double at=p*steps;int j=int(at);double f=at-j;double gate=cycleNoise(j,0xc983+v*7919)>.1?1.:-1.;y=gate*std::pow(std::sin(tau*.5*f),.2+2*amount);break;}
        }table[i]=float(std::clamp(y,-1.,1.));
      }table[tableSize]=table[0];
    }
    return storage;
  }();
  return *bank;
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
    const double intervals[]={4.,2.,1.,.5,8.,16.};
    int wave=std::clamp(s.wave,0,maxWave);
    if(s.waveRandom>0){int64_t step=int64_t(std::floor(beat/intervals[std::clamp(s.waveRandom-1,0,5)]+1e-9));int count=std::clamp(s.randomWaveCount,waveCount,tableWaveCount);wave=std::clamp(int((cycleNoise(step,seed_+0x57a9ULL)+1.)*.5*count),0,count-1);if(wave>=waveCount)wave+=2;
      if(s.randomFirst>=0){int first=std::clamp(std::min(s.randomFirst,s.randomLast),0,maxWave),last=std::clamp(std::max(s.randomFirst,s.randomLast),0,maxWave);wave=first+std::min(last-first,int((cycleNoise(step,seed_+0x57a9ULL)+1.)*.5*(last-first+1)));}
    }
    activeWave_=wave;
    if(previousWave_>=0&&previousWave_!=wave){transition_=s.waveRandom>0?0.:1.;transitionFrom_=lastOutput_;}
    previousWave_=wave;
    double output=0.;
    if(wave==128||wave==129) {
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
      const auto& table=(*bank_)[wave>=130?wave-2:wave];
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
  std::array<std::array<int,modTargetCount>,lfoCount> routes_{};
  std::array<int,lfoCount> routeCounts_{};
  std::array<double,lfoCount> outputs_{};
public:
  void prepare() { for(int i=0;i<lfoCount;++i) lfos_[i].prepare(0x13579BDFULL+uint64_t(i)*104729); }
  void reset() { for(auto& lfo:lfos_) lfo.reset(); outputs_.fill(0.); }
  double output(int i)const{return outputs_[i];}
  int wave(int i)const{return lfos_[i].wave();}
  double phase(int i)const{return lfos_[i].phase();}
  int64_t cycle(int i)const{return lfos_[i].cycle();}
  int64_t epoch(int i)const{return lfos_[i].epoch();}
  void configure(const std::array<LfoSettings,lfoCount>& settings){
    routeCounts_.fill(0);
    for(int i=0;i<lfoCount;++i)for(int t=0;t<modTargetCount;++t)
      if(settings[i].amount[t]!=0.||settings[i].positive[t]!=0.||settings[i].negative[t]!=0.)routes_[i][routeCounts_[i]++]=t;
  }
  ModValues process(const std::array<LfoSettings,lfoCount>& settings,const ModValues& base,
                    double beat,double sampleRate,bool gateTrigger,int64_t gateStep,bool configured=false) {
    if(!configured)configure(settings);
    ModValues result=base;
    for(int i=0;i<lfoCount;++i) {
      const double value=lfos_[i].process(settings[i],beat,sampleRate,gateTrigger,gateStep);
      outputs_[i]=settings[i].enabled?value:0.;
      if(!settings[i].enabled)continue;
      const double unipolar=(value+std::clamp(settings[i].depth,0.,1.))*.5;
      for(int n=0;n<routeCounts_[i];++n){const int target=routes_[i][n];
        result[target]+=value*settings[i].amount[target]+unipolar*(settings[i].positive[target]-settings[i].negative[target]);
      }
    }
    for(auto& value:result) value=std::clamp(value,0.,1.);
    return result;
  }
};
}

