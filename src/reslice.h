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
};
class Reslice {
 std::array<std::vector<float>,2> buffer_;ResliceSettings s_;double sr_=48000.;int64_t write_=0;
 double lastBeat_=0.,nextCut_=0.,tempo_=120.,start_=0.,span_=0.,cursor_=0.,speed_=1.,mix_=0.;
 bool reverse_=false,playing_=false,active_=false,pending_=true;int step_=0,fade_=0,fadeSize_=24;
 std::array<int,16> pattern_{};double lastWet_[2]{},from_[2]{};uint32_t rng_=1;uint64_t events_=0;
 double random(){rng_^=rng_<<13;rng_^=rng_>>17;rng_^=rng_<<5;return (rng_&0xffffff)/16777216.;}
 double read(int ch,double position)const{int64_t a=int64_t(std::floor(position));if(a<0||a<write_-int64_t(buffer_[ch].size())||a+1>=write_)return 0.;size_t i=size_t(a)%buffer_[ch].size(),j=(i+1)%buffer_[ch].size();return buffer_[ch][i]+(buffer_[ch][j]-buffer_[ch][i])*(position-a);}
 void crossfade(){for(int ch=0;ch<2;++ch)from_[ch]=lastWet_[ch];fade_=fadeSize_;}
 void choose(double beat,double samplesPerBeat){
  const double bar=std::clamp(s_.barBeats,.25,32.),phrase=std::clamp(s_.phraseBeats,bar,bar*8);double phrasePhase=beat-std::floor(beat/phrase)*phrase;
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
  // Every bar and phrase begins on the host grid, even after tempo/rate changes.
  duration=std::max(1e-7,std::min(std::max(1./48.,duration),bar-barPhase));nextCut_=beat+duration;
  double history=std::min({double(write_-2),std::clamp(s_.beats,2.,16.)*samplesPerBeat,double(buffer_[0].size())-4});
  double requestedSpan=std::max(32.,duration*samplesPerBeat);speed_=1.;
  if(mode==1&&random()<variation){constexpr double ratios[]={.5,.75,1.,1.5,2.};speed_=ratios[int(random()*5)];}
  bool repeat=events_>0&&random()<std::clamp(s_.repeat,0.,1.);bool anchor=barPhase<1e-7;
  if(!repeat||anchor||span_<32||start_<double(write_-2)-history){
   span_=std::min(requestedSpan,history);double depth=std::max(0.,history-span_);
   double back=(mode==0&&anchor)?std::max(0.,std::min(depth,bar*samplesPerBeat-span_)):random()*depth*variation;
   double grid=samplesPerBeat*(mode==1?1./16.:.25);back=std::floor(back/std::max(1.,grid))*grid;
   start_=double(write_-2)-span_-back;
  }else span_=std::min(span_,requestedSpan);
  cursor_=0;reverse_=random()<std::clamp(s_.reverse,0.,1.);crossfade();++events_;
  int source=int(std::clamp((double(write_-2)-start_)/std::max(1.,history),0.,.999)*16);pattern_[step_]=source;
 }
public:
 void prepare(double sr){sr_=std::max(8000.,sr);for(auto& b:buffer_)b.assign(size_t(sr_*96.)+64,0.f);fadeSize_=std::max(8,int(sr_*.003));lastWet_[0]=lastWet_[1]=from_[0]=from_[1]=0;mix_=0;reset();}
 void reset(){write_=0;pending_=true;active_=false;span_=cursor_=0;events_=0;rng_=s_.seed?s_.seed:1;pattern_.fill(0);fade_=0;}
 void set(const ResliceSettings& s){if(s.algorithm!=s_.algorithm||s.seed!=s_.seed||s.phraseBeats!=s_.phraseBeats||s.barBeats!=s_.barBeats){pending_=true;rng_=s.seed?s.seed:1;}if(s.enabled&&!s_.enabled)pending_=true;s_=s;}
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
  for(int ch=0;ch<2;++ch){double wet=input[ch];if(span_>2){double phase=std::min(cursor_,span_-2);wet=read(ch,start_+(reverse_?span_-2-phase:phase));if(fade_){double t=1.-double(fade_)/fadeSize_;t=t*t*(3.-2*t);wet=from_[ch]+t*(wet-from_[ch]);}}lastWet_[ch]=wet;double out=input[ch]+mix_*(wet-input[ch]);if(ch==0)l=float(out);else r=float(out);}
  if(fade_)--fade_;if(next&&span_>2){cursor_+=speed_;if(cursor_>=span_-2){cursor_=std::fmod(cursor_,span_-2);crossfade();}}
 }
 int step()const{return step_;}bool active()const{return active_;}
 uint64_t events()const{return events_;}double nextCut()const{return nextCut_;}double speed()const{return speed_;}bool reversed()const{return reverse_;}
 size_t allocatedBytes()const{return (buffer_[0].capacity()+buffer_[1].capacity())*sizeof(float);}
};
}
