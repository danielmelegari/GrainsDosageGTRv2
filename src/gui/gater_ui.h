#pragma once

#include "module_ui.h"

namespace grains::gui {

class GaterUI : public ModuleUI {
public:
    static constexpr ModuleRect kRect{"Gater", 16.0, 886.0, 1288.0, 108.0};

    const char* name() const override { return kRect.name; }
    ModuleRect rect() const override { return kRect; }

    void draw() override {
        // Gater drawing logic lives here.
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
