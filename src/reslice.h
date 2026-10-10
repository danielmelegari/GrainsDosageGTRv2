#pragma once
#include <array>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace qg {
// Original procedural cutters; no LiveCut/BBCut source code is used.
struct ResliceSettings {
 bool enabled=false,random=false;double randomBeats=4.,beats=4.,mix=1.;
 // Reserved legacy fields keep old state/automation IDs readable.
 std::array<bool,16> on{{true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true}};
 std::array<int,16> slice{{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}};
 int algorithm=0;double barBeats=4.,phraseBeats=16.,repeat=.45,variation=.5,fill=.5,reverse=.1;uint32_t seed=1;
 // Extended phrase/cut and output shaping: neutral defaults preserve the existing Reslice sound.
 int minPhrase=1,maxPhrase=8,minRepeats=0,maxRepeats=4,combType=0;
 double stutter=.8,area=.5,fadeMs=3.,duty=1.,fillDuty=1.,minAmp=1.,maxAmp=1.,minPan=0.,maxPan=0.;
 double minPitch=0.,maxPitch=0.,warpStraight=.3,warpRegular=.5,warpRitard=.5,warpSpeed=.9,pusherActivity=.5;
 bool crusherOn=false,combOn=false;
 double minBits=32.,maxBits=32.,minFreq=44100.,maxFreq=44100.,combFeedback=.5,minDelay=.01,maxDelay=.01;
};
class Reslice {
 std::array<std::vector<float>,2> buffer_;ResliceSettings s_;double sr_=48000.;int64_t write_=0;
 double lastBeat_=0.,nextCut_=0.,tempo_=120.,start_=0.,span_=0.,cursor_=0.,speed_=1.,mix_=0.;
 bool reverse_=false,playing_=false,active_=false,pending_=true;int step_=0,fade_=0,fadeSize_=24;
 std::array<int,16> pattern_{};double lastWet_[2]{},from_[2]{};uint32_t rng_=1;uint64_t events_=0;int repeatRun_=0;
 double gain_=1.,pan_=0.,duty_=1.,pitchRatio_=1.,cutDuration_=1.,cutSamples_=1.;
 int crusherCounter_=0,crusherHold_=1,crusherBits_=32;double crushed_[2]{};
 std::array<std::vector<float>,2> combBuffer_;size_t combWrite_=0,combDelay_=1;double combFb_=0.;
 double random(){rng_^=rng_<<13;rng_^=rng_>>17;rng_^=rng_<<5;return (rng_&0xffffff)/16777216.;}
 double read(int ch,double position)const{int64_t a=int64_t(std::floor(position));if(a<0||a<write_-int64_t(buffer_[ch].size())||a+1>=write_)return 0.;size_t i=size_t(a)%buffer_[ch].size(),j=(i+1)%buffer_[ch].size();return buffer_[ch][i]+(buffer_[ch][j]-buffer_[ch][i])*(position-a);}
 void crossfade(){for(int ch=0;ch<2;++ch)from_[ch]=lastWet_[ch];fade_=fadeSize_;}
 void choose(double beat,double samplesPerBeat){
  const double bar=std::clamp(s_.barBeats,.25,32.),phrase=std::clamp(s_.phraseBeats,bar*std::clamp(s_.minPhrase,1,8),bar*std::clamp(std::max(s_.minPhrase,s_.maxPhrase),1,8));double phrasePhase=beat-std::floor(beat/phrase)*phrase;
  double barPhase=beat-std::floor(beat/bar)*bar;bool fill=phrase-phrasePhase<=1.+1e-7&&random()<std::clamp(s_.fill,0.,1.);
  double variation=std::clamp(s_.variation,0.,1.),duration=.5;int mode=std::clamp(s_.algorithm,0,2);
  if(mode==0){ // CutDSG: strong bar anchors, straight cuts, phrase-ending rolls.
   duration=fill?(random()<.5?.0625:.125):(random()<variation?.25:(random()<.65?.5:1.));
  }else if(mode==1){ // WarpDSG: mixed straight/triplet micro-rhythms and playback ratios.
   constexpr double lengths[]={1./12.,.125,.1875,.25,.375,.5};int choice=int(random()*6);duration=variation>.2?lengths[choice]:.5;if(fill)duration*=.5;
  }else{ // PushDSG: a 3+1 / 2+1+1 push-pull skeleton with syncopated interruptions.
   constexpr double lengths[]={.75,.25,.5,.25,.25,.5,.25,.75,.5};
   double boundary=0;for(double length:lengths){boundary+=length;if(boundary>std::fmod(barPhase,4.)+1e-7){duration=boundary-std::fmod(barPhase,4.);break;}}
   if(random()<variation*.5)duration=std::min(duration,.125);if(fill)duration=std::min(duration,random()<.5?.0625:.125);
  }
  // Phrase-aware stutter and warp controls, kept at their legacy behavior with defaults.
  if(fill&&random()<std::clamp(s_.stutter,0.,1.))duration=std::min(duration,.125);
  if(mode==1){double warp=std::clamp(s_.warpSpeed,0.,1.);if(random()<s_.warpRegular*.15)duration=std::min(duration,.5);if(random()<s_.warpStraight)duration=std::min(duration,.25);if(random()<s_.warpRitard*.2)duration*=1.+(1.-warp);}
  if(mode==2&&random()<s_.pusherActivity*.2)duration=std::min(duration,.25);
  // Every bar and phrase begins on the host grid, even after tempo/rate changes.
  duration=std::max(1e-7,std::min(std::max(1./48.,duration),bar-barPhase));nextCut_=beat+duration;
  double history=std::min({double(write_-2),std::clamp(s_.beats,2.,16.)*samplesPerBeat,double(buffer_[0].size())-4});
  double requestedSpan=std::max(32.,duration*samplesPerBeat);speed_=1.;
  if(mode==1&&random()<variation){constexpr double ratios[]={.5,.75,1.,1.5,2.};speed_=ratios[int(random()*5)];}if(mode==1)speed_*=std::pow(2.,(std::clamp(s_.warpSpeed,0.,1.)-.9)*2.);
  bool repeat=events_>0&&repeatRun_<std::max(0,s_.maxRepeats)&&(repeatRun_>0&&repeatRun_<s_.minRepeats || random()<std::clamp(s_.repeat,0.,1.));if(repeat)++repeatRun_;else repeatRun_=0;bool anchor=barPhase<1e-7;
  if(!repeat||anchor||span_<32||start_<double(write_-2)-history){
   span_=std::min(requestedSpan,history);double depth=std::max(0.,history-span_);
   double back=(mode==0&&anchor)?std::max(0.,std::min(depth,bar*samplesPerBeat-span_)):random()*depth*variation*std::clamp(s_.area*2.,0.,2.);
   double grid=samplesPerBeat*(mode==1?1./16.:.25);back=std::floor(back/std::max(1.,grid))*grid;
   start_=double(write_-2)-span_-back;
  }else span_=std::min(span_,requestedSpan);
  cursor_=0;reverse_=random()<std::clamp(s_.reverse,0.,1.);
  const double pitch=s_.minPitch+random()*(s_.maxPitch-s_.minPitch);pitchRatio_=std::pow(2.,std::clamp(pitch,-48.,48.)/12.);
  gain_=std::clamp(s_.minAmp+random()*(s_.maxAmp-s_.minAmp),0.,2.);pan_=std::clamp(s_.minPan+random()*(s_.maxPan-s_.minPan),-1.,1.);
  duty_=std::clamp(fill?s_.fillDuty:s_.duty,0.,1.);cutDuration_=duration;cutSamples_=std::max(1.,requestedSpan);
  crusherBits_=int(std::clamp(s_.minBits+random()*(s_.maxBits-s_.minBits),2.,32.));
  double freq=std::clamp(s_.minFreq+random()*(s_.maxFreq-s_.minFreq),200.,sr_);crusherHold_=std::max(1,int(sr_/freq));
  combDelay_=size_t(std::clamp(s_.minDelay+random()*(s_.maxDelay-s_.minDelay),.0002,.1)*sr_);
  combFb_=std::clamp(s_.combFeedback,-.95,.95);
  crossfade();++events_;
  int source=int(std::clamp((double(write_-2)-start_)/std::max(1.,history),0.,.999)*16);pattern_[step_]=source;
 }
public:
 void prepare(double sr){sr_=std::max(8000.,sr);for(auto& b:buffer_)b.assign(size_t(sr_*96.)+64,0.f);for(auto& b:combBuffer_)b.assign(size_t(sr_*.11)+8,0.f);fadeSize_=std::max(8,int(sr_*.003));lastWet_[0]=lastWet_[1]=from_[0]=from_[1]=0;mix_=0;reset();}
 void reset(){write_=0;pending_=true;active_=false;span_=cursor_=0;events_=0;repeatRun_=0;rng_=s_.seed?s_.seed:1;pattern_.fill(0);fade_=0;crusherCounter_=0;combWrite_=0;for(auto& b:combBuffer_)std::fill(b.begin(),b.end(),0.f);}
 void set(const ResliceSettings& s){fadeSize_=std::max(1,int(sr_*std::clamp(s.fadeMs,.2,40.)*.001));if(s.algorithm!=s_.algorithm||s.seed!=s_.seed||s.phraseBeats!=s_.phraseBeats||s.barBeats!=s_.barBeats){pending_=true;rng_=s.seed?s.seed:1;}if(s.enabled&&!s_.enabled)pending_=true;s_=s;}
 int source(int i)const{return pattern_[std::clamp(i,0,15)];}
 void process(float& l,float& r,double beat,double tempo,bool playing=true){
  if(!std::isfinite(beat)){beat=0;playing=false;}tempo=std::isfinite(tempo)?std::clamp(tempo,20.,400.):120.;double spb=sr_*60./tempo;
  bool jump=playing_&&playing&&(beat<lastBeat_-1e-6||beat-lastBeat_>4./spb);
  if(jump||playing!=playing_)reset();if(tempo!=tempo_)pending_=true;tempo_=tempo;lastBeat_=beat;playing_=playing;
  const double input[2]={l,r};for(int ch=0;ch<2;++ch)buffer_[ch][size_t(write_)%buffer_[ch].size()]=float(input[ch]);++write_;
  step_=int((int64_t(std::floor(beat/std::clamp(s_.barBeats,.25,32.)*16+1e-8))%16+16)%16);
  bool next=s_.enabled&&playing&&write_>std::max(64.,spb*.125);
  if(next&&(pending_||beat+1e-8>=nextCut_)){choose(pending_?beat:nextCut_,spb);pending_=false;}
  active_=next;double target=next?std::clamp(s_.mix,0.,1.):0.;mix_+=std::clamp(target-mix_,-1./fadeSize_,1./fadeSize_);
  for(int ch=0;ch<2;++ch){double wet=input[ch];if(span_>2){double phase=std::min(cursor_,span_-2);wet=read(ch,start_+(reverse_?span_-2-phase:phase));if(fade_){double t=1.-double(fade_)/fadeSize_;t=t*t*(3.-2*t);wet=from_[ch]+t*(wet-from_[ch]);}}lastWet_[ch]=wet;double shaped=wet;
   if(next){
    if(std::fmod(cursor_,std::max(2.,cutSamples_))/std::max(2.,cutSamples_)>duty_)shaped=0.;
    shaped*=gain_*std::sqrt(ch==0?1.-std::max(0.,pan_):1.+std::min(0.,pan_));
    if(s_.crusherOn){if(crusherCounter_==0){double levels=std::pow(2.,std::min(crusherBits_,24));crushed_[ch]=std::round(std::clamp(shaped,-1.,1.)*levels)/levels;}shaped=crushed_[ch];}
    if(s_.combOn&&!combBuffer_[ch].empty()){auto& buf=combBuffer_[ch];size_t d=std::min(combDelay_,buf.size()-1);double delayed=buf[(combWrite_+buf.size()-d)%buf.size()];double v=shaped+(s_.combType==1?-delayed:delayed)*combFb_;buf[combWrite_]=float(std::clamp(v,-4.,4.));shaped=std::clamp(v,-4.,4.);}
   }
   double out=input[ch]+mix_*(shaped-input[ch]);if(ch==0)l=float(out);else r=float(out);}
  if(fade_)--fade_;if(next){if(++crusherCounter_>=crusherHold_)crusherCounter_=0;if(!combBuffer_[0].empty())combWrite_=(combWrite_+1)%combBuffer_[0].size();}if(next&&span_>2){cursor_+=speed_*pitchRatio_;if(cursor_>=span_-2){cursor_=std::fmod(cursor_,span_-2);crossfade();}}
 }
 int step()const{return step_;}bool active()const{return active_;}
 uint64_t events()const{return events_;}double nextCut()const{return nextCut_;}double speed()const{return speed_;}bool reversed()const{return reverse_;}
 size_t allocatedBytes()const{return (buffer_[0].capacity()+buffer_[1].capacity())*sizeof(float);}
};
}
