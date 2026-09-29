#include "src/declick.h"
#include <cassert>
#include <vector>
#include <cmath>
#include <iostream>
int main(){
 for(double sr:{44100.,48000.,96000.,192000.}){
  const int n=12000;std::vector<double> clean(n),damaged(n),out(n+32),right(n+32);
  for(int i=0;i<n;++i)clean[i]=.3*std::sin(6.283185307179586*220*i/sr);damaged=clean;
  for(int width:{1,4,8,16})for(int j=0;j<width;++j)damaged[2000+width*300+j]+=1.;
  qg::InputDeclick d;d.prepare(sr,true);double originalError=0,repairedError=0;
  for(int i=0;i<n+32;++i){double l=i<n?damaged[i]:0,r=i<n?clean[i]:0;d.process(l,r,true,.5);out[i]=l;right[i]=r;}
  for(int i=1000;i<n-100;++i){originalError+=std::pow(damaged[i]-clean[i],2);repairedError+=std::pow(out[i+32]-clean[i],2);assert(std::abs(right[i+32]-clean[i])<1e-12);}
  assert(repairedError<originalError*.01);
 }
 // Disabled and global bypass are exact delayed copies, also in double precision.
 for(bool bypass:{false,true}){qg::InputDeclick d;d.prepare(48000,!bypass);std::vector<double> history;for(int i=0;i<3000;++i){double v=std::sin(i*1.791)*.72345678912345;history.push_back(v);double l=v,r=-v;d.process(l,r,bypass,.9,bypass);if(i>600){assert(l==history[i-32]);assert(r==-history[i-32]);}}}
 // A sustained edge and a kick-like decaying transient must not be repaired.
 for(int kind=0;kind<2;++kind){qg::InputDeclick d;d.prepare(48000,true);std::vector<double> h;for(int i=0;i<5000;++i){double v=i<1000?0:kind==0?.6:.8*std::exp(-(i-1000)/3000.)*std::cos((i-1000)*.008);h.push_back(v);double l=v,r=v;d.process(l,r,true,.5);if(i>=32)assert(l==h[i-32]);}}
 std::cout<<"PASS: short impulse repair at four sample rates, clean stereo channel, exact delayed bypass, sustained edges and kick transient\n";
}
