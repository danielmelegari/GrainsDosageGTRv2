#include "../src/trial_license_win.h"
#include <commdlg.h>
#include <iostream>

int main(int argc, char** argv) {
  using namespace grains::trial;
  if (argc == 2 && std::string(argv[1]) == "--machine-id") {
    const auto id = machineId();
    std::cout << id << '\n';
    return id.empty() ? 1 : 0;
  }
  auto date = [](Tick tick) {
    FILETIME ft{DWORD(tick), DWORD(tick >> 32)};
    SYSTEMTIME utc{}; FileTimeToSystemTime(&ft, &utc);
    wchar_t buffer[64]{};
    swprintf(buffer, sizeof(buffer)/sizeof(*buffer), L"%04u-%02u-%02u %02u:%02u UTC", utc.wYear, utc.wMonth, utc.wDay, utc.wHour, utc.wMinute);
    return std::wstring(buffer);
  };
  License installed;
  const auto installedPath=licenseDirectory()+L"\\trial.license";
  std::wstring status=L"No valid license is installed for this computer.";
  if(readLicense(installedPath,installed,false)){
    const auto now=wallTime();
    if(installed.permanent)status=L"Permanent license installed for this computer. License ID: GDL2-"+std::wstring(installed.licenseId.begin(),installed.licenseId.end());
    else {status=(now<installed.start?L"Trial has not started.":now>=installed.end?L"Trial has expired.":L"Trial is within its signed 14-day period.");
    status+=L"\nStarts: "+date(installed.start)+L"\nExpires: "+date(installed.end);}
  }
  if(argc==2&&std::string(argv[1])=="--status"){std::wcout<<status<<L'\n';return 0;}
  const std::wstring prompt=status+L"\n\n"+
    L"Choose Yes to save a machine registration request on your Desktop. Send that file to the publisher.\n\n"
    L"Choose No to install the signed TXT license file you received from the publisher.\n\n"
    L"The trial lasts 14 calendar days from its signed start, with no one-hour session limit.";
  const int choice = MessageBoxW(nullptr, prompt.c_str(),
    L"GrainsDosage Offline Trial Registration", MB_YESNOCANCEL | MB_ICONINFORMATION);
  if (choice == IDCANCEL) return 0;
  if (choice == IDYES) {
    const auto id = machineId();
    PWSTR desktop = nullptr;
    if (id.empty() || FAILED(SHGetKnownFolderPath(FOLDERID_Desktop, KF_FLAG_CREATE, nullptr, &desktop))) return 1;
    std::filesystem::path request = std::filesystem::path(desktop) / L"GrainsDosage-machine-request.json";
    CoTaskMemFree(desktop);
    std::ofstream output(request, std::ios::binary | std::ios::trunc);
    output << "{\n  \"product\": \"GrainsDosage-Trial-v1\",\n  \"machine_id\": \"" << id << "\"\n}\n";
    output.close();
    MessageBoxW(nullptr, output ? L"Saved GrainsDosage-machine-request.json to your Desktop. Send it to the publisher."
                               : L"Could not write the request file.", L"GrainsDosage", MB_OK);
    return output ? 0 : 1;
  }
  wchar_t path[32768]{};
  OPENFILENAMEW dialog{};
  dialog.lStructSize = sizeof(dialog);
  dialog.lpstrFilter = L"GrainsDosage licenses (*.txt;*.license)\0*.txt;*.license\0\0";
  dialog.lpstrFile = path;
  dialog.nMaxFile = 32768;
  dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
  if (!GetOpenFileNameW(&dialog)) return 0;
  License license;
  // A future-dated signed license can be installed before its start date.
  if (!readLicense(path, license, false) || (!license.permanent && wallTime() >= license.end)) {
    MessageBoxW(nullptr, L"License is invalid, expired, or registered to another computer.", L"GrainsDosage", MB_OK | MB_ICONERROR);
    return 1;
  }
  const auto directory = licenseDirectory();
  if (directory.empty() || (!CreateDirectoryW(directory.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS)) return 1;
  const auto destination = directory + L"\\trial.license";
  // Allow choosing the already installed file without overwriting itself.
  if (std::filesystem::path(path) != std::filesystem::path(destination) && !CopyFileW(path, destination.c_str(), FALSE)) return 1;
  const std::wstring message = license.permanent ? L"Permanent license installed for this computer. Reload the plugin in your DAW." : L"License installed for this computer.\n\nStarts: " + date(license.start) +
    L"\nExpires: " + date(license.end) + L"\n\nReload the plugin in your DAW.";
  MessageBoxW(nullptr, message.c_str(), L"GrainsDosage", MB_OK | MB_ICONINFORMATION);
  return 0;
}
