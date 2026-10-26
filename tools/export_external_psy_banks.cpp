// Offline authoring/export tool. Never called by the plugin or installer.
#include "../src/preset_io.h"
#include "../src/parameter_settings.h"
#include <filesystem>
#include <fstream>
#include <set>
#include <iostream>
#include <stdexcept>
using namespace aztec;
using Patch=std::array<double,kCount>;
namespace fs=std::filesystem;
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void plain(Patch& p,ParamID id,double v){const auto& s=resliceControlSpecs[id-kResliceSubdivision];p[id]=(v-s.lo)/(s.hi-s.lo);}
void lfo(Patch& p,int l,int target,double depth,int wave,int grid){p[lfoID(l,lEnabled)]=1;p[lfoID(l,lSync)]=1;p[lfoID(l,lGrid)]=grid/20.;p[lfoID(l,lReset)]=1;p[lfoID(l,lGlide)]=0;p[modParam(kLfoWaveChoice0,l)]=(wave+1.)/qg::waveChoiceCount;p[slotTarget(l,0)]=(target+1.)/qg::modTargetCount;p[slotAmount(l,0)]=.5+depth*.5;}
void gate(Patch& p,unsigned mask,double length,int grid=2){p[kGaterEnabled]=1;p[kGaterGrid]=grid/3.;for(int i=0;i<16;++i){p[kGaterState0+i]=(mask>>i)&1;p[kGaterLength0+i]=(length-.05)/.95;p[kGaterSustain0+i]=0;}}
void repeat(Patch& p,unsigned mask,int division,int lastDivision,int lastPitch=0){p[kRepeatEnabled]=p[kRepeatSeq]=1;for(int i=0;i<16;++i){p[kRepeatStep0+i]=(mask>>i)&1;p[kRepeatRate0+i]=(i%4==3?lastDivision:division)/15.;p[kRepeatPitch0+i]=.5+(i%4==3?lastPitch:0)/96.;}}
void cuts(Patch& p,int subdivision,int repeats,double area,double pitch,double delay=0){p[kResliceEnabled]=1;p[kReslicePhrase]=0;p[kResliceSeed]=1234./65534.;p[kResliceVariation]=p[kResliceReverse]=0;p[kResliceRepeat]=1;p[kResliceFill]=0;plain(p,kResliceMinPhrase,1);plain(p,kResliceSubdivision,subdivision);plain(p,kResliceMinRepeats,repeats);plain(p,kResliceMaxRepeats,repeats);plain(p,kResliceArea,area);plain(p,kResliceMinPitch,pitch);plain(p,kResliceMaxPitch,pitch);plain(p,kResliceFade,2);if(delay){plain(p,kResliceCombOn,1);plain(p,kResliceMinDelay,delay);plain(p,kResliceMaxDelay,delay);plain(p,kResliceCombFeedback,52);}}
struct Design{const char* name;double ms,density,pitch,look;int filter;double hz,res;const char* source;const char* intent;};
const Design forest[]={
 {"01 Root Conversation",68,9,-7,.12,12,850,.46,"Dry FM or woody percussion loop","Rounded vowel movement and a 3-3-2 gate; grounded forest call-and-response."},
 {"02 Bark Scratcher",24,12,-12,.055,17,1250,.38,"Scrapes, shakers or short noise hits","Close, rough grain clusters with a narrow moving spectral band."},
 {"03 Night Insects",19,17,12,.09,11,1750,.52,"Dry metallic ticks or an FM stab","High chattering grains; fast small formant motion over an uneven pulse."},
 {"04 Hollow Trunk",92,7,-12,.20,7,196,.48,"Woodblocks or a short broadband hit","Low comb ringing and long grains; tuned around G3 rather than a bright laser."},
 {"05 Vine Dialogue",46,11,0,.16,9,940,.43,"Midrange FM phrase or dry vocal fragment","Two-bar vowel sweep with sparse repeated answers at the phrase ends."},
 {"06 Spore Spiral",110,8,7,.28,15,720,.38,"Sustained rough synth or textured field recording","Slow phaser orbit and bounded lookback movement; an evolving bed without reverb wash."},
 {"07 Ritual Feet",38,10,-5,.075,2,1600,.43,"Dry hand-percussion loop","Short dark grains and a fixed interlocking gate with full-length attack spaces."},
 {"08 Understory Cuts",58,9,0,.11,10,1100,.40,"Dry FM rhythm or woody loop","Controlled two-repeat cuts into an E vowel; fixed pitch and source-area limits."},
 {"09 Swamp Call",135,6,-19,.31,13,700,.50,"Short saw/FM stab","Low throaty U vowel and a slow breathing cutoff, for sparse replies."},
 {"10 Canopy Echo",75,8,7,.18,1,2600,.25,"Sparse wood or bell hits","Dedicated fully wet short reverb texture; leave gaps between source hits."},
 {"11 Mycelium Pulse",31,14,0,.065,3,1800,.56,"Dry midrange saw or FM loop","Damped acid motion with sixteenth-note gates; rhythmic forest lead processing."},
 {"12 Deep Clearing",180,6,-12,.42,16,1300,.30,"Textured drone or sustained vocal","Dedicated fully wet dark space for transitions, with a four-bar cutoff cycle."}
};
const Design hitech[]={
 {"01 Falling Laser",18,20,12,.025,5,3400,.56,"Sustained saw or bright FM tone","One-beat falling pitch sweep of +/-18 semitones plus a falling resonant peak."},
 {"02 Needle Burst",15,26,24,.025,1,6200,.32,"Bright FM stab or metallic tick","Thirty-second-note needle gates and sixty-fourth-note answers."},
 {"03 Binary Teeth",21,22,0,.040,4,3200,.50,"Dry midrange synth loop","Alternating binary gates with short repeat cuts; no random probability."},
 {"04 Chrome Rail",25,18,7,.050,7,784,.56,"Broadband transient or short saw stab","Tight metallic comb ring with a bounded quarter-beat comb-frequency sweep."},
 {"05 Triplet Ricochet",20,18,12,.035,5,2800,.48,"Dry tonal stab loop","Programmed triplet repeat lengths; every fourth step answers one octave up."},
 {"06 Octave Circuit",28,16,0,.060,2,4200,.42,"Single-note saw or FM loop","Fixed sixteen-step pitch phrase: root, octave, fifth and two octaves."},
 {"07 Reverse Voltage",35,14,12,.085,8,1046.5,.48,"Short metallic or FM loop","Reverse cut bursts through a negative comb; fixed reverse setting and source area."},
 {"08 Data Splinters",17,24,7,.030,17,4600,.40,"Noisy FM or percussion loop","Fixed 10-bit/12 kHz cut texture, short gates and bounded two-repeat fragments."},
 {"09 Alien Syllables",22,19,12,.045,9,1800,.52,"Bright FM phrase or dry vocal","Fast A-vowel articulation with timed holes and phrase-end repetitions."},
 {"10 Rising Railgun",16,25,12,.025,5,4200,.54,"Sustained bright saw/FM tone","Half-beat rising pitch and resonant-frequency sweeps for energetic laser runs."},
 {"11 Comb Shrapnel",19,21,0,.040,1,6500,.28,"Short noise hit or metallic percussion","Three fixed repeats into a 2 ms internal comb, for metallic glitch shards."},
 {"12 Terminal Fill",23,20,12,.035,3,5000,.46,"Dry FM/percussion loop; use for a one-bar fill","Authored acceleration from sixteenths to sixty-fourths, octave answers and an end gap."}
};
Patch author(int bank,int i){const auto& d=bank?hitech[i]:forest[i];auto p=initialParameters();
 for(auto id:{kGrainEnabled,kRepeatEnabled,kResliceEnabled,kGaterEnabled,kReverbOn,kMasterFilter,kGlitchEnabled,kRepeatAuto,kRepeatOn,kRepeatSeq,kStretchOn,kFilterSeqOn,kGaterLengthRnd,kGaterStepRnd,kXYEnable})p[id]=0;
 for(int l=0;l<qg::lfoCount;++l){p[lfoID(l,lEnabled)]=0;for(int j=0;j<6;++j){p[slotTarget(l,j)]=0;p[slotAmount(l,j)]=.5;}}
 p[kGrainEnabled]=p[kMasterFilter]=p[kMasterLimiter]=p[kMix]=1;p[kLimiterCeiling]=.5;p[kNormalize]=0;p[kChaos]=0;p[kPanMode]=0;p[kGrainPan]=.5;p[kGrainBuffer]=.25;p[kRoutingOrder]=fourModuleRouting({{0,2,3,4}});
 p[kSize]=(d.ms/1000.-.015)/.235;p[kDensity]=(d.density-1)/31.;p[kPitch]=.5+d.pitch/96.;p[kPosition]=(d.look-.015)/1.985;p[kFilterModel]=d.filter/18.;p[kFilterCutoff]=std::log(d.hz/20.)/std::log(1000.);p[kFilterResonance]=d.res;p[kFilterDrive]=.08;
 if(!bank)switch(i){
 case 0:lfo(p,0,8,.10,0,3);gate(p,0x4949,.72);break;
 case 1:lfo(p,0,8,.12,7,4);gate(p,0xb6db,.58);break;
 case 2:lfo(p,0,8,.09,4,6);gate(p,0x6db6,.42);break;
 case 3:lfo(p,0,8,.025,0,2);gate(p,0x1111,.88);break;
 case 4:lfo(p,0,8,.13,0,2);repeat(p,0x8080,3,4);break;
 case 5:lfo(p,0,8,.11,0,1);lfo(p,1,3,.035,0,2);break;
 case 6:gate(p,0x5959,.70);lfo(p,0,8,.08,16,3);break;
 case 7:cuts(p,8,2,50,0);lfo(p,0,8,.08,0,3);break;
 case 8:lfo(p,0,8,.12,0,2);gate(p,0x0101,.9,1);break;
 case 9:p[kReverbOn]=1;p[kReverbModel]=.25;p[kReverbLength]=(1.2-.2)/19.8;for(int j=0;j<16;++j)p[kReverbStep0+j]=1;break;
 case 10:gate(p,0xeeee,.62);lfo(p,0,8,.14,23,5);break;
 case 11:p[kReverbOn]=1;p[kReverbModel]=0;p[kReverbLength]=(3.2-.2)/19.8;for(int j=0;j<16;++j)p[kReverbStep0+j]=1;lfo(p,0,8,.10,0,1);break;
 }else switch(i){
 case 0:lfo(p,0,2,18./48.,68,5);lfo(p,1,8,.19,68,5);break;
 case 1:gate(p,0xeeee,.32,3);repeat(p,0x8888,5,6,12);break;
 case 2:gate(p,0xaaaa,.48,3);repeat(p,0x3333,5,6);break;
 case 3:lfo(p,0,8,.08,68,7);gate(p,0x7777,.62);break;
 case 4:repeat(p,0xffff,10,11,12);break;
 case 5:repeat(p,0xffff,4,5);{const int notes[]={0,0,12,0,7,0,12,24,0,12,7,0,24,12,7,0};for(int j=0;j<16;++j)p[kRepeatPitch0+j]=.5+notes[j]/96.;}break;
 case 6:cuts(p,16,2,25,0);p[kResliceReverse]=1;break;
 case 7:cuts(p,16,2,25,0);plain(p,kResliceCrushOn,1);plain(p,kResliceMinBits,10);plain(p,kResliceMaxBits,10);plain(p,kResliceMinFreq,12000);plain(p,kResliceMaxFreq,12000);gate(p,0xbdbd,.50);break;
 case 8:lfo(p,0,8,.16,16,6);gate(p,0x6db6,.46);repeat(p,0x8080,5,6,7);break;
 case 9:lfo(p,0,2,12./48.,52,6);lfo(p,1,8,.18,52,6);break;
 case 10:cuts(p,16,3,25,0,2);gate(p,0x7777,.60);break;
 case 11:repeat(p,0x7fff,4,6,12);for(int j=0;j<16;++j)p[kRepeatRate0+j]=(j<4?4:j<8?5:6)/15.;gate(p,0x7fff,.58);break;
 }
 const ParamID enabled[]={kGrainEnabled,kGlitchEnabled,kRepeatEnabled,kResliceEnabled,kReverbOn};const ParamID mix[]={kGrainMix,kGlitchMix,kRepeatMix,kResliceMix,kReverbMix};for(int j=0;j<5;++j)p[mix[j]]=p[enabled[j]]>=.5?1:0;
 return p;
}
int main(int argc,char** argv){require(argc==2,"output folder required");fs::path root=argv[1];fs::create_directories(root);std::ofstream report(root/"validation.csv");report<<"bank,preset,sample_rate,bpm,rms,peak\n";std::set<std::string> unique;
 for(int bank=0;bank<2;++bank){std::string name=bank?"Psy Hi-Tech - Laser Circuits":"Parvati Inspired - Forest Rituals";auto dir=root/name;fs::create_directories(dir);std::ofstream guide(dir/"PRESET-GUIDE.md");guide<<"# "<<name<<"\n\n12 individually authored effect presets for GrainsDosage 0.13.22.\n"<<(bank?"Suggested tempo: 175–200 BPM.\n":"Suggested tempo: 145–155 BPM. Original forest-psy designs; not an official Parvati Records product or a recreation of specific tracks.\n")<<"\nAll enabled modules with a Mix control use 100%; disabled module mixes are 0%. Master mix is 100%. Filter and Gater have no separate mix. No modulation targets any mix. Limiter enabled at -6 dBFS.\n\nUse on a separate synth/percussion/FX track, preferably with a dry source and some headroom. These process incoming audio; they do not generate a forest or laser sound from silence. Avoid the kick/bass bus for the first audition. Let audio play for a bar to fill the buffers. Pitches depend on your source.\n\nThe rhythm masks, LFO waves, rates and pitch phrases are authored, not randomized. Granular scheduling and Reslice retain their inherent engine behavior; fixed Reslice seeds/ranges constrain their variation.\n\n";
 for(int i=0;i<12;++i){auto p=author(bank,i);const auto& d=bank?hitech[i]:forest[i];for(double v:p)require(std::isfinite(v)&&v>=0&&v<=1,"invalid parameter");auto text=encodePreset([&](int id){return p[id];});require(unique.insert(text).second,"duplicate patch");Patch decoded;require(decodePreset(text,decoded),"decode failed");for(int id=0;id<presetCount;++id)if(presetParameter(id))require(p[id]==decoded[id],"roundtrip mismatch");std::ofstream(dir/(std::string(d.name)+".gdspreset"))<<text;
 guide<<"## "<<d.name<<"\n\n"<<d.intent<<"\n\nSource: "<<d.source<<". Grain: "<<d.ms<<" ms; density "<<d.density<<"; pitch "<<d.pitch<<" semitones. Filter: "<<qg::filterModels[d.filter]<<", "<<d.hz<<" Hz.\n\n";
 for(int rate:{44100,48000}){double bpm=bank?(rate==44100?180:195):(rate==44100?148:154);auto engine=std::make_unique<qg::Engine>();auto s=settings(decoded,bpm,rate);engine->prepare(rate);engine->set(s);qg::MasterFx master;master.prepare(rate);master.set(false,0,1000,0,true,0,0,-6);double energy=0,peak=0;int samples=int(rate*60./bpm*16);for(int n=0;n<samples;++n){if(n%128==0)engine->set(s);double t=double(n)/rate,beat=t*bpm/60.,phase=std::fmod(beat,.5);double envelope=.20+.8*std::exp(-phase*14);float input=float(.12*envelope*(std::sin(qg::tau*196*t+2.3*std::sin(qg::tau*392*t))+.35*std::sin(qg::tau*1733*t)));float l=0,r=0;engine->process(input,input*.93f,beat,l,r);master.process(l,r);require(std::isfinite(l)&&std::isfinite(r),"nonfinite audio");if(n>rate){energy+=l*l+r*r;peak=std::max({peak,std::abs(double(l)),std::abs(double(r))});}}double rms=std::sqrt(energy/(2*(samples-rate)));require(rms>1e-5,"silent preset");require(peak<=.501188,"ceiling exceeded");report<<bank<<','<<d.name<<','<<rate<<','<<bpm<<','<<rms<<','<<peak<<'\n';}
 std::cout<<name<<" / "<<d.name<<" OK\n"<<std::flush;
 }
 }
}
