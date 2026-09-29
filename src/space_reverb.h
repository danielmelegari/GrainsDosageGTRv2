#pragma once
#include "step_reverb.h"
namespace qg {
// Additional original diffused/modulated FDNs; legacy network remains unchanged.
class SpaceReverb {
 struct Delay{std::vector<double> b;int pos=0;double low=0;double read(double samples)const{double x=pos-std::clamp(samples,1.,double(b.size()-2));while(x<0)x+=b.size();int i=int(x),j=(i+1)%int(b.size());return b[i]+(b[j]-b[i])*(x-i);}void push(double x){b[pos]=std::abs(x)<1e-20?0.:x;pos=(pos+1)%int(b.size());}};
 StepReverb legacy_;std::array<Delay,8> line_;std::array<std::array<Delay,3>,2> diff_;
 double sr_=48000.,phase_=0.,smooth_=0.,modelMix_=0.,send_=0.,amount_=0.,targetAmount_=0.,length_=2.8,scale_=1.,targetScale_=1.;bool on_=false;int model_=0;
public:
 void prepare(double sr){sr_=sr;legacy_.prepare(sr);smooth_=std::exp(-1./(.02*sr));for(auto& l:line_){l.b.assign(size_t(.5*sr)+4,0.);l.pos=0;l.low=0;}for(int c=0;c<2;++c)for(int j=0;j<3;++j){auto& d=diff_[c][j];d.b.assign(size_t((.0047+.0031*j+.0007*c)*sr)+1,0.);d.pos=0;}phase_=modelMix_=send_=amount_=0;scale_=targetScale_;}
 void reset(){legacy_.reset();for(auto& l:line_){std::fill(l.b.begin(),l.b.end(),0.);l.pos=0;l.low=0;}for(auto& channel:diff_)for(auto& d:channel){std::fill(d.b.begin(),d.b.end(),0.);d.pos=0;}modelMix_=send_=amount_=0;}
 void set(bool on,int type,double grid,uint16_t pattern,double amount,double length=0,bool kill=false,bool random=false,double randomGrid=.25,int model=0){legacy_.set(on,type,grid,pattern,amount,length,kill,random,randomGrid);model_=std::clamp(model,0,4);on_=on;targetAmount_=amount;length_=std::clamp(length>0?length:2.8,.2,20.);targetScale_=model_==1?.55:model_==2?2.2:model_==3?2.8:3.3;}
 bool gateOpen()const{return legacy_.gateOpen();}double killAmount()const{return legacy_.killAmount();}
 void process(float& l,float& r,double beat){float cl=l,cr=r;legacy_.process(cl,cr,beat);double target=model_?1.:0.;modelMix_=target+smooth_*(modelMix_-target);if(model_==0&&modelMix_<1e-12){l=cl;r=cr;return;}
  send_=(gateOpen()?1.:0.)+smooth_*(send_-(gateOpen()?1.:0.));amount_=(on_?targetAmount_:0.)+smooth_*(amount_-(on_?targetAmount_:0.));scale_=targetScale_+smooth_*(scale_-targetScale_);phase_+=.13/sr_;if(phase_>=1)phase_-=1;
  double input[2]={l*send_,r*send_};for(int c=0;c<2;++c)for(auto& d:diff_[c]){double delayed=d.b[d.pos],out=delayed-.65*input[c];d.push(input[c]+.65*out);input[c]=out;}
  constexpr double times[]={.0311,.0379,.0437,.0533,.0617,.0713,.0839,.0971};double v[8],sum=0;
  for(int i=0;i<8;++i){auto& d=line_[i];double modulation=model_==1?0.:sr_*.0004*std::sin(6.283185307179586*(phase_+i*.137));double x=d.read(sr_*times[i]*scale_+modulation);double damp=model_==1?.16:model_==3?.87:.55;d.low=damp*d.low+(1-damp)*x;v[i]=d.low;sum+=v[i];}
  for(int i=0;i<8;++i){double fb=std::pow(.001,times[i]*scale_/length_);line_[i].push(input[i%2]*.3*(i<4?1.:-1.)+fb*(sum*.25-v[i]));}
  double wetL=(v[0]+v[2]-v[4]-v[6])*.5,wetR=(v[1]+v[3]-v[5]-v[7])*.5;
  double nl=l*(1-killAmount())+wetL*amount_,nr=r*(1-killAmount())+wetR*amount_;l=float(cl+modelMix_*(nl-cl));r=float(cr+modelMix_*(nr-cr));
 }
};
}
