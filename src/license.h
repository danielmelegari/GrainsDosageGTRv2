#pragma once
#include <atomic>
#include <cstdint>
#include <string>
#include <chrono>
namespace aztec::license {
inline std::atomic<int64_t> automaticEnd{0},automaticSteadyEnd{0};
constexpr int64_t trialSeconds=14*24*60*60;
enum class State { Ready, Trial, Expired, Licensed, Required, StorageError, ClockError };
inline State evaluate(int64_t first,int64_t last,int64_t now) {
 if(first<=0||last<first)return State::StorageError;
 if(now+300<last||now<first)return State::ClockError;
 return now-first>=trialSeconds?State::Expired:State::Trial;
}
#if defined(GRAINS_COMMERCIAL) && defined(GRAINS_UNLOCKED)
#error "Unlocked builds cannot include commercial activation"
#endif
#ifdef GRAINS_COMMERCIAL
#if defined(GRAINS_TRIAL) && GRAINS_TRIAL
inline constexpr bool activationRequired=false;
#else
inline constexpr bool activationRequired=true;
#endif
inline std::atomic<bool> allowed{false};
std::string deviceCode();
void initialize();
void firstOpen();
std::string status();
bool activate(const std::string&,std::string& error);
#else
inline constexpr bool activationRequired=false;
inline std::atomic<bool> allowed{true};
inline std::string deviceCode(){return {};}
inline void initialize(){}
inline void firstOpen(){}
inline std::string status(){
#ifdef GRAINS_UNLOCKED
 return "LICENSE ACTIVE - UNLOCKED";
#else
 return {};
#endif
}
inline bool activate(const std::string&,std::string&){return false;}
#endif
}

namespace aztec::license {
inline bool processingAllowed(){
 if(!allowed.load(std::memory_order_acquire))return false;
#if defined(GRAINS_TRIAL) && GRAINS_TRIAL
 auto wall=std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
 auto steady=std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
 if(wall>=automaticEnd.load()||steady>=automaticSteadyEnd.load()){allowed.store(false,std::memory_order_release);return false;}
#endif
 return true;
}
}
