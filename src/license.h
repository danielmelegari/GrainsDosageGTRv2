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
#ifdef GRAINS_COMMERCIAL
inline std::atomic<bool> allowed{false};
void initialize();
void firstOpen();
std::string status();
bool activate(const std::string&,std::string& error);
#else
inline std::atomic<bool> allowed{true};
inline void initialize(){}
inline void firstOpen(){}
inline std::string status(){return {};}
inline bool activate(const std::string&,std::string&){return false;}
#endif
}
