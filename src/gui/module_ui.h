#pragma once

#include <array>
#include <cstdint>

namespace grains::gui {

struct ModuleRect {
    const char* name;
    double x;
    double y;
    double w;
    double h;
};

class ModuleUI {
public:
    virtual ~ModuleUI() = default;

    virtual const char* name() const = 0;
    virtual ModuleRect rect() const = 0;

    virtual void draw() = 0;
    virtual bool hitTest(double x, double y) const = 0;
    virtual bool onMouseDown(double x, double y, bool doubleClick = false) = 0;
    virtual bool onMouseDrag(double x, double y) = 0;
    virtual void onMouseUp() = 0;
    virtual bool onScroll(double x, double y, int delta) = 0;
};

}  // namespace grains::gui
