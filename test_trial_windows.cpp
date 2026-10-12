#include "src/trial_license_win.h"
#include <cassert>
#include <thread>
#include <iostream>

int main() {
  using namespace grains::trial;
  std::ifstream input("tests/trial_test.license", std::ios::binary);
  std::string text((std::istreambuf_iterator<char>(input)), {});
  License license;
  assert(parseLicense(text, license));
  assert(signatureValid(license));
  license.payload[20] ^= 1;
  assert(!signatureValid(license));
  assert(parseLicense(text, license));
  license.signature[10] ^= 1;
  assert(!signatureValid(license));
  assert(machineIdValid(machineId()));
  assert(machineId() == machineId());
  assert(!readLicense(L"tests/trial_test.license", license, false)); // Wrong machine.
  State source{130000000000000000ULL,130000000010000000ULL}, restored{};
  std::vector<BYTE> bytes;
  assert(protect(source,bytes));
  assert(unprotect(bytes,restored));
  assert(restored.start == source.start && restored.last == source.last);
  bytes[bytes.size()/2] ^= 1;
  assert(!unprotect(bytes,restored));

  wchar_t temp[MAX_PATH]{};
  assert(GetTempPathW(MAX_PATH,temp));
  const auto suffix = std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64());
  const auto directory = std::wstring(temp) + L"GrainsTrialTest-" + suffix;
  const auto registry = L"Software\\GrainsDosageTests\\" + suffix;
  Store store(registry, directory);
  const Tick start = wallTime();
  State state{start,start};
  assert(store.refresh(start,state,true));
  assert(store.refresh(start+second,state,false));
  bool okA = false, okB = false;
  auto concurrent = [&](bool& ok) { State copy{start,start}; ok = store.refresh(start+2*second,copy,true); };
  std::thread a(concurrent,std::ref(okA)), b(concurrent,std::ref(okB)); a.join(); b.join();
  assert(okA && okB);
  assert(store.refresh(start+300*second,state,false));
  assert(!store.refresh(start,state,false)); // Clock rollback.
  assert(store.refresh(start+301*second,state,false));
  assert(!store.refresh(start+duration,state,false));
  State reloaded{start,start};
  assert(!store.refresh(start+duration-1,reloaded,true)); // Persisted expiration.
  assert(DeleteFileW((directory+L"\\licensed-state.dat").c_str()));
  reloaded={start,start};
  assert(!store.refresh(start+second,reloaded,true)); // Deleting one copy does not reset.
  assert(RegDeleteTreeW(HKEY_CURRENT_USER,registry.c_str()) == ERROR_SUCCESS);
  // Both copies removed still uses the signed start supplied by the caller.
  assert(DeleteFileW((directory+L"\\licensed-state.dat").c_str()));
  reloaded={start,start};
  assert(!store.refresh(start+duration,reloaded,true));
  RegDeleteTreeW(HKEY_CURRENT_USER,registry.c_str());
  std::filesystem::remove_all(directory);
  std::cout << "Windows signature, machine identity, DPAPI, persistence and concurrent-instance tests passed\n";
}
