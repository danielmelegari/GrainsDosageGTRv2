#pragma once
#include "public.sdk/source/vst/vsteditcontroller.h"
namespace aztec {
Steinberg::IPlugView* createEditor(Steinberg::Vst::EditController* controller);
}
