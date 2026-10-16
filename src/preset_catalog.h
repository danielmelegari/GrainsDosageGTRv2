#pragma once
#include "preset_banks.h"
namespace aztec {
inline std::pair<int,int> installedFactoryIndex(const std::string& stem){
 for(int bank=0;bank<presetBankCount;++bank)for(int i=0;i<presetsPerBank;++i)if(stem==bankPresetName(bank,i))return {bank,i};
 return {-1,-1};
}
// Hide only exact, untouched bundled patches. Never delete, rewrite, or hide a
// modified/user-named file. Legacy imported user patches remain accessible.
inline bool untouchedFactoryPreset(const std::string& stem,const std::string& bytes){
 auto [bank,index]=installedFactoryIndex(stem);if(bank<0)return false;
 std::array<double,kCount> saved{};if(!decodePreset(bytes,saved))return false;
 const auto expected=bankPreset(bank,index);bool equal=true;
 for(int id=0;id<kCount;++id)if(presetParameter(id)&&saved[id]!=expected[id]){equal=false;break;}
 return equal||needsBankMixUpgrade(bytes,bank,index);
}
// Index -1 is the immutable built-in Default; other indices are saved files.
inline int adjacentPreset(int current,int direction,int files){int total=files+1;return ((current+1+(direction>0?1:total-1))%total)-1;}
}
