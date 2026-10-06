#pragma once

#include "module_ui.h"

namespace grains::gui {

class FilterSeqUI : public ModuleUI {
public:
    static constexpr ModuleRect kRect{"FilterSeq", 16.0, 1154.0, 1288.0, 128.0};

    const char* name() const override { return kRect.name; }
    ModuleRect rect() const override { return kRect; }

    void draw() override {
        // FilterSeq drawing logic lives here.
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
