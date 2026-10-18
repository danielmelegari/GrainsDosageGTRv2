#pragma once
#include "public.sdk/source/vst/vsteditcontroller.h"
#include <atomic>
#include <memory>
#include <string>
namespace aztec {
// UI-only state owned by the controller, which outlives individual windows.
// Each plugin instance gets its own state; no global preset selection is shared.
struct EditorState { std::string presetName="PRESETS ▾"; };
Steinberg::IPlugView* createEditor(Steinberg::Vst::EditController* controller,
                                 std::shared_ptr<EditorState> state = {});
// Lock-free transport for the audio thread's live modulation magnitudes. The
// Processor publishes its engine snapshot into this shared cell every process
// block; the editors read it at ~30fps to drive the knob modulation bars.
struct ModCell { std::atomic<uint64_t> packed{0}; };
inline ModCell& modCell() { static ModCell c; return c; }
}
