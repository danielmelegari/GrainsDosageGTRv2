#pragma once

#include <array>

#include "module_ui.h"
#include "granulizer_ui.h"
#include "preslicer_ui.h"
#include "beatrepeater_ui.h"
#include "modulation_ui.h"
#include "morph_ui.h"
#include "reslice_ui.h"
#include "gater_ui.h"
#include "filter_ui.h"
#include "reverb_ui.h"
#include "filterseq_ui.h"
#include "masterout_ui.h"

namespace grains::gui {

// Central registry for all 11 GUI modules.
// Each module is self-contained with its own rect, hit-testing, draw, and mouse logic.
// The rack orchestrates them in render order and dispatches input events.
class EditorModuleRack {
public:
    GranulizerUI granulizer;
    PreSlicerUI preslicer;
    BeatRepeaterUI beatRepeater;
    ModulationUI modulation;
    MorphUI morph;
    ResliceUI reslice;
    GaterUI gater;
    FilterUI filter;
    ReverbUI reverb;
    FilterSeqUI filterSeq;
    MasterOutUI masterOut;

    std::array<ModuleUI*, 11> modules = {{
        &granulizer,
        &preslicer,
        &beatRepeater,
        &modulation,
        &morph,
        &reslice,
        &gater,
        &filter,
        &reverb,
        &filterSeq,
        &masterOut,
    }};

    // Draw all modules in order (back to front).
    void drawAll() {
        for (auto* module : modules) {
            if (module) {
                module->draw();
            }
        }
    }

    // Hit-test all modules; return the first one that contains the point.
    ModuleUI* hitTest(double x, double y) {
        for (auto* module : modules) {
            if (module && module->hitTest(x, y)) {
                return module;
            }
        }
        return nullptr;
    }
};

}  // namespace grains::gui
