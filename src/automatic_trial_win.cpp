#include "automatic_trial.h"
#include "trial_store_win.h"
namespace aztec::license {
namespace {
struct Storage {
 static constexpr int64_t epoch=11644473600LL;
 int64_t now()const{return automatic::wallSeconds();}
 int64_t steady()const{return automatic::steadySeconds();}
 State update(bool create,int64_t current,int64_t minimum,int64_t& first,int64_t& last){
  using grains::trial::Tick;using grains::trial::Store;using grains::trial::Refresh;constexpr auto second=grains::trial::second;
  PWSTR path=nullptr;if(FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData,KF_FLAG_CREATE,nullptr,&path)))return State::StorageError;
  std::wstring directory(path);CoTaskMemFree(path);
  Store store(L"Software\\GrainsDosage\\AutomaticTrial",directory+L"\\GrainsDosage-AutomaticTrial");
  grains::trial::State record{first?Tick(first+epoch)*second:0,last?Tick(last+epoch)*second:0};
  const auto result=store.check(Tick(current+epoch)*second,record,create,minimum?Tick(minimum+epoch)*second:0,true);
  if(record.start){first=int64_t(record.start/second)-epoch;last=int64_t(record.last/second)-epoch;}
  switch(result){case Refresh::active:return State::Trial;case Refresh::expired:return State::Expired;case Refresh::missing:return State::Ready;case Refresh::invalid:return State::ClockError;default:return State::StorageError;}
 }
};
automatic::Manager<Storage>& manager(){static automatic::Manager<Storage> instance;return instance;}
}
void initialize(){(void)manager();}
void firstOpen(){manager().open();}
std::string status(){return manager().label();}
std::string deviceCode(){return {};}
bool activate(const std::string&,std::string& error){error="The trial starts automatically. Install the licensed edition to activate a purchase.";return false;}
}
