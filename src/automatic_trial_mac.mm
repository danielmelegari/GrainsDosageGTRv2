#include "automatic_trial.h"
#include "trial_helper_process.h"
#include <dlfcn.h>
#include <string>
namespace aztec::license {
namespace {
std::string helperPath(){
 Dl_info info{};
 if(!dladdr(reinterpret_cast<const void*>(&helperPath),&info)||!info.dli_fname)return {};
 std::string path=info.dli_fname;auto slash=path.rfind('/');
 if(slash==std::string::npos)return {};
 return path.substr(0,slash)+"/../Resources/GrainsDosageTrialHelper";
}
struct Storage {
 int64_t now()const{return automatic::wallSeconds();}
 int64_t steady()const{return automatic::steadySeconds();}
 State update(bool create,int64_t current,int64_t minimum,int64_t& first,int64_t& last){
  return automatic::runHelper(helperPath(),create,current,minimum,first,last);
 }
};
automatic::Manager<Storage>& manager(){static automatic::Manager<Storage> instance(true);return instance;}
}
void initialize(){(void)manager();}
void firstOpen(){manager().open();}
std::string status(){return manager().label();}
std::string deviceCode(){return {};}
bool activate(const std::string&,std::string& error){error="The trial starts automatically. Install the licensed edition to activate a purchase.";return false;}
}
