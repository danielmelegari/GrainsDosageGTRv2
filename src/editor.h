#pragma once
#include "public.sdk/source/vst/vsteditcontroller.h"
#include <atomic>
namespace aztec {
Steinberg::IPlugView* createEditor(Steinberg::Vst::EditController* controller);
// Lock-free transport for the audio thread's live modulation magnitudes. The
// Processor publishes its engine snapshot into this shared cell every process
// block; the editors read it at ~30fps to drive the knob modulation bars.
struct ModCell { std::atomic<uint64_t> packed{0}; };
inline ModCell& modCell() { static ModCell c; return c; }
}
