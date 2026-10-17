#include "src/automatic_trial.h"
#include <cassert>
#include <iostream>
using namespace aztec::license;
struct Storage {
 static inline std::atomic<int64_t> clock{automatic::wallSeconds()},mono{automatic::steadySeconds()};
 static inline int64_t start=0,seen=0;
 static inline std::atomic<bool> fail{false};
 int64_t now()const{return clock;}int64_t steady()const{return mono;}
 State update(bool create,int64_t now,int64_t minimum,int64_t& first,int64_t& last){
  if(fail)return State::StorageError;
  if(!start){if(first)return State::StorageError;if(!create)return State::Ready;start=seen=now;}
  first=start;last=seen=std::max({seen,now,minimum});
  if(last-first>=trialSeconds)return State::Expired;
  if(now<first||now+120<last)return State::ClockError;
  return State::Trial;
 }
};
int main(){
 const auto start=Storage::clock.load(),steady=Storage::mono.load();
 {automatic::Manager<Storage> m;assert(!allowed.load()&&Storage::start==0);m.open();assert(allowed.load());assert(m.label()=="TRIAL - 14 DAYS LEFT");
  for(int day=1;day<14;++day){Storage::clock=start+day*86400;Storage::mono=steady+day*86400;m.open();assert(allowed.load());assert(m.label()=="TRIAL - "+std::to_string(14-day)+" DAYS LEFT");}
  Storage::clock=start+trialSeconds-1;Storage::mono=steady+trialSeconds-1;m.open();assert(allowed.load());
  Storage::clock++;Storage::mono++;m.open();assert(!allowed.load()&&m.label()=="TRIAL EXPIRED");
 }
 {automatic::Manager<Storage> restarted;restarted.open();assert(!allowed.load());Storage::clock=start;restarted.open();assert(!allowed.load());}
 Storage::start=Storage::seen=0;Storage::clock=start;Storage::mono=steady;
 {automatic::Manager<Storage> m;m.open();Storage::fail=true;m.open();assert(!allowed.load());Storage::fail=false;m.open();assert(allowed.load());
  Storage::mono=steady+trialSeconds; // Frozen wall clock cannot extend an active session.
  m.open();assert(!allowed.load());assert(Storage::seen>=start+trialSeconds);
 }
 {automatic::Manager<Storage> restarted;assert(!allowed.load());}
 // Audio-boundary checks independently expire a currently allowed session.
 automaticEnd=automatic::wallSeconds()-1;automaticSteadyEnd=automatic::steadySeconds()+100;allowed=true;assert(!processingAllowed());
 automaticEnd=automatic::wallSeconds()+100;automaticSteadyEnd=automatic::steadySeconds()-1;allowed=true;assert(!processingAllowed());
 std::cout<<"PASS: automatic first use, daily countdown, exact 14-day boundary, restart, persisted expiry, frozen clock and fail-closed storage\n";
}
