#pragma once
#include "master_fx.h"
#include <array>
#include <vector>
namespace qg {
inline constexpr const char* filterModels[]={"Classic (legacy)","Northern VA","Transistor Ladder","Acid Ladder","Notch","Resonant Peak","All-pass","Comb +","Comb -","Formant A","Formant E","Formant I","Formant O","Formant U","Phaser 4","Phaser 8","Low + High","Low + Band","Band + High"};
inline constexpr int filterModelCount=sizeof(filterModels)/sizeof(*filterModels);
// Original algorithms, not component-level or bit-exact models of branded filters.
class FilterBank {
 struct Svf {double a=0,b=0;std::array<double,3> run(double x,double g,double k){double h=1./(1.+g*(g+k)),v1=h*(a+g*(x-b)),v2=b+g*v1;a=2*v1-a;b=2*v2-b;if(std::abs(a)<1e-20)a=0;if(std::abs(b)<1e-20)b=0;return {v2,x-k*v1-v2,k*v1};}};
 struct Voice {
  int model=0;std::array<std::array<Svf,4>,2> svf{};double ladder[2][4]{},allpass[2][8]{},oldInput[2]{},feedback[2]{};
  std::array<std::vector<double>,2> delay;int write=0,valid=0;double combDelay=0.,oldCombDelay=0.;int combFade=0;
  void prepare(){for(auto& d:delay)d.assign(16384,0.);reset(0);}
  void reset(int m){model=m;svf={};for(int c=0;c<2;++c){for(auto& x:ladder[c])x=0;for(auto& x:allpass[c])x=0;oldInput[c]=feedback[c]=0;}write=valid=0;combDelay=oldCombDelay=0.;combFade=0;}
  void process(double* x,double sr,double fc,double res,double slope,int type,bool tuned){
   double g=std::tan(3.141592653589793*std::clamp(fc,20.,sr*.4)/sr),k=1./(.5+11.5*res);
   for(int c=0;c<2;++c){double input=x[c],out=input;
    if(model==1){auto y=svf[c][0].run(std::tanh(input*1.3)/1.3,g,std::max(.13,k));double first=y[type];auto z=svf[c][1].run(std::tanh(first*1.2)/1.2,g,std::max(.2,k));out=first+slope*(z[type]-first);}
    else if(model==2||model==3){
     // Four trapezoidal one-poles, implicit linear feedback solve, nonlinear drive.
     double a=g/(1.+g),a2=a*a,a3=a2*a,a4=a3*a;double sum=(1-a)*(a3*ladder[c][0]+a2*ladder[c][1]+a*ladder[c][2]+ladder[c][3]);
     double fb=res*(model==3?3.95:3.8),v=(input-fb*sum)/(1.+fb*a4);v=std::tanh(v*(model==3?2.:1.))/(model==3?2.:1.);double tap[4];
     for(int j=0;j<4;++j){double y=a*v+(1-a)*ladder[c][j];ladder[c][j]=2*y-ladder[c][j];v=y;tap[j]=y;}
     double lp=tap[1]+slope*(tap[3]-tap[1]);double hp=input-4*tap[0]+6*tap[1]-4*tap[2]+tap[3];double bp=2*(tap[1]-tap[3]);out=(type==0?lp:type==1?hp:bp)*(1.+res*.35);
    }else if(model>=7&&model<=8){
     double n=std::clamp(sr/(fc*(tuned&&model==8?2.:1.)),2.,16380.);
     if(c==0){if(combDelay==0.)combDelay=oldCombDelay=n;if(tuned&&std::abs(n-combDelay)>1e-6){oldCombDelay=combDelay;combDelay=n;combFade=std::max(1,int(sr*.005));}else if(!tuned)combDelay=n;}
     auto tap=[&](double len){double pos=write-len;while(pos<0)pos+=16384.;int a=int(pos),b=(a+1)&16383;return valid>int(len)+1?delay[c][a]+(delay[c][b]-delay[c][a])*(pos-a):0.;};
     double v=tap(n);if(combFade){double t=1.-combFade/double(std::max(1,int(sr*.005)));v=tap(oldCombDelay)*(1.-t)+v*t;}
     double sign=model==7?1.:-1.,fb=.92*res;
     delay[c][write]=input+sign*fb*std::tanh(v);out=.5*(input+sign*v);
    }else if(model>=9&&model<=13){
     constexpr double vowels[5][3]={{800,1150,2900},{400,1700,2600},{300,2200,3000},{450,800,2830},{325,700,2530}};out=0;
     for(int j=0;j<3;++j){double f=std::clamp(vowels[model-9][j]*(fc/1000.),20.,sr*.4);auto y=svf[c][j].run(input,std::tan(3.141592653589793*f/sr),1./(2.+18.*res));out+=y[2]*(j==0?1.:j==1?.7:.4);}
     out/=1.5;
    }else if(model==14||model==15){
     double y=input+.7*res*std::tanh(feedback[c]);int count=model==14?4:8;
     for(int j=0;j<count;++j){double f=std::clamp(fc*std::pow(1.35,j-(count-1)*.5),20.,sr*.4),t=std::tan(3.141592653589793*f/sr),a=(t-1.)/(t+1.);double v=a*y+allpass[c][j];allpass[c][j]=y-a*v;y=v;}
     feedback[c]=y;out=.5*(input+y);
    }else{
     auto y=svf[c][0].run(input,g,k);
     if(model==4)out=y[0]+y[1];else if(model==5)out=input+(1.+3.*res)*y[2];else if(model==6)out=input-2*y[2];
     else if(model==16){auto z=svf[c][1].run(input,std::tan(3.141592653589793*std::min(sr*.4,fc*2)/sr),k);out=.5*(y[0]+z[1]);}
     else if(model==17)out=.5*(y[0]+y[2]);else if(model==18)out=.5*(y[1]+y[2]);
    }
    x[c]=std::isfinite(out)?out:0.;
   }
   if(combFade)--combFade;
   if(model==7||model==8){write=(write+1)&16383;valid=std::min(valid+1,16384);}
  }
 };
 MasterFx classic_;Voice voices_[2];double sr_=48000,fc_=1000,targetFc_=1000,res_=0,targetRes_=0,drive_=0,targetDrive_=0,slope_=0,targetSlope_=0,blend_=0,smooth_=0;int type_=0,requested_=0,active_=0,fade_=0,fadeSize_=480;bool on_=false,tuned_=false;
public:
 void prepare(double sr){sr_=std::max(8000.,sr);classic_.prepare(sr_);for(auto& v:voices_)v.prepare();fc_=targetFc_;res_=targetRes_;drive_=targetDrive_;slope_=targetSlope_;blend_=0;active_=fade_=0;smooth_=std::exp(-1./(.005*sr_));fadeSize_=std::max(1,int(.01*sr_));}
 void set(bool on,int type,double fc,double res,bool limiter,int slope=0,double drive=0,double ceiling=0,int model=0,bool tuned=false){tuned_=tuned;classic_.set(on,type,fc,res,limiter,slope,drive,ceiling);on_=on;type_=std::clamp(type,0,2);targetFc_=std::clamp(fc,20.,std::min(20000.,sr_*.4));targetRes_=std::clamp(res,0.,1.);targetDrive_=std::clamp(drive,0.,24.);targetSlope_=slope?1.:0.;requested_=std::clamp(model,0,filterModelCount-1);}
 void process(float& l,float& r){
  float cl=l,cr=r;classic_.process(cl,cr);
  fc_=tuned_?targetFc_:targetFc_+smooth_*(fc_-targetFc_);res_=targetRes_+smooth_*(res_-targetRes_);drive_=targetDrive_+smooth_*(drive_-targetDrive_);slope_=targetSlope_+smooth_*(slope_-targetSlope_);blend_=(on_?1.:0.)+smooth_*(blend_-(on_?1.:0.));
  if(!fade_&&voices_[active_].model!=requested_){voices_[1-active_].reset(requested_);fade_=fadeSize_;}
  if(!fade_&&voices_[active_].model==0){l=cl;r=cr;return;}
  auto run=[&](Voice& v,double* out){if(v.model==0){out[0]=cl;out[1]=cr;return;}double d=std::pow(10.,drive_/20.);out[0]=drive_>1e-6?std::tanh(l*d)/std::tanh(d):l;out[1]=drive_>1e-6?std::tanh(r*d)/std::tanh(d):r;v.process(out,sr_,fc_,res_,slope_,type_,tuned_);out[0]=l+blend_*(out[0]-l);out[1]=r+blend_*(out[1]-r);};
  double out[2];run(voices_[active_],out);if(fade_){double next[2];run(voices_[1-active_],next);double t=1.-double(fade_)/fadeSize_;t=t*t*(3.-2*t);for(int c=0;c<2;++c)out[c]+=t*(next[c]-out[c]);if(!--fade_)active_=1-active_;}l=float(out[0]);r=float(out[1]);
 }
};
}
