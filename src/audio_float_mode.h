#pragma once
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <xmmintrin.h>
#endif
namespace qg {
// Decaying DSP state can become subnormal during silence. Keep that work fast
// on x86 audio threads, restoring the host's floating-point mode on every exit.
class ScopedAudioFloatMode {
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
 unsigned saved_;
public:
 ScopedAudioFloatMode() : saved_(_mm_getcsr()) { _mm_setcsr(saved_ | 0x8040u); }
 ~ScopedAudioFloatMode() { _mm_setcsr(saved_); }
#else
public:
 ScopedAudioFloatMode() = default;
#endif
 ScopedAudioFloatMode(const ScopedAudioFloatMode&) = delete;
 ScopedAudioFloatMode& operator=(const ScopedAudioFloatMode&) = delete;
};
}
