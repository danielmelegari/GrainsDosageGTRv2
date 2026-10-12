#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincrypt.h>
#include <shlobj.h>
#include <array>
#include <cstring>
#include <string>
#include <vector>
#include "trial_policy.h"

namespace grains::trial {
inline Tick wallTime() {
  FILETIME time{};
  GetSystemTimeAsFileTime(&time);
  return (Tick(time.dwHighDateTime) << 32) | time.dwLowDateTime;
}
struct Handle {
  HANDLE value = INVALID_HANDLE_VALUE;
  ~Handle() { if (value != INVALID_HANDLE_VALUE) CloseHandle(value); }
};
struct Key {
  HKEY value = nullptr;
  ~Key() { if (value) RegCloseKey(value); }
};
struct LocalBuffer {
  DATA_BLOB blob{};
  ~LocalBuffer() { if (blob.pbData) LocalFree(blob.pbData); }
};
enum class Read { missing, valid, invalid };

// DPAPI authenticates the record and binds it to the current Windows account.
// Entropy identifies the product; it is deliberately not treated as a secret.
inline DATA_BLOB entropy() {
  static BYTE bytes[] = "GrainsDosage/WindowsTrial/v2";
  return {sizeof(bytes), bytes};
}
inline bool protect(const State& state, std::vector<BYTE>& bytes) {
  std::array<Tick, 3> record{0x4752545249414c32ULL, state.start, state.last};
  DATA_BLOB input{DWORD(sizeof(record)), reinterpret_cast<BYTE*>(record.data())};
  auto extra = entropy();
  LocalBuffer output;
  if (!CryptProtectData(&input, L"GrainsDosage trial", &extra, nullptr, nullptr,
                        CRYPTPROTECT_UI_FORBIDDEN, &output.blob)) return false;
  bytes.assign(output.blob.pbData, output.blob.pbData + output.blob.cbData);
  return true;
}
inline bool unprotect(std::vector<BYTE>& bytes, State& state) {
  DATA_BLOB input{DWORD(bytes.size()), bytes.data()};
  auto extra = entropy();
  LocalBuffer output;
  if (!CryptUnprotectData(&input, nullptr, &extra, nullptr, nullptr,
                          CRYPTPROTECT_UI_FORBIDDEN, &output.blob)) return false;
  std::array<Tick, 3> record{};
  if (output.blob.cbData != sizeof(record)) return false;
  std::memcpy(record.data(), output.blob.pbData, sizeof(record));
  if (record[0] != 0x4752545249414c32ULL) return false;
  state = {record[1], record[2]};
  return wellFormed(state);
}

class Store {
  std::wstring registryPath_, directory_;
  static Read readRegistry(HKEY key, State& state) {
    DWORD size = 0, type = 0;
    const auto status = RegQueryValueExW(key, L"LicensedStateV1", nullptr, &type, nullptr, &size);
    if (status == ERROR_FILE_NOT_FOUND) return Read::missing;
    if (status != ERROR_SUCCESS || type != REG_BINARY || size == 0 || size > 4096) return Read::invalid;
    std::vector<BYTE> bytes(size);
    if (RegQueryValueExW(key, L"LicensedStateV1", nullptr, &type, bytes.data(), &size) != ERROR_SUCCESS)
      return Read::invalid;
    bytes.resize(size);
    return unprotect(bytes, state) ? Read::valid : Read::invalid;
  }
  static Read readFile(HANDLE file, State& state) {
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file, &size)) return Read::invalid;
    if (size.QuadPart == 0) return Read::missing;
    if (size.QuadPart < 0 || size.QuadPart > 4096) return Read::invalid;
    std::vector<BYTE> bytes(static_cast<size_t>(size.QuadPart));
    DWORD count = 0;
    LARGE_INTEGER zero{};
    if (!SetFilePointerEx(file, zero, nullptr, FILE_BEGIN) ||
        !ReadFile(file, bytes.data(), DWORD(bytes.size()), &count, nullptr) || count != bytes.size())
      return Read::invalid;
    return unprotect(bytes, state) ? Read::valid : Read::invalid;
  }

public:
  // Explicit paths also allow tests to use isolated state, never the real trial.
  Store(std::wstring registryPath, std::wstring directory)
    : registryPath_(std::move(registryPath)), directory_(std::move(directory)) {}
  static Store production() {
    PWSTR path = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, nullptr, &path)))
      return Store(L"", L"");
    std::wstring directory(path);
    CoTaskMemFree(path);
    return Store(L"Software\\GrainsDosage\\Trial", directory + L"\\GrainsDosage-Trial");
  }
  bool refresh(Tick now, State& state, bool allowCreate, Tick minimumTime = 0) const {
    if (directory_.empty() || registryPath_.empty()) return false;
    if (!CreateDirectoryW(directory_.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) return false;
    Handle file;
    file.value = CreateFileW((directory_ + L"\\licensed-state.dat").c_str(), GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file.value == INVALID_HANDLE_VALUE) return false;
    // Serialize plugin instances and DAW processes, including different sessions.
    // Closing the handle releases the lock on every return path.
    OVERLAPPED overlapped{};
    bool locked = false;
    for (int attempt = 0; attempt < 500; ++attempt) {
      if (LockFileEx(file.value, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY,
                     0, MAXDWORD, MAXDWORD, &overlapped)) { locked = true; break; }
      if (GetLastError() != ERROR_LOCK_VIOLATION) return false;
      Sleep(10);
    }
    if (!locked) return false;
    Key key;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, registryPath_.c_str(), 0, nullptr, 0,
                       KEY_QUERY_VALUE | KEY_SET_VALUE, nullptr, &key.value, nullptr) != ERROR_SUCCESS) return false;
    State registryState{}, fileState{}, next{};
    const auto registryRead = readRegistry(key.value, registryState);
    const auto fileRead = readFile(file.value, fileState);
    if (registryRead == Read::missing && fileRead == Read::missing) {
      if (!allowCreate || !wellFormed(state)) return false;
      next = state;
    } else if (registryRead != Read::valid || fileRead != Read::valid ||
               !merge(registryState, fileState, next)) return false;
    if (next.start != state.start || now < state.start) return false;
    // Persist the high-water mark even when time has expired, so rolling the
    // clock back after a forward jump cannot reactivate an expired trial.
    bool active = advance(next, now);
    if (active && minimumTime > next.last) active = advance(next, minimumTime);
    std::vector<BYTE> bytes;
    if (!protect(next, bytes)) return false;
    LARGE_INTEGER zero{};
    DWORD count = 0;
    if (!SetFilePointerEx(file.value, zero, nullptr, FILE_BEGIN) ||
        !WriteFile(file.value, bytes.data(), DWORD(bytes.size()), &count, nullptr) || count != bytes.size() ||
        !SetEndOfFile(file.value) || !FlushFileBuffers(file.value)) return false;
    if (RegSetValueExW(key.value, L"LicensedStateV1", 0, REG_BINARY, bytes.data(), DWORD(bytes.size())) != ERROR_SUCCESS)
      return false;
    state = next;
    return active;
  }
};
} // namespace grains::trial
