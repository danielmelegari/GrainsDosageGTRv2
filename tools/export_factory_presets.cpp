#include "../src/preset_io.h"
#include <fstream>
#include <iostream>
int main(int argc,char** argv){if(argc!=2)return 2;using namespace aztec;
 std::string directory=argv[1];std::ofstream catalog(directory+"/Preset-Catalog.md");catalog<<"# GrainsDosage Routing Bank — 128 presets\n\nEight categories with sixteen new presets each. Every preset stores an explicit five-module routing.\n\nCopy the .gdspreset files into the GrainsDosage preset folder. The plugin installs these automatically on first opening its preset menu. Existing user files are never overwritten.\n\n";
 const char* stage[]={"Granulizer","Preslicer","BeatRepeater","Reslice","Gater"};
 for(int n=0;n<factoryPresetCount;++n){if(n%16==0)catalog<<"\n## "<<factoryCategories[n/16]<<"\n\n| Preset | Audio routing |\n| --- | --- |\n";auto p=factoryPreset(n);std::ofstream out(directory+"/"+factoryNames[n]+".gdspreset",std::ios::binary);out<<encodePreset([&](int id){return p[id];});if(!out)return 1;catalog<<"| "<<factoryNames[n]<<" | ";auto chain=fiveModuleOrder(int(std::round(p[kRoutingOrder]*120))-1);for(int i=0;i<5;++i){if(i)catalog<<" → ";catalog<<stage[chain[i]];}catalog<<" |\n";}
 if(!catalog)return 1;std::cout<<"Exported 128 presets and category/routing catalog\n";
}
