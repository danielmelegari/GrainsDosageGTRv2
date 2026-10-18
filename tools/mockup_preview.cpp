// Draw the same scene as the native editors as an SVG for visual review.
// g++ -std=c++17 -O2 tools/mockup_preview.cpp -o /tmp/mockup-preview
// /tmp/mockup-preview > /tmp/mockup-preview.svg
#include "../src/mockup_ui.h"
#include "../src/preset_banks.h"
#include <iostream>
#include <fstream>
#include <sstream>
using namespace aztec;
static std::string escape(std::string s){std::string o;for(char c:s){if(c=='&')o+="&amp;";else if(c=='<')o+="&lt;";else if(c=='>')o+="&gt;";else o+=c;}return o;}
static std::string colour(skin::Rgb c){return "rgb("+std::to_string(c.r)+","+std::to_string(c.g)+","+std::to_string(c.b)+")";}
int main(int argc,char** argv){
 int selectedMod=argc>9?std::clamp(std::atoi(argv[9]),0,4):0;
 int tab=argc>1?std::atoi(argv[1]):0;std::ifstream image("assets/mockup/knob.png",std::ios::binary);std::string bytes((std::istreambuf_iterator<char>(image)),{}),base64;
 const char* alphabet="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
 unsigned buffer=0;int bits=0;for(unsigned char c:bytes){buffer=(buffer<<8)|c;bits+=8;while(bits>=6){bits-=6;base64+=alphabet[(buffer>>bits)&63];}}
 if(bits)base64+=alphabet[(buffer<<(6-bits))&63];while(base64.size()%4)base64+='=';
 auto state=bankPreset(1,0);state[kUiTab]=tabToValue(tab);state[kXYX]=argc>3?std::atof(argv[3]):.5;state[kXYY]=argc>4?std::atof(argv[4]):.5;
 if(argc>6)state[kResliceAlgorithm]=std::clamp(std::atoi(argv[6]),0,2)*.5;
 state[kUiGrainActive]=1;state[kUiGrainStart]=.25;state[kUiGrainEnd]=.4;state[kUiGrainHead]=.31;state[kUiLfoPhase0]=.3;state[kGaterEnabled]=1;state[kUiGaterStep]=5./15;state[kUiGaterPhase]=.45;for(int i=0;i<16;++i)state[kUiGaterLength0+i]=.2+(i%4)*.2;state[kReverbRateV2]=5./6.;state[kReverbSource]=1;state[kFilterSeqOn]=1;state[kUiFilterSeqStep]=7./31;state[kPitch]=state[kTranspose]=.5;state[kStretchSpeed]=.2;state[kGrainMix]=.5;
 // Optional final argument previews live modulation without an audio host.
 if(argc>8&&std::atoi(argv[8])){state[kReverbMix]=.6;state[kUiModulationActive]=1.;state[kUiLfoValue0]=.85;state[lfoID(0,lEnabled)]=1.;state[lfoID(0,lDepth)]=1.;
  const int ids[]={kSize,kPitch,kFilterCutoff,kReverbMix,kResliceFade,kResliceVariation};
  for(int slot=0;slot<6;++slot)for(int t=0;t<qg::modTargetCount;++t)if(modulationParameter(t)==ids[slot]){state[slotTarget(0,slot)]=(t+1.)/qg::modTargetCount;state[slotAmount(0,slot)]=slot%2?.2:.8;state[slotPolarity(0,slot)]=0.;}
 }
 // Representative incoming audio for this offline preview; the editor reads live monitor parameters.
 for(int i=0;i<128;++i)state[kUiWave0+i]=.12+.65*std::abs(std::sin(i*1.74)*std::cos(i*.183));
 for(int i=0;i<waveformBins;++i){double h=.12+.65*std::abs(std::sin(i*.87)*std::cos(i*.0915));state[kUiWaveLow0+i]=.5-h*.5;state[kUiWaveHigh0+i]=.5+h*.5;}
 mockup::Painter p;
 std::cout<<"<svg xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink' width='1083' height='1050' viewBox='0 0 1083 1050'>";
 const char* assets[]={"assets/approved-rack/backplate.png","assets/approved-rack/controls.png","assets/approved-rack/logo.png","assets/approved-rack/cables.png"};
 std::cout<<"<defs>";
 for(int n=0;n<4;++n){std::ifstream f(assets[n],std::ios::binary);std::string data((std::istreambuf_iterator<char>(f)),{}),encoded;unsigned acc=0;int bits=0;for(unsigned char c:data){acc=(acc<<8)|c;bits+=8;while(bits>=6){bits-=6;encoded+=alphabet[(acc>>bits)&63];}}if(bits)encoded+=alphabet[(acc<<(6-bits))&63];while(encoded.size()%4)encoded+='=';std::cout<<"<image id='art"<<(n==3?int(mockup::Cables):n==2?int(mockup::Logo):n)<<"' width='"<<(n==3?1199:n==2?2172:n?1254:1083)<<"' height='"<<(n==3?1312:n==2?724:n?1254:1452)<<"' xlink:href='data:image/png;base64,"<<encoded<<"'/>";}
 std::cout<<"</defs>";
 p.image=[](int art,mockup::Rect r,mockup::Rect source){if(art==mockup::Cables)source={0,0,1199,1312};static int screwId=0;bool screw=art==mockup::RoundScrew;if(screw){art=mockup::Backplate;std::cout<<"<defs><clipPath id='screw"<<++screwId<<"'><ellipse cx='"<<r.x+r.w/2<<"' cy='"<<r.y+r.h/2<<"' rx='"<<r.w/2<<"' ry='"<<r.h/2<<"'/></clipPath></defs><g clip-path='url(#screw"<<screwId<<")'>";}std::cout<<"<svg x='"<<r.x<<"' y='"<<r.y<<"' width='"<<r.w<<"' height='"<<r.h<<"' viewBox='"<<source.x<<" "<<source.y<<" "<<source.w<<" "<<source.h<<"' preserveAspectRatio='none'><use xlink:href='#art"<<art<<"'/></svg>";if(screw)std::cout<<"</g>";};
 p.box=[](mockup::Rect r,skin::Rgb fill,skin::Rgb edge,double radius){std::cout<<"<rect x='"<<r.x<<"' y='"<<r.y<<"' width='"<<r.w<<"' height='"<<r.h<<"' rx='"<<radius<<"' fill='"<<colour(fill)<<"' stroke='"<<colour(edge)<<"'/>";};
 p.text=[](const std::string& s,mockup::Rect r,double size,skin::Rgb c,bool center){std::cout<<"<text x='"<<(center?r.x+r.w/2:r.x)<<"' y='"<<r.y+r.h/2+size*.35<<"' font-family='DejaVu Sans' font-weight='500' font-size='"<<size<<"' text-anchor='"<<(center?"middle":"start")<<"' fill='"<<colour(c)<<"'>"<<escape(s)<<"</text>";};
 p.line=[](double x,double y,double xx,double yy,skin::Rgb c,double w){std::cout<<"<path d='M "<<x<<" "<<y<<" L "<<xx<<" "<<yy<<"' stroke='"<<colour(c)<<"' stroke-width='"<<w<<"' stroke-linecap='round'/>";};
 p.polyline=[](const std::vector<std::pair<double,double>>& pts,skin::Rgb c,double width){std::cout<<"<polyline fill='none' stroke='"<<colour(c)<<"' stroke-width='"<<width<<"' stroke-linecap='round' stroke-linejoin='round' points='";for(auto pt:pts)std::cout<<pt.first<<","<<pt.second<<" ";std::cout<<"'/>";};
 p.polygon=[](const std::vector<std::pair<double,double>>& pts,skin::Rgb c){std::cout<<"<polygon fill='"<<colour(c)<<"' points='";for(auto pt:pts)std::cout<<pt.first<<","<<pt.second<<" ";std::cout<<"'/>";};
 p.knob=[&](mockup::Rect r,double v){std::cout<<"<image x='"<<r.x<<"' y='"<<r.y<<"' width='"<<r.w<<"' height='"<<r.h<<"' transform='rotate("<<270*v-135<<" "<<r.x+r.w/2<<" "<<r.y+r.h/2<<")' xlink:href='data:image/png;base64,"<<base64<<"'/>";};
 p.moduleBackground=[&](int module,mockup::Rect r){const char* names[]={"granulizer","retired","beatrepeater","reslice","gater","input-options"};std::ifstream f(std::string("assets/module-skins/")+names[module]+".png",std::ios::binary);std::string data((std::istreambuf_iterator<char>(f)),{}),encoded;unsigned acc=0;int bits=0;for(unsigned char c:data){acc=(acc<<8)|c;bits+=8;while(bits>=6){bits-=6;encoded+=alphabet[(acc>>bits)&63];}}if(bits)encoded+=alphabet[(acc<<(6-bits))&63];while(encoded.size()%4)encoded+='=';std::cout<<"<image x='"<<r.x<<"' y='"<<r.y<<"' width='"<<r.w<<"' height='"<<r.h<<"' preserveAspectRatio='none' xlink:href='data:image/png;base64,"<<encoded<<"'/><rect x='"<<r.x<<"' y='"<<r.y<<"' width='"<<r.w<<"' height='"<<r.h<<"' fill='black' opacity='"<<(module==mockup::inputOptionsBackground?0.:module==3?.45:.30)<<"'/>";};
 auto display=[](ParamID id,double v)->std::string{
  if(id==kRndSource)return v<.5?"Random key Root trigger":"Gater steps";
  if(id==kRndRoot){const char* names[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};return names[int(std::round(v*11))];}
  if(id==kRndMinor)return v<.5?"Major":"Minor";
  if(id==kRndNoteMin||id==kRndNoteMax)return std::to_string(int(std::round(v*127)));
  if(id==kRndLengthMin||id==kRndLengthMax)return std::to_string(int(std::round(5+v*1995)))+" ms";
  if(id==kRndGateEvery)return std::to_string(1+int(std::round(v*15)));
  if(id>=kRndLfo&&id<kRndRoutes)id=kLfo0+id-kRndLfo;
  if(id>=kRndSlots&&id<kRndPolarity)id=kLfoSlots0+id-kRndSlots;
  if(id>=kRndPolarity&&id<kRndExtra)id=kSlotPolarity0+id-kRndPolarity;
  for(auto base:{kRandomSteps0,kLfoSpeed0,kModWaveRate0,kLfoWaveChoice0,kModWaveFrom0,kModWaveTo0})if(id==modParam(base,4)){id=base;break;}

  if(id>=kModuleLoopWindow0&&id<kModuleLoopWindow0+4){const char* n[]={"2/1","1/1","1/2","1/4"};return n[int(std::round(v*3))];}
  if(id==kAudioQuality){const char* names[]={"Draft","Normal","High"};return names[int(std::round(v*2))];}
  if(id==kResliceSeed)return std::to_string(1+int(std::round(v*65534)));
  if(id==kResliceCombType)return v<.5?"FEEDFORWARD":"FEEDBACK";
  if(id>=kResliceSubdivision&&id<kModuleLoopOn0){const auto& spec=resliceControlSpecs[id-kResliceSubdivision];double plain=spec.lo+v*(spec.hi-spec.lo);std::ostringstream out;out<<plain<<" "<<spec.unit;return out.str();}
  if(id>=kSlotPolarity0&&id<kRoutingOrder){const char* names[]={"+/-","+","-"};return names[int(std::round(v*2))];}
  if(id==kReverbRateV2){const char* n[]={"PRESET","1/4","1/8","1/16","1/32","1/4D","1/8D"};return n[int(std::round(v*6))];}
  switch(id){case kResliceAlgorithm:{const char* n[]={"CutDSG","WarpDSG","PushDSG"};return n[int(std::round(v*2))];}case kReslicePhrase:return std::to_string(1<<int(std::round(v*3)))+" BARS";case kResliceLength:{const char* n[]={"4/1","2/1","1/1","1/2"};return n[int(std::round(v*3))];}case kSize:return "73.75 MS";case kDensity:return "4 X STEP";case kPosition:return "2000 MS";case kPitch:case kTranspose:return "0 ST";case kStretchSpeed:return "100%";
  case kGrainBuffer:return "4S";case kPanMode:return "MANUAL";case kFilterCutoff:return "253 HZ";case kFilterDrive:return "6.45 DB";case kFilterModel:return "NORTHERN";case kFilterType:return "HP";case kFilterSlope:return "12DB";
  case kFilterSeqMode:return "ARP";case kFilterSeqPattern:return "01 RISE 1";case kFilterSeqRate:return "1/16";case kReverbModel:return "PLATE";case kReverbType:return "ROOM";case kReverbSource:return "RANDOM IMP.";case kReverbRandomRate:case kReverbGrid:return "1/4";case kLimiterCeiling:return "-10DB";case kModuleOrder:return "G > P > R";case kXTarget:return "PITCH";case kYTarget:return "DENSITY";case kLfoSpeed0:{const char* n[]={"0.25X","0.5X","1X","2X"};return n[int(std::round(v*3))];}case kModWaveRate0:{const char* n[]={"Saved rate","Off","1/1","1/2","1/4","1/8","2/1","4/1"};return n[int(std::round(v*7))];}case kModWaveRnd0:return "1/2";case kRandomSteps0:return "16";}
  if(id>=kLfoWaveChoice0&&id<kModWaveFrom0){int w=std::max(0,int(std::round(v*qg::waveChoiceCount))-1);if(w>=130)return qg::extraWaveName(w);if(w>=128)return w==128?"S&H":"S&G";return std::string(waveFamilies[w/16])+" "+std::to_string(w%16+1);}
  if(id==lfoID(0,lHz)){std::ostringstream out;out<<std::fixed<<std::setprecision(2)<<(.01+39.99*v)<<" Hz";return out.str();}
  if(id==lfoID(0,lWave))return "125 RANDOM CURVE";if(id==lfoID(0,lGrid))return "4 BEATS";
  for(int i=0;i<6;++i)if(id==slotTarget(0,i)){int target=int(std::round(v*qg::modTargetCount))-1;return target<0?"None":modNames[target];}
  return std::to_string(int(v*100))+"%";
 };
 mockup::RackState rack;rack.resliceAdvanced=argc>7&&std::atoi(argv[7])!=0;rack.page=std::clamp(tab,0,2);rack.scroll[rack.page]=argc>2?std::atof(argv[2]):0;if(argc>5&&rack.page==0)rack.scroll[0]=std::clamp(mockup::moduleRect(std::atoi(argv[5]),[&](ParamID id){return state[id];},rack).y-220.,0.,mockup::maxScroll(rack));int clipId=0;p.clip=[&](mockup::Rect r){std::cout<<"<defs><clipPath id='clip"<<++clipId<<"'><rect x='"<<r.x<<"' y='"<<r.y<<"' width='"<<r.w<<"' height='"<<r.h<<"'/></clipPath></defs><g clip-path='url(#clip"<<clipId<<")'>";};p.unclip=[](){std::cout<<"</g>";};
 mockup::render(p,[&](ParamID id){return state[id];},display,"PRESET..",0,selectedMod,0,0,0,nullptr,-1,-1,nullptr,mockup::RenderPass::All,rack);std::cout<<"</svg>";
}
