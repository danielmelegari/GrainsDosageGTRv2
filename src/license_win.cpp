#include "license.h"
#include "trial_license_win.h"
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace aztec::license {
namespace {
using namespace grains::trial;
class Manager {
 std::mutex mutex;std::condition_variable wake;bool stopping=false;std::thread worker;
 License license;bool verified=false,protectedOkay=false,hasStore=false,deadlineSet=false;grains::trial::State persisted{};
 std::chrono::steady_clock::time_point deadline{},lastStore{};
 std::atomic<aztec::license::State> visible{aztec::license::State::Required};
 std::atomic<Tick> end{0};unsigned failures=0;
 void publish(aztec::license::State state){visible.store(state,std::memory_order_release);allowed.store(state==aztec::license::State::Trial||state==aztec::license::State::Licensed,std::memory_order_release);}
 void refresh(bool initial=false){
  if(!verified){publish(aztec::license::State::Required);return;}
  if(license.permanent){publish(aztec::license::State::Licensed);return;}
  const auto current=wallTime();const auto steady=std::chrono::steady_clock::now();
  if(current>=license.end){publish(aztec::license::State::Expired);return;}
  if(current<license.start){publish(aztec::license::State::ClockError);return;}
  if(!deadlineSet){deadline=steady+std::chrono::nanoseconds((license.end-current)*100);deadlineSet=true;}
  if(steady>=deadline){publish(aztec::license::State::Expired);return;}
  if(initial||steady-lastStore>=std::chrono::seconds(30)){
   const auto remaining=std::chrono::duration_cast<std::chrono::nanoseconds>(deadline-steady).count()/100;
   const auto minimum=remaining>0?license.end-Tick(remaining):license.end;
   auto store=Store::production();const auto result=store.check(current,persisted,initial||!hasStore,minimum);
   lastStore=steady;
   if(result==Refresh::unavailable){++failures;if(initial||failures>=3){protectedOkay=false;publish(aztec::license::State::StorageError);return;}}
   else{failures=0;protectedOkay=result==Refresh::active;if(protectedOkay)hasStore=true;if(result==Refresh::expired){publish(aztec::license::State::Expired);return;}if(result==Refresh::invalid){publish(aztec::license::State::ClockError);return;}}
  }
  // Only successful validation can recover a storage/clock error. Do not let a
  // one-second clock tick bypass a failed protected-state check.
  if(!protectedOkay)return;
  publish(aztec::license::State::Trial);
 }
 void adopt(const License& next){
  license=next;verified=true;protectedOkay=false;hasStore=false;deadlineSet=false;end.store(license.end);failures=0;
  persisted={license.start,license.start};

  refresh(true);
 }
 void load(){
  License next;
  if(readLicense(licenseDirectory()+L"\\trial.license",next,false)){
   if(!verified||next.payload!=license.payload)adopt(next);else refresh();
  }else if(verified)refresh();else publish(aztec::license::State::Required);
 }
public:
 Manager(){load();worker=std::thread([this]{std::unique_lock<std::mutex> lock(mutex);while(!wake.wait_for(lock,std::chrono::seconds(1),[this]{return stopping;})){try{refresh();}catch(...){publish(aztec::license::State::StorageError);}}});}
 ~Manager(){{std::lock_guard<std::mutex> lock(mutex);stopping=true;}wake.notify_all();worker.join();}
 void open(){std::lock_guard<std::mutex> lock(mutex);load();}
 std::string label(){switch(visible.load(std::memory_order_acquire)){
  case aztec::license::State::Licensed:return "LICENSE ACTIVE";
  case aztec::license::State::Trial:{const Tick finish=end.load(),current=wallTime();return current>=finish?"TRIAL EXPIRED":"14 DAYS LICENSE ACTIVE - "+std::to_string((finish-current+86400*second-1)/(86400*second))+" DAYS LEFT";}
  case aztec::license::State::Expired:return "TRIAL EXPIRED";
  case aztec::license::State::StorageError:return "LICENSE STORAGE ERROR";
  case aztec::license::State::ClockError:return "CHECK SYSTEM CLOCK";
  default:return "ACTIVATE LICENSE";
 }}
 bool activate(const std::string& input,std::string& error){std::lock_guard<std::mutex> lock(mutex);
  std::string text;for(char c:input)if(c!='\r')text+=c;
  const auto first=text.find_first_not_of(" \t\n");if(first!=std::string::npos)text.erase(0,first);
  while(!text.empty()&&(text.back()=='\n'||text.back()==' '||text.back()=='\t'))text.pop_back();text+='\n';
  License next;
  if(!verifyLicenseText(text,next,machineId(),wallTime(),false)){error="Invalid signature or Machine ID. Paste the complete Windows TXT, including all five lines.";return false;}
  const auto current=wallTime();if(!next.permanent&&(current<next.start||current>=next.end)){error="This signed trial has not started or has expired.";return false;}
  if(verified&&license.permanent&&!next.permanent){error="This computer already has a permanent license.";return false;}
  const auto directory=licenseDirectory();if(directory.empty()||(!CreateDirectoryW(directory.c_str(),nullptr)&&GetLastError()!=ERROR_ALREADY_EXISTS)){error="Cannot create the local licensing folder.";return false;}
  wchar_t temporary[MAX_PATH]{};if(!GetTempFileNameW(directory.c_str(),L"GDL",0,temporary)){error="Cannot create a temporary license file.";return false;}
  bool saved=false;{
   Handle file;file.value=CreateFileW(temporary,GENERIC_WRITE,0,nullptr,TRUNCATE_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
   if(file.value!=INVALID_HANDLE_VALUE){DWORD written=0;saved=WriteFile(file.value,text.data(),DWORD(text.size()),&written,nullptr)&&written==text.size()&&FlushFileBuffers(file.value);}
  }
  if(saved)saved=MoveFileExW(temporary,(directory+L"\\trial.license").c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
  if(!saved){DeleteFileW(temporary);error="Cannot save the activation file.";return false;}
  if(verified&&next.payload==license.payload)refresh(true);else adopt(next);if(!allowed.load()){error="License saved, but its protected state or system clock is invalid. Correct the clock/storage access and reopen the plugin.";return false;}return true;
 }
};
Manager& manager(){static Manager instance;return instance;}
}
std::string deviceCode(){return grains::trial::machineId();}
void initialize(){(void)manager();}
void firstOpen(){manager().open();}
std::string status(){return manager().label();}
bool activate(const std::string& input,std::string& error){return manager().activate(input,error);}
}
