#include "../src/preset_io.h"
#include "../src/preset_banks.h"
#include <fstream>
#include <iostream>
int main(int argc,char** argv){if(argc<2||argc>3)return 2;using namespace aztec;
 int bank=argc==3?std::atoi(argv[2]):0;if(bank<0||bank>=3)return 2;
 std::string directory=argv[1];std::ofstream catalog(directory+"/Preset-Catalog.md");catalog<<"# GrainsDosage — "<<presetBankNames[bank]<<" — 128 presets\n\n";
 if(bank==0)catalog<<"The original 128 routing-bank presets, with module Mix set to 100% when ON and 0% when OFF.\n\n";
 if(bank==1)catalog<<"Original glitch presets inspired by Richard Devine's intricate, experimental sound design: micro-slicing, irregular clocks and resonant fragments.\n\n";
 if(bank==2)catalog<<"Original slow, harmonic and organic forest textures inspired by the atmosphere of Parvati Records.\n\n";
 catalog<<"All module Mix controls, including Reverb: ON = 100%, OFF = 0%. Gater has no Mix control.\n\nEight categories, sixteen patches each. These effect presets process incoming audio; tempo follows the host.\n\nCopy the .gdspreset files into the GrainsDosage preset folder. This build installs all three banks and updates untouched older factory files. User-edited files are preserved.\n\n";
 const char* stage[]={"Granulizer","Preslicer","BeatRepeater","Reslice","Gater"};
 for(int n=0;n<128;++n){if(n%16==0)catalog<<"\n## "<<bankCategoryName(bank,n/16)<<"\n\n| Preset | Audio routing |\n| --- | --- |\n";auto p=bankPreset(bank,n);auto name=bankPresetName(bank,n);std::ofstream out(directory+"/"+name+".gdspreset",std::ios::binary);out<<encodePreset([&](int id){return p[id];});if(!out)return 1;catalog<<"| "<<name<<" | ";auto chain=fiveModuleOrder(int(std::round(p[kRoutingOrder]*120))-1);for(int i=0;i<5;++i){if(i)catalog<<" → ";catalog<<stage[chain[i]];}catalog<<" |\n";}
 if(!catalog)return 1;std::cout<<"Exported "<<presetBankNames[bank]<<": 128 presets and category/routing catalog\n";
}
