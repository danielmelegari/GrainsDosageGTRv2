#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <chrono>
#include <cstdint>

namespace grains {
// Per-user trial starts on first plugin load. Registry I/O stays off the audio thread.
class TrialGuard {
  std::chrono::steady_clock::time_point deadline_{};
public:
  TrialGuard() {
    constexpr std::uint64_t duration = 14ULL * 24 * 60 * 60 * 10000000;
    FILETIME ft{};
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER timestamp{};
    timestamp.LowPart=ft.dwLowDateTime; timestamp.HighPart=ft.dwHighDateTime;
    const auto now=timestamp.QuadPart;
    HKEY key{}; DWORD disposition{};
    if(RegCreateKeyExW(HKEY_CURRENT_USER,L"Software\\GrainsDosage\\Trial",0,nullptr,0,
                      KEY_QUERY_VALUE|KEY_SET_VALUE,nullptr,&key,&disposition)!=ERROR_SUCCESS) return;
    std::uint64_t start=now, last=now; DWORD type{}, size=sizeof(start);
    bool valid=true;
    if(disposition==REG_CREATED_NEW_KEY) {
      valid=RegSetValueExW(key,L"Start",0,REG_QWORD,reinterpret_cast<const BYTE*>(&start),sizeof(start))==ERROR_SUCCESS;
    } else {
      valid=RegQueryValueExW(key,L"Start",nullptr,&type,reinterpret_cast<BYTE*>(&start),&size)==ERROR_SUCCESS
        && type==REG_QWORD && size==sizeof(start);
      size=sizeof(last);
      valid=valid && RegQueryValueExW(key,L"LastSeen",nullptr,&type,reinterpret_cast<BYTE*>(&last),&size)==ERROR_SUCCESS
        && type==REG_QWORD && size==sizeof(last);
    }
    valid=valid && now>=start && now>=last && now-start<duration;
    if(valid) valid=RegSetValueExW(key,L"LastSeen",0,REG_QWORD,reinterpret_cast<const BYTE*>(&now),sizeof(now))==ERROR_SUCCESS;
    RegCloseKey(key);
    if(valid) deadline_=std::chrono::steady_clock::now()+std::chrono::seconds((duration-(now-start))/10000000);
  }
  bool expired() const { return std::chrono::steady_clock::now()>=deadline_; }
};
}
