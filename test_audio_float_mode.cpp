#include "src/audio_float_mode.h"
#include <cassert>
#include <cmath>
int main(){
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
 const unsigned original=_mm_getcsr();
 const unsigned host=original & ~0x8040u;
 _mm_setcsr(host);
 {qg::ScopedAudioFloatMode mode;assert((_mm_getcsr()&0x8040u)==0x8040u);
  volatile double tiny=1e-300,factor=1e-20;volatile double result=tiny*factor;assert(result==0.);
  {qg::ScopedAudioFloatMode nested;}assert((_mm_getcsr()&0x8040u)==0x8040u);
 }
 assert(_mm_getcsr()==host);_mm_setcsr(original);
#endif
}
