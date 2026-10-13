#pragma once
#include <atomic>
#include <cstdint>
#include <string>
namespace aztec::license {
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
inline constexpr bool activationRequired=true;
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
