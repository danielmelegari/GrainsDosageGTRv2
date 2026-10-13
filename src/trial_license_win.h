#pragma once
#include "trial_store_win.h"
#include "trial_license.h"
#ifndef GRAINS_TRIAL_PUBLIC_KEY_HEADER
#error "Configure the publisher public key before building licensed trial targets"
#endif
#include GRAINS_TRIAL_PUBLIC_KEY_HEADER
#include <bcrypt.h>
#include <filesystem>
#include <fstream>

namespace grains::trial {
struct Algorithm {
  BCRYPT_ALG_HANDLE value = nullptr;
  ~Algorithm() { if (value) BCryptCloseAlgorithmProvider(value, 0); }
};
struct CryptoKey {
  BCRYPT_KEY_HANDLE value = nullptr;
  ~CryptoKey() { if (value) BCryptDestroyKey(value); }
};
inline bool sha256(const std::string& input, std::array<BYTE, 32>& hash) {
  Algorithm algorithm;
  if (BCryptOpenAlgorithmProvider(&algorithm.value, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0) return false;
  return BCryptHash(algorithm.value, nullptr, 0,
    reinterpret_cast<PUCHAR>(const_cast<char*>(input.data())), ULONG(input.size()),
    hash.data(), ULONG(hash.size())) >= 0;
}
inline bool signatureValid(const License& license, const BYTE* publicKey = trialPublicKey,
                           ULONG publicKeySize = sizeof(trialPublicKey)) {
  Algorithm algorithm;
  CryptoKey key;
  std::array<BYTE, 32> hash{};
  if (!sha256(license.payload, hash) ||
      BCryptOpenAlgorithmProvider(&algorithm.value, BCRYPT_RSA_ALGORITHM, nullptr, 0) < 0 ||
      BCryptImportKeyPair(algorithm.value, nullptr, BCRYPT_RSAPUBLIC_BLOB, &key.value,
                          const_cast<PUCHAR>(publicKey), publicKeySize, 0) < 0) return false;
  BCRYPT_PKCS1_PADDING_INFO padding{BCRYPT_SHA256_ALGORITHM};
  return BCryptVerifySignature(key.value, &padding, hash.data(), ULONG(hash.size()),
    const_cast<PUCHAR>(license.signature.data()), ULONG(license.signature.size()), BCRYPT_PAD_PKCS1) >= 0;
}
inline std::string machineId() {
  // Both values are local. Raw hardware/Windows identifiers are never exported.
  Key key;
  if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Cryptography", 0,
                   KEY_QUERY_VALUE | KEY_WOW64_64KEY, &key.value) != ERROR_SUCCESS) return {};
  wchar_t guid[128]{}; DWORD type = 0, size = sizeof(guid);
  if (RegQueryValueExW(key.value, L"MachineGuid", nullptr, &type,
                      reinterpret_cast<BYTE*>(guid), &size) != ERROR_SUCCESS ||
      type != REG_SZ || size < 2 || size % sizeof(wchar_t) != 0 || size > sizeof(guid) || guid[size/sizeof(wchar_t)-1] != L'\0') return {};
  wchar_t windows[MAX_PATH]{}, volume[MAX_PATH]{};
  DWORD serial = 0;
  if (!GetWindowsDirectoryW(windows, MAX_PATH) || !GetVolumePathNameW(windows, volume, MAX_PATH) ||
      !GetVolumeInformationW(volume, nullptr, 0, &serial, nullptr, nullptr, nullptr, 0)) return {};
  std::string identity = "GrainsDosage-machine-v1:";
  for (wchar_t c : std::wstring(guid)) {
    if (c > 127) return {};
    identity += char(c >= L'A' && c <= L'Z' ? c + (L'a'-L'A') : c);
  }
  identity += ":" + std::to_string(serial);
  std::array<BYTE, 32> hash{};
  if (!sha256(identity, hash)) return {};
  const char* digits = "0123456789abcdef";
  std::string result;
  for (BYTE b : hash) { result += digits[b >> 4]; result += digits[b & 15]; }
  return result;
}
inline std::wstring licenseDirectory() {
  PWSTR path = nullptr;
  if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, nullptr, &path))) return {};
  std::wstring result(path);
  CoTaskMemFree(path);
  return result + L"\\GrainsDosage-Trial";
}
inline bool verifyLicenseText(const std::string& text, License& license,
                              const std::string& expectedMachine, Tick now, bool checkTime=true,
                              const BYTE* key=trialPublicKey,ULONG keySize=sizeof(trialPublicKey)) {
  License parsed;
  if(!machineIdValid(expectedMachine)||!parseLicense(text,parsed)||
     !signatureValid(parsed,key,keySize)||parsed.machine!=expectedMachine)return false;
  if(checkTime&&!parsed.permanent&&(now<parsed.start||now>=parsed.end))return false;
  license=std::move(parsed);return true;
}
inline bool readLicense(const std::wstring& path, License& license, bool checkTime = true) {
  std::ifstream input(std::filesystem::path(path), std::ios::binary);
  if (!input) return false;
  std::string text(2049, '\0');
  input.read(text.data(), std::streamsize(text.size()));
  text.resize(static_cast<size_t>(input.gcount()));
  return verifyLicenseText(text,license,machineId(),wallTime(),checkTime);
}
inline bool loadLicense(License& license) {
  const auto directory = licenseDirectory();
  return !directory.empty() && readLicense(directory + L"\\trial.license", license);
}
} // namespace grains::trial
