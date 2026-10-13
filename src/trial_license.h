#pragma once
#include "trial_policy.h"
#include <array>
#include <charconv>
#include <string>
#include <string_view>
#include <vector>

namespace grains::trial {
struct License {
  std::string machine;
  bool permanent = false;
  std::string licenseId;
  Tick start = 0, end = 0;
  std::string payload;
  std::vector<unsigned char> signature;
};
inline int hexDigit(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}
inline bool machineIdValid(std::string_view id) {
  if (id.size() != 64) return false;
  for (char c : id) if (hexDigit(c) < 0) return false;
  return true;
}
inline bool parseLicense(const std::string& text, License& license) {
  if (text.size() > 2048) return false;
  std::array<std::string_view, 5> lines{};
  size_t pos = 0, payloadEnd = 0;
  for (size_t i = 0; i < lines.size(); ++i) {
    const auto end = text.find('\n', pos);
    if (end == std::string::npos) return false;
    lines[i] = std::string_view(text).substr(pos, end - pos);
    pos = end + 1;
    if (i == 3) payloadEnd = pos;
  }
  if (pos != text.size() || (lines[0] != "GRAINS-TRIAL-1" && lines[0] != "GRAINS-PERMANENT-1") || !machineIdValid(lines[1])) return false;
  License result;
  result.machine = std::string(lines[1]);
  auto number = [](std::string_view s, Tick& n) {
    if (s.empty()) return false;
    const auto r = std::from_chars(s.data(), s.data() + s.size(), n);
    return r.ec == std::errc{} && r.ptr == s.data() + s.size();
  };
  if (lines[0] == "GRAINS-PERMANENT-1") {
    if (lines[2].size()!=32 || lines[3]!="3") return false;
    for(char c:lines[2]) if(hexDigit(c)<0) return false;
    result.permanent=true;result.licenseId=std::string(lines[2]);
  } else if (!number(lines[2], result.start) || !number(lines[3], result.end) || result.start == 0 ||
      result.end <= result.start || result.end - result.start != duration) return false;
  if(lines[4].size()!=512) return false;
  result.signature.resize(256);
  for (size_t i = 0; i < result.signature.size(); ++i) {
    int a = hexDigit(lines[4][2*i]), b = hexDigit(lines[4][2*i+1]);
    if (a < 0 || b < 0) return false;
    result.signature[i] = static_cast<unsigned char>((a << 4) | b);
  }
  result.payload = text.substr(0, payloadEnd);
  license = std::move(result);
  return true;
}
} // namespace grains::trial
