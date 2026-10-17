#pragma once
#include "license.h"
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace aztec::license::automatic {
inline int64_t wallSeconds(){return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();}
inline int64_t steadySeconds(){return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
// Storage must serialize processes and persist the original start/high-water mark.
// No storage or locking is called by the audio thread.
template<class Storage> class Manager {
 Storage storage;
 std::mutex mutex;std::condition_variable wake;bool stopping=false;
 std::thread worker;
 const bool backgroundOnly;std::atomic<bool> openRequested{false};
 State state=State::Ready;int64_t first=0,last=0,deadline=0,nextStore=0;
 std::atomic<State> visible{State::Ready};std::atomic<int64_t> end{0};
 void publish(State s){state=s;visible.store(s);allowed.store(s==State::Trial,std::memory_order_release);}
 void refresh(bool create=false){
  const auto now=storage.now(),steady=storage.steady();
  if(create||steady>=nextStore||!first){
   const auto result=storage.update(create,now,deadline?first+trialSeconds-std::max<int64_t>(0,deadline-steady):0,first,last);
   nextStore=steady+30;
   if(result!=State::Trial&&result!=State::Expired){publish(result);return;}
   end.store(first+trialSeconds);
   if(!deadline)deadline=steady+std::max<int64_t>(0,first+trialSeconds-std::max(now,last));
   automaticEnd.store(first+trialSeconds);automaticSteadyEnd.store(deadline);
   if(result==State::Expired){publish(State::Expired);return;}
   state=State::Trial;
  }
  if(!first){publish(State::Ready);return;}
  if(now<first||now+120<last){publish(State::ClockError);return;}
  if(now>=first+trialSeconds||last>=first+trialSeconds||steady>=deadline){
   // Persist expiry immediately; a clock rollback must not revive this record.
   storage.update(false,now,first+trialSeconds,first,last);publish(State::Expired);return;
  }
  // Storage failures stay blocked until the next successful protected refresh.
  if(state==State::StorageError||state==State::ClockError)return;
  publish(State::Trial);
 }
public:
 explicit Manager(bool background=false):backgroundOnly(background){
  if(!backgroundOnly){try{refresh();}catch(...){publish(State::StorageError);}}
  worker=std::thread([this]{
   std::unique_lock<std::mutex> lock(mutex);
   // Background mode defers even the first read until actual editor/audio use.
   while(!stopping){
    auto requested=[this]{return stopping||openRequested.load();};
    if(!backgroundOnly&&(state==State::Ready||state==State::Expired))
     wake.wait(lock,[&]{return requested()||(state!=State::Ready&&state!=State::Expired);});
    else wake.wait_for(lock,std::chrono::seconds(1),requested);
    if(stopping)break;
    const bool create=openRequested.exchange(false);
    if(!create&&(state==State::Ready||state==State::Expired))continue;
    try{refresh(create);}catch(...){publish(State::StorageError);}
   }
  });
 }
 ~Manager(){{std::lock_guard<std::mutex> lock(mutex);stopping=true;}wake.notify_all();if(worker.joinable())worker.join();}
 void open(){
  // AU cold-open can hold host/loader locks. Never wait for Keychain or the
  // storage worker there. Audio remains blocked until validation succeeds.
  if(backgroundOnly){openRequested.store(true);wake.notify_all();return;}
  std::lock_guard<std::mutex> lock(mutex);try{refresh(true);}catch(...){publish(State::StorageError);}wake.notify_all();}
 std::string label(){const auto s=visible.load();if(s==State::Trial){auto remaining=std::min(end.load()-storage.now(),automaticSteadyEnd.load()-storage.steady());if(remaining<=0)return "TRIAL EXPIRED";return "TRIAL - "+std::to_string((remaining+86399)/86400)+" DAYS LEFT";}if(s==State::Expired)return "TRIAL EXPIRED";if(s==State::ClockError)return "CHECK SYSTEM CLOCK";if(s==State::StorageError)return "TRIAL STORAGE ERROR";return "14 DAY TRIAL - STARTS ON FIRST USE";}
};
}
