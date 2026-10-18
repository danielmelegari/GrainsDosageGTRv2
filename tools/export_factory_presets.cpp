#include "../src/preset_io.h"
#include <fstream>
#include <iostream>
int main(int argc,char** argv){if(argc!=2)return 2;auto p=aztec::initialParameters();std::ofstream out(std::string(argv[1])+"/Default.gdspreset",std::ios::binary);out<<aztec::encodePreset([&](int id){return p[id];});if(!out)return 1;std::cout<<"Exported Default (the only bundled preset)\n";}
