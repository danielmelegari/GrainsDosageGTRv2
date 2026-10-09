// Draw the same scene as the native editors as an SVG for visual review.
// g++ -std=c++17 -O2 tools/mockup_preview.cpp -o /tmp/mockup-preview
// /tmp/mockup-preview > /tmp/mockup-preview.svg
#include "../src/mockup_ui.h"
#include "../src/factory_presets.h"
#include <iostream>
#include <fstream>
#include <sstream>
using namespace aztec;
static std::string escape(std::string s){std::string o;for(char c:s){if(c=='&')o+="&amp;";else if(c=='<')o+="&lt;";else if(c=='>')o+="&gt;";else o+=c;}return o;}
static std::string colour(skin::Rgb c){return "rgb("+std::to_string(c.r)+","+std::to_string(c.g)+","+std::to_string(c.b)+")";}
int main(int argc,char** argv){
 int tab=argc>1?std::atoi(argv[1]):0;std::ifstream image("assets/mockup/knob.png",std::ios::binary);std::string bytes((std::istreambuf_iterator<char>(image)),{}),base64;
 const char* alphabet="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
 unsigned buffer=0;int bits=0;for(unsigned char c:bytes){buffer=(buffer<<8)|c;bits+=8;while(bits>=6){bits-=6;base64+=alphabet[(buffer>>bits)&63];}}
 if(bits)base64+=alphabet[(buffer<<(6-bits))&63];while(base64.size()%4)base64+='=';
 auto state=factoryPreset(0);state[kUiTab]=tabToValue(tab);state[kXYX]=argc>3?std::atof(argv[3]):.5;state[kXYY]=argc>4?std::atof(argv[4]):.5;
 state[kUiGrainActive]=1;state[kUiGrainStart]=.25;state[kUiGrainEnd]=.4;state[kUiGrainHead]=.31;state[kUiLfoPhase0]=.3;state[kGaterEnabled]=1;state[kUiGaterStep]=5./15;state[kUiGaterPhase]=.45;for(int i=0;i<16;++i)state[kUiGaterLength0+i]=.2+(i%4)*.2;state[kReverbRateV2]=5./6.;state[kReverbSource]=1;state[kFilterSeqOn]=1;state[kUiFilterSeqStep]=7./31;state[kPitch]=state[kTranspose]=.5;state[kStretchSpeed]=.2;state[kGrainMix]=.5;
 // Representative incoming audio for this offline preview; the editor reads live monitor parameters.
 for(int i=0;i<128;++i)state[kUiWave0+i]=.12+.65*std::abs(std::sin(i*1.74)*std::cos(i*.183));
 for(int i=0;i<waveformBins;++i){double h=.12+.65*std::abs(std::sin(i*.87)*std::cos(i*.0915));state[kUiWaveLow0+i]=.5-h*.5;state[kUiWaveHigh0+i]=.5+h*.5;}
 mockup::Painter p;
 std::cout<<"<svg xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink' width='1083' height='1050' viewBox='0 0 1083 1050'>";
 const char* assets[]={"assets/approved-rack/backplate.png","assets/approved-rack/controls.png"};
 std::cout<<"<defs>";
 for(int n=0;n<2;++n){std::ifstream f(assets[n],std::ios::binary);std::string data((std::istreambuf_iterator<char>(f)),{}),encoded;unsigned acc=0;int bits=0;for(unsigned char c:data){acc=(acc<<8)|c;bits+=8;while(bits>=6){bits-=6;encoded+=alphabet[(acc>>bits)&63];}}if(bits)encoded+=alphabet[(acc<<(6-bits))&63];while(encoded.size()%4)encoded+='=';std::cout<<"<image id='art"<<n<<"' width='"<<(n?1254:1083)<<"' height='"<<(n?1254:1452)<<"' xlink:href='data:image/png;base64,"<<encoded<<"'/>";}
 std::cout<<"</defs>";
 p.image=[](int art,mockup::Rect r,mockup::Rect source){std::cout<<"<svg x='"<<r.x<<"' y='"<<r.y<<"' width='"<<r.w<<"' height='"<<r.h<<"' viewBox='"<<source.x<<" "<<source.y<<" "<<source.w<<" "<<source.h<<"' preserveAspectRatio='none'><use xlink:href='#art"<<art<<"'/></svg>";};
 p.box=[](mockup::Rect r,skin::Rgb fill,skin::Rgb edge,double radius){std::cout<<"<rect x='"<<r.x<<"' y='"<<r.y<<"' width='"<<r.w<<"' height='"<<r.h<<"' rx='"<<radius<<"' fill='"<<colour(fill)<<"' stroke='"<<colour(edge)<<"'/>";};
 p.text=[](const std::string& s,mockup::Rect r,double size,skin::Rgb c,bool center){std::cout<<"<text x='"<<(center?r.x+r.w/2:r.x)<<"' y='"<<r.y+r.h/2+size*.35<<"' font-family='DejaVu Sans' font-weight='500' font-size='"<<size<<"' text-anchor='"<<(center?"middle":"start")<<"' fill='"<<colour(c)<<"'>"<<escape(s)<<"</text>";};
 p.line=[](double x,double y,double xx,double yy,skin::Rgb c,double w){std::cout<<"<path d='M "<<x<<" "<<y<<" L "<<xx<<" "<<yy<<"' stroke='"<<colour(c)<<"' stroke-width='"<<w<<"'/>";};
 p.polygon=[](const std::vector<std::pair<double,double>>& pts,skin::Rgb c){std::cout<<"<polygon fill='"<<colour(c)<<"' points='";for(auto pt:pts)std::cout<<pt.first<<","<<pt.second<<" ";std::cout<<"'/>";};
 p.knob=[&](mockup::Rect r,double v){std::cout<<"<image x='"<<r.x<<"' y='"<<r.y<<"' width='"<<r.w<<"' height='"<<r.h<<"' transform='rotate("<<270*v-135<<" "<<r.x+r.w/2<<" "<<r.y+r.h/2<<")' xlink:href='data:image/png;base64,"<<base64<<"'/>";};
 auto display=[](ParamID id,double v)->std::string{
  if(id>=kSlotPolarity0&&id<kRoutingOrder){const char* names[]={"+/-","+","-"};return names[int(std::round(v*2))];}
  if(id==kReverbRateV2){const char* n[]={"PRESET","1/4","1/8","1/16","1/32","1/4D","1/8D"};return n[int(std::round(v*6))];}
  switch(id){case kSize:return "73.75 MS";case kDensity:return "4 X STEP";case kPosition:return "2000 MS";case kPitch:case kTranspose:return "0 ST";case kStretchSpeed:return "100%";
  case kGrainBuffer:return "4S";case kPanMode:return "MANUAL";case kFilterCutoff:return "253 HZ";case kFilterDrive:return "6.45 DB";case kFilterModel:return "NORTHERN";case kFilterType:return "HP";case kFilterSlope:return "12DB";
  case kFilterSeqMode:return "ARP";case kFilterSeqPattern:return "01 RISE 1";case kFilterSeqRate:return "1/16";case kReverbModel:return "PLATE";case kReverbType:return "ROOM";case kReverbSource:return "RANDOM IMP.";case kReverbRandomRate:case kReverbGrid:return "1/4";case kLimiterCeiling:return "-10DB";case kModuleOrder:return "G > P > R";case kXTarget:return "PITCH";case kYTarget:return "DENSITY";case kLfoSpeed0:return "0.25X";case kModWaveRnd0:return "1/2";case kRandomSteps0:return "16";}
  if(id==lfoID(0,lWave))return "125 RANDOM CURVE";if(id==lfoID(0,lGrid))return "4 BEATS";
  for(int i=0;i<6;++i)if(id==slotTarget(0,i)){const char* dest[]={"FILTER CUTOFF","TRANSPOSE","GRAIN SIZE","GRAIN MIX","PITCH","DENSITY"};return dest[i];}
  return std::to_string(int(v*100))+"%";
 };
 mockup::RackState rack;rack.page=std::clamp(tab,0,2);rack.scroll[rack.page]=argc>2?std::atof(argv[2]):0;int clipId=0;p.clip=[&](mockup::Rect r){std::cout<<"<defs><clipPath id='clip"<<++clipId<<"'><rect x='"<<r.x<<"' y='"<<r.y<<"' width='"<<r.w<<"' height='"<<r.h<<"'/></clipPath></defs><g clip-path='url(#clip"<<clipId<<")'>";};p.unclip=[](){std::cout<<"</g>";};
 mockup::render(p,[&](ParamID id){return state[id];},display,"PRESET..",0,0,0,0,0,nullptr,-1,-1,nullptr,mockup::RenderPass::All,rack);std::cout<<"</svg>";
}
