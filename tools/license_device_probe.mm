#include "license.h"
#include <cassert>
#include <iostream>
int main(){auto code=aztec::license::deviceCode();assert(code.size()==71&&code.substr(0,7)=="GDDEV2-");assert(code.find_first_not_of("0123456789abcdef",7)==std::string::npos);assert(code==aztec::license::deviceCode());std::cout<<"PASS: real Mac hardware provides a stable product-specific device code\n";}
