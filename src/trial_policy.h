#pragma once
#include <algorithm>
#include <cstdint>

namespace grains::trial {
using Tick = std::uint64_t; // Windows FILETIME: 100 ns since 1601, UTC.
constexpr Tick second = 10000000;
constexpr Tick duration = 14ULL * 24 * 60 * 60 * second;
constexpr Tick clockTolerance = 120 * second;
struct State { Tick start = 0, last = 0; };

inline bool wellFormed(const State& state) {
  return state.start != 0 && state.last >= state.start;
}
// Keep a high-water mark. Small NTP corrections never add trial time.
inline bool advance(State& state, Tick now) {
  if (!wellFormed(state) || now == 0) return false;
  if (now < state.last && state.last - now > clockTolerance) return false;
  const Tick effective = std::max(now, state.last);
  state.last = effective;
  return effective - state.start < duration;
}
inline bool merge(const State& a, const State& b, State& result) {
  if (!wellFormed(a) || !wellFormed(b) || a.start != b.start) return false;
  result = {a.start, std::max(a.last, b.last)};
  return true;
}
} // namespace grains::trial
