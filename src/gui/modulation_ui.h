#pragma once

#include "module_ui.h"

namespace grains::gui {

class ModulationUI : public ModuleUI {
public:
    static constexpr ModuleRect kRect{"Modulation", 16.0, 516.0, 836.0, 236.0};

    const char* name() const override { return kRect.name; }
    ModuleRect rect() const override { return kRect; }

    void draw() override {
        // Modulation drawing logic lives here.
    }

    bool hitTest(double x, double y) const override {
        return x >= kRect.x && x <= kRect.x + kRect.w &&
               y >= kRect.y && y <= kRect.y + kRect.h;
    }

    bool onMouseDown(double x, double y, bool doubleClick = false) override {
        (void)x; (void)y; (void)doubleClick;
        return true;
    }

    bool onMouseDrag(double x, double y) override {
        (void)x; (void)y;
        return true;
    }

    void onMouseUp() override {}

    bool onScroll(double x, double y, int delta) override {
        (void)x; (void)y; (void)delta;
        return true;
    }
};

}  // namespace grains::gui
