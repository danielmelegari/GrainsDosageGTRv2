#pragma once

#include <string>
#include <array>

namespace grains::gui {

// Module PNG metadata: dimensions and canvas position
struct ModuleImage {
    const char* name;      // e.g., "granulizer"
    int width;             // e.g., 416
    int height;            // e.g., 404
    int canvasX;           // e.g., 16
    int canvasY;           // e.g., 98
};

static constexpr std::array<ModuleImage, 11> kModuleImages = {{
    {"granulizer",  416, 404, 16,   98},
    {"preslicer",   416, 404, 452,  98},
    {"beatrepeater", 416, 404, 888,  98},
    {"modulation",  836, 236, 16,  516},
    {"morph",       440, 236, 864, 516},
    {"reslice",    1288, 108, 16,  766},
    {"gater",      1288, 108, 16,  886},
    {"filter",      540, 136, 16, 1006},
    {"reverb",      736, 136, 568, 1006},
    {"filterseq",  1288, 128, 16, 1154},
    {"masterout",  1288,  56, 16, 1294},
}};

// Helper: get module image metadata by index or name
const ModuleImage* getModuleImage(int index) {
    if (index >= 0 && index < (int)kModuleImages.size()) {
        return &kModuleImages[index];
    }
    return nullptr;
}

const ModuleImage* getModuleImageByName(const char* name) {
    for (const auto& img : kModuleImages) {
        if (img.name && std::string(img.name) == name) {
            return &img;
        }
    }
    return nullptr;
}

}  // namespace grains::gui
