// Linux CI stub: the VST3 processor/controller state test links against
// plugin.cpp, whose controller calls aztec::createEditor(). The real editors
// are macOS (.mm) and Windows (.cpp) only; on Linux no GUI is tested, so this
// keeps the link valid and preserves the documented "no editor" behaviour.
#include "editor.h"
namespace aztec {
Steinberg::IPlugView* createEditor(Steinberg::Vst::EditController* /*controller*/) { return nullptr; }
}
