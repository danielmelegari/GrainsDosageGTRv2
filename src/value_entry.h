#pragma once
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "pluginterfaces/base/ustring.h"
#include <string>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cmath>
namespace aztec {
inline std::string entryText(const Steinberg::Vst::TChar* text){char out[512]{};Steinberg::UString(const_cast<Steinberg::Vst::TChar*>(text),128).toAscii(out,512);return out;}
inline std::string cleanEntry(std::string s){auto a=s.find_first_not_of(" \t\r\n"),b=s.find_last_not_of(" \t\r\n");if(a==std::string::npos)return {};s=s.substr(a,b-a+1);for(auto& c:s)c=char(std::tolower(static_cast<unsigned char>(c)));return s;}
inline bool parseParameterEntry(Steinberg::Vst::EditController* controller,Steinberg::Vst::ParamID id,std::string text,double& result){
 using namespace Steinberg;using namespace Steinberg::Vst;auto* parameter=controller->getParameterObject(id);if(!parameter)return false;auto info=parameter->getInfo();text=cleanEntry(text);if(text.empty())return false;
 const std::string unit=cleanEntry(entryText(info.units));
 if(info.flags&ParameterInfo::kIsList){for(int i=0;i<=info.stepCount;++i){double n=info.stepCount?double(i)/info.stepCount:0.;String128 s{};controller->getParamStringByValue(id,n,s);auto label=cleanEntry(entryText(s));if(text==label||(!unit.empty()&&text==label+" "+unit)){result=n;return true;}}return false;}
 std::replace(text.begin(),text.end(),',','.');char* end=nullptr;double plain=std::strtod(text.c_str(),&end);if(end==text.c_str()||!std::isfinite(plain))return false;auto tail=cleanEntry(end);if(!tail.empty()&&tail!=unit)return false;
 double n=parameter->toNormalized(plain);if(!std::isfinite(n)||n<0.||n>1.)return false;
 if(std::abs(parameter->toPlain(n)-plain)>1e-6*std::max(1.,std::abs(plain)))return false;
 result=info.stepCount?std::round(n*info.stepCount)/info.stepCount:n;return true;
}
}
