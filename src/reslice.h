#pragma once
#include <array>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace qg {
// Original procedural cutters inspired by breakbeat cutting; not LiveCut source.
struct ResliceSettings {
 bool enabled=false,random=false;double randomBeats=4.,beats=4.,mix=1.;
 std::array<bool,16> on{{true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true}};
 std::array<int,16> slice{{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}};
 int algorithm=0;double barBeats=4.,phraseBeats=16.,repeat=.45,variation=.5,fill=.5,reverse=.1;uint32_t seed=1;
 int subdivision=8,minRepeats=0,maxRepeats=0;
 double fadeMs=3.,minAmp=1.,maxAmp=1.,minPan=0.,maxPan=0.,minPitch=0.,maxPitch=0.,duty=1.,fillDuty=1.,minPhraseBeats=0.;
 double stutter=0.,area=1.,straight=.5,regular=0.,ritard=0.,warpSpeed=.5,activity=1.;
 bool crusher=false,comb=false;int minBits=32,maxBits=32,combType=0;
 double minFreq=44100.,maxFreq=44100.,combFeedback=.5,minDelay=10.,maxDelay=10.;
};
class Reslice {
 std::array<std::vector<float>,2> buffer_,combBuffer_;size_t mask_=0,combMask_=0,combWrite_=0,combValid_=0;
 ResliceSettings s_;double sr_=48000.;int64_t write_=0;
 double lastBeat_=0.,nextCut_=0.,tempo_=0.,spb_=24000.,bar_=4.,start_=0.,span_=0.,cursor_=0.,speed_=1.,mix_=0.;
 double invBar_=.25,beatJump_=4./24000.,mixStep_=1./144.,invFade_=1./144.;
 double phraseStart_=0.,phraseEnd_=0.,elapsed_=0.,cutSamples_=1.,cutDuty_=1.,gain_[2]{1,1};
 double crusherPhase_=1.,crusherRate_=1.,quantize_=0.,held_[2]{},delay_=480.,feedback_=.5;
 bool reverse_=false,playing_=false,active_=false,pending_=true;int step_=0,fade_=0,fadeSize_=144,mixFade_=144,repeatsLeft_=0;
 std::array<int,16> pattern_{};double lastWet_[2]{},from_[2]{};uint32_t rng_=1;uint64_t events_=0;
 double random(){rng_^=rng_<<13;rng_^=rng_>>17;rng_^=rng_<<5;return (rng_&0xffffff)/16777216.;}
 double between(double a,double b){if(a>b)std::swap(a,b);return a==b?a:a+random()*(b-a);}
 int integer(int a,int b){if(a>b)std::swap(a,b);return a==b?a:a+std::min(b-a,int(random()*(b-a+1)));}
 static size_t capacity(double samples){size_t n=1;while(n<size_t(samples))n<<=1;return n;}
 void crossfade(){for(int ch=0;ch<2;++ch)from_[ch]=lastWet_[ch];fade_=fadeSize_;}
 void choose(double beat){
  const double bar=bar_;double barPhase=beat-std::floor(beat/bar)*bar;
  if(phraseEnd_<=phraseStart_||beat+1e-7>=phraseEnd_||beat<phraseStart_){
   int hi=std::clamp(int(std::round(s_.phraseBeats/bar)),1,8),lo=s_.minPhraseBeats>0?std::clamp(int(std::round(s_.minPhraseBeats/bar)),1,8):hi;
   phraseStart_=std::floor((beat+1e-8)/bar)*bar;phraseEnd_=phraseStart_+integer(lo,hi)*bar;
  }
  const double phrase=phraseEnd_-phraseStart_,phase=beat-phraseStart_;
  bool fill=phraseEnd_-beat<=1.+1e-7&&random()<std::clamp(s_.fill,0.,1.);
  double variation=std::clamp(s_.variation,0.,1.),duration=.5;int mode=std::clamp(s_.algorithm,0,2);
  if(mode==0){
   duration=fill?(random()<.5?.0625:.125):(random()<variation?.25:(random()<.65?.5:1.));
   if(random()<std::clamp(s_.stutter,0.,1.))duration*=.5;
  }else if(mode==1){
   constexpr double straightLengths[]={.125,.25,.5},irregularLengths[]={1./12.,.1875,.375};
   duration=variation>.2?(random()<s_.straight?straightLengths[integer(0,2)]:irregularLengths[integer(0,2)]):.5;
   if(random()<std::clamp(s_.regular,0.,1.))duration=.5;
   if(fill)duration*=.5;duration*=1.+std::clamp(s_.ritard,0.,1.)*phase/std::max(bar,phrase)*3.;
  }else{
   constexpr double lengths[]={.75,.25,.5,.25,.25,.5,.25,.75,.5};double boundary=0;
   for(double length:lengths){boundary+=length;if(boundary>std::fmod(barPhase,4.)+1e-7){duration=boundary-std::fmod(barPhase,4.);break;}}
   if(random()<variation*.5*std::clamp(s_.activity,0.,1.))duration=std::min(duration,.125);
   if(fill&&random()<std::clamp(s_.activity,0.,1.))duration=std::min(duration,random()<.5?.0625:.125);
  }
  duration*=8./std::clamp(s_.subdivision,1,32);
  duration=std::max(1e-7,std::min(std::max(1./192.,duration),bar-barPhase));nextCut_=beat+duration;
  cutSamples_=duration*spb_;elapsed_=0;cutDuty_=std::clamp(fill?s_.fillDuty:s_.duty,0.,1.);
  double history=std::min({double(write_-2),std::clamp(s_.beats,2.,16.)*spb_,double(buffer_[0].size())-4});
  double requestedSpan=std::max(32.,cutSamples_);speed_=1.;
  if(mode==1){if(random()<variation){constexpr double ratios[]={.5,.75,1.,1.5,2.};speed_=ratios[integer(0,4)];}speed_*=std::exp2((std::clamp(s_.warpSpeed,0.,1.)-.5)*4.);}
  speed_*=std::exp2(between(std::clamp(s_.minPitch,-2400.,2400.),std::clamp(s_.maxPitch,-2400.,2400.))/1200.);
  const bool forced=repeatsLeft_>0;bool repeat=events_>0&&(forced||random()<std::clamp(s_.repeat,0.,1.));
  if(forced)--repeatsLeft_;else repeatsLeft_=integer(std::clamp(s_.minRepeats,0,16),std::clamp(s_.maxRepeats,0,16));
  bool anchor=barPhase<1e-7;
  if(!repeat||anchor||span_<32||start_<double(write_-2)-history){
   span_=std::min(requestedSpan,history);double depth=std::max(0.,history-span_);if(mode==0)depth*=std::clamp(s_.area,0.,1.);
   double back=(mode==0&&anchor)?std::max(0.,std::min(depth,bar*spb_-span_)):random()*depth*variation;
   double grid=spb_*(mode==1?1./16.:.25);back=std::floor(back/std::max(1.,grid))*grid;start_=double(write_-2)-span_-back;
  }else span_=std::min(span_,requestedSpan);
  double amp=between(std::clamp(s_.minAmp,0.,2.),std::clamp(s_.maxAmp,0.,2.));
  double pan=between(std::clamp(s_.minPan,-1.,1.),std::clamp(s_.maxPan,-1.,1.));gain_[0]=amp*(pan>0?1.-pan:1.);gain_[1]=amp*(pan<0?1.+pan:1.);
  if(s_.crusher){int bits=integer(std::clamp(s_.minBits,1,32),std::clamp(s_.maxBits,1,32));quantize_=bits==32?0.:std::exp2(bits-1.);crusherRate_=std::clamp(between(s_.minFreq,s_.maxFreq)/sr_,100./sr_,1.);crusherPhase_=1.;}
  if(s_.comb)delay_=std::clamp(between(s_.minDelay,s_.maxDelay)*sr_*.001,1.,double(combMask_));
  cursor_=0;reverse_=random()<std::clamp(s_.reverse,0.,1.);crossfade();++events_;
  pattern_[step_]=int(std::clamp((double(write_-2)-start_)/std::max(1.,history),0.,.999)*16);
 }
public:
 void prepare(double sr){sr_=std::max(8000.,sr);size_t n=capacity(sr_*48.+64),cn=capacity(sr_*.1+4);mask_=n-1;combMask_=cn-1;
  for(auto& b:buffer_)b.assign(n,0.f);for(auto& b:combBuffer_)b.assign(cn,0.f);
  mixFade_=std::max(8,int(sr_*.003));fadeSize_=int(sr_*std::clamp(s_.fadeMs,0.,50.)*.001);mixStep_=1./mixFade_;invFade_=fadeSize_?1./fadeSize_:0.;lastWet_[0]=lastWet_[1]=from_[0]=from_[1]=0;mix_=0;tempo_=0;reset();}
 void reset(){write_=0;pending_=true;active_=false;span_=cursor_=0;events_=0;rng_=s_.seed?s_.seed:1;pattern_.fill(0);fade_=0;phraseStart_=phraseEnd_=0;combValid_=combWrite_=0;repeatsLeft_=0;}
 void set(const ResliceSettings& s){
  if(s.algorithm!=s_.algorithm||s.seed!=s_.seed||s.phraseBeats!=s_.phraseBeats||s.minPhraseBeats!=s_.minPhraseBeats||s.barBeats!=s_.barBeats){pending_=true;rng_=s.seed?s.seed:1;phraseStart_=phraseEnd_=0;}
  if(s.enabled&&!s_.enabled)pending_=true;
  if(s.crusher!=s_.crusher||s.comb!=s_.comb){pending_=true;combValid_=0;}
  if(s.fadeMs!=s_.fadeMs){fadeSize_=int(sr_*std::clamp(s.fadeMs,0.,50.)*.001);fade_=std::min(fade_,fadeSize_);invFade_=fadeSize_?1./fadeSize_:0.;}
  bar_=std::clamp(s.barBeats,.25,32.);invBar_=1./bar_;feedback_=std::clamp(s.combFeedback,0.,.95);s_=s;
 }
 int source(int i)const{return pattern_[std::clamp(i,0,15)];}
 void process(float& l,float& r,double beat,double tempo,bool playing=true){
  if(!std::isfinite(beat)){beat=0;playing=false;}tempo=std::isfinite(tempo)?std::clamp(tempo,20.,400.):120.;
  if(tempo!=tempo_){tempo_=tempo;spb_=sr_*60./tempo;beatJump_=4./spb_;pending_=true;}
  bool jump=playing_&&playing&&(beat<lastBeat_-1e-6||beat-lastBeat_>beatJump_);
  if(jump||playing!=playing_)reset();lastBeat_=beat;playing_=playing;
  const double input[2]={l,r};size_t w=size_t(write_)&mask_;buffer_[0][w]=l;buffer_[1][w]=r;++write_;
  bool next=s_.enabled&&playing&&write_>std::max(64.,spb_*.125);active_=next;
  if(!next&&mix_==0.)return;
  step_=int((int64_t(std::floor(beat*invBar_*16+1e-8))%16+16)%16);
  if(next&&(pending_||beat+1e-8>=nextCut_)){choose(pending_?beat:nextCut_);pending_=false;}
  double target=next?std::clamp(s_.mix,0.,1.):0.;if(mix_!=target)mix_+=std::clamp(target-mix_,-mixStep_,mixStep_);
  double position=start_+(reverse_?span_-2-std::min(cursor_,span_-2):std::min(cursor_,span_-2));
  int64_t absolute=int64_t(std::floor(position));size_t a=size_t(absolute)&mask_,b=(a+1)&mask_;double fraction=position-absolute;
  bool valid=span_>2&&absolute>=0&&absolute>=write_-int64_t(buffer_[0].size())&&absolute+1<write_;
  double gate=elapsed_<cutSamples_*cutDuty_?1.:0.;if(gate&&fadeSize_>0&&elapsed_>cutSamples_*cutDuty_-fadeSize_)gate=std::max(0.,(cutSamples_*cutDuty_-elapsed_)*invFade_);
  bool capture=false;if(s_.crusher){crusherPhase_+=crusherRate_;if(crusherPhase_>=1.){crusherPhase_-=std::floor(crusherPhase_);capture=true;}}
  size_t d0=0,d1=0;double df=0;if(s_.comb){d0=(combWrite_-size_t(delay_))&combMask_;d1=(d0-1)&combMask_;df=delay_-std::floor(delay_);}
  double blend=fade_&&fadeSize_?1.-double(fade_)*invFade_:1.;blend=blend*blend*(3.-2*blend);
  for(int ch=0;ch<2;++ch){
   double wet=valid?(buffer_[ch][a]+(buffer_[ch][b]-buffer_[ch][a])*fraction)*gain_[ch]*gate:0.;
   if(s_.crusher){if(capture)held_[ch]=quantize_?std::round(std::clamp(wet,-1.,1.)*quantize_)/quantize_:wet;wet=held_[ch];}
   if(s_.comb){double delayed=combValid_>size_t(delay_)+1?combBuffer_[ch][d0]+(combBuffer_[ch][d1]-combBuffer_[ch][d0])*df:0.;
    double v=s_.combType?(1.-feedback_)*wet+feedback_*delayed:(wet+feedback_*delayed)/(1.+feedback_);
    double store=s_.combType?v:wet;combBuffer_[ch][combWrite_]=float(std::abs(store)<1e-20?0.:store);wet=v;}
   if(fade_)wet=from_[ch]+blend*(wet-from_[ch]);lastWet_[ch]=wet;
   double out=input[ch]+mix_*(wet-input[ch]);if(ch==0)l=float(out);else r=float(out);
  }
  if(s_.comb){combWrite_=(combWrite_+1)&combMask_;combValid_=std::min(combValid_+1,combMask_);}
  ++elapsed_;if(fade_)--fade_;if(next&&span_>2){cursor_+=speed_;if(cursor_>=span_-2){cursor_=std::fmod(cursor_,span_-2);crossfade();}}
 }
 int step()const{return step_;}bool active()const{return active_;}
 uint64_t events()const{return events_;}double nextCut()const{return nextCut_;}double speed()const{return speed_;}bool reversed()const{return reverse_;}
 size_t allocatedBytes()const{return (buffer_[0].capacity()+buffer_[1].capacity()+combBuffer_[0].capacity()+combBuffer_[1].capacity())*sizeof(float);}
};
}
