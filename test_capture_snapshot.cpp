#include "src/rhythmic_loop.h"
#include "tests/reference_rhythmic_loop.h"
#include <cassert>
#include <chrono>
#include <iostream>
#include <algorithm>

int main(){
 // Two physical wraps after each capture exercise both the ring-backed and
 // preserved portions, plus offsets, reverse/pitch and changing held length.
 for(double sr:{8000.,44100.,96000.}){
  qg::RhythmicLoop current;reference::RhythmicLoop prior;current.prepare(sr);prior.prepare(sr);
  for(int n=0;n<int(sr*65);++n){
   bool request=n>int(sr*2)&&n<int(sr*62);
   bool recapture=n==int(sr*20)||n==int(sr*40);
   double length=n<int(sr*25)?sr*1.7:n<int(sr*28)?sr*.7:n<int(sr*40)?sr*1.6:sr*15.9;
   double offset=n<int(sr*40)?sr*.23:0.;
   double pitch=n%17000<8500?-12.:7.;bool reverse=n%11000<5500;
   float l=float(std::sin(n*.013)*.2),r=float(std::cos(n*.017)*.3),a=l,b=r;
   current.process(l,r,request,true,length,pitch,reverse,.81,offset,recapture);
   prior.process(a,b,request,true,length,pitch,reverse,.81,offset,recapture);
   assert(l==a&&r==b);assert(current.active()==prior.active());
   if(n==int(sr*63)){current.reset();prior.reset();}
  }
 }
 // Timing is informational, never a flaky wall-clock test threshold.
 auto measure=[](auto& loop){
  loop.prepare(96000.);for(int n=0;n<1600000;++n){float l=.2f,r=.3f;loop.process(l,r,false,true,1535998,0,false,1.);}
  double total=0,maximum=0;for(int n=0;n<200;++n){float l=.4f,r=.5f;auto start=std::chrono::steady_clock::now();
   loop.process(l,r,true,true,1535998,0,false,1.,0.,true);
   double us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();total+=us;maximum=std::max(maximum,us);
  }std::cout<<"capture mean_us="<<total/200<<" max_us="<<maximum<<'\n';
 };
 reference::RhythmicLoop prior;qg::RhythmicLoop current;
 std::cout<<"Before ";measure(prior);std::cout<<"After ";measure(current);
 std::cout<<"PASS: bit-identical held audio across wrap, recapture, offset, length, pitch, reverse and reset\n";
}
