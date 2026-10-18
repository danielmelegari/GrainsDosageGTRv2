#include "src/factory_presets.h"
#include "src/randomize.h"
#include "src/gater.h"
#include "src/engine.h"
#include "src/mockup_ui.h"
#include <iostream>
#include <vector>
int main(){using namespace aztec;int failures=0;
 auto check=[&](bool pass,const char* name){std::cout<<(pass?"PASS: ":"FAIL: ")<<name<<'\n';failures+=!pass;};
 auto p=initialParameters();std::array<bool,kCount> touched{};uint32_t seed=91;
 randomizeSound(seed,[&](ParamID id){return p[id];},[&](ParamID id,double v){p[id]=v;touched[id]=true;});
 check(touched[lfoID(4,lEnabled)]&&touched[slotTarget(4,5)]&&touched[kRndSource]&&touched[kRndLengthMin],"Global Random includes the fifth modulator");
 qg::Gater gate;qg::GaterSettings gs;gs.enabled=true;gs.latch=true;gs.release=1;gate.prepare(48000);gate.set(gs);float l=0,r=0;gate.process(l,r,0,120,true);
 check(!gate.trigger(),"Latch release steps cannot trigger RND");
 gs.release=0;gate.set(gs);gate.process(l,r,.25,120,true);check(gate.trigger(),"An active Gater step triggers RND");gs.enabled=false;gate.set(gs);check(!gate.trigger(),"Disabling Gater clears its pending RND trigger");
 // A pending event from the previous sample must not fire after host stop.
 for(int order:{0,6}){qg::Engine engine;engine.prepare(48000);qg::Settings settings;settings.order=order;settings.routing=-1;settings.gater.enabled=true;settings.lfos[4].rndSource=1;engine.set(settings);engine.process(0,0,0,l,r);settings.playing=false;settings.lfos[4].enabled=true;engine.set(settings);engine.process(0,0,0,l,r);check(engine.rndEnvelope()==0.,"Host stop rejects a pending Gater trigger");}
 p=initialParameters();mockup::Painter painter;std::vector<std::string> labels;
 painter.box=[](auto,auto,auto,double){};painter.line=[](double,double,double,double,auto,double){};painter.text=[&](const std::string& text,auto,double,auto,bool){labels.push_back(text);};
 auto render=[&](){labels.clear();mockup::renderControl(painter,{modParam(kModWaveRate0,4),{0,0,146,29},mockup::Select,"MOD W."},[&](ParamID id){return p[id];},[](ParamID,double){return std::string("Saved rate");},4);return std::find(labels.begin(),labels.end(),"Off")!=labels.end();};
 check(render(),"RND wave randomizer displays its effective saved rate");
 for(int n=0;n<200;++n){touched.fill(false);randomizeSound(seed,[&](ParamID id){return p[id];},[&](ParamID id,double v){p[id]=v;touched[id]=true;});
 if(p[kRndLengthMin]>p[kRndLengthMax]||p[kRndNoteMin]>p[kRndNoteMax]){check(false,"Ordered RND random ranges");break;}
 for(int i=0;i<4;++i)if(p[moduleLoopOn(i)]!=0.){check(false,"Global Random keeps module loops off");break;}
 for(int slot=0;slot<6;++slot){int t=int(std::round(p[slotTarget(4,slot)]*qg::modTargetCount))-1;if(t>=101&&extendedResliceIds[t-101]==kModuleLoopOn0+2)check(false,"RND random routes cannot enable a module loop");}
 }
 return failures?1:0;
}
