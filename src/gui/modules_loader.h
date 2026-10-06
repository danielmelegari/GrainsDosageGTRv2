#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// GrainsDosage — per-module PNG artwork registry.
//
// The editor GUI is layered as independent visual PNGs, one per module:
//
//   Granulizer.png  PreSlicer.png  BeatRepeater.png  Modulation.png  Morph.png
//   Reslice.png     Gater.png      Filter.png        Reverb.png      FilterSeq.png
//   MasterOut.png
//
// plus the shared GUI element sprites (knob face, slider thumb, step pad,
// header grips, XY nebula) in assets/sprites/ → the runtime skin folder
// (GrainsDosage-skin/knob.png, slider.png, step.png, header-left.png,
// header-right.png, xy-nebula.png). See src/gui/README.md.
//
// This header is the single source of truth for each layer's size and canvas
// position. Both platform editors resolve the layers from the replaceable
// runtime skin folder:
//     <plugin dir>/GrainsDosage-skin/modules/<name>.png   (@2x/ siblings too)
// falling back to the monolithic factory background when a layer is absent,
// so swapping one PNG restyles exactly one module with no rebuild.
//
// Generated artwork lives in the repo at assets/modules/ (1x) and
// assets/modules/@2x/ (retina); regenerate with tools/split_module_skins.py.
// The MODULES table in that tool MUST stay in sync with kModuleImages below.
// ─────────────────────────────────────────────────────────────────────────────
#include <string>
#include <array>

namespace aztec { namespace gui {

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

// Canonical artwork file name for a module layer, e.g. index 0 -> "Granulizer.png".
// The runtime skin folder stores layers lowercase under modules/ (granulizer.png),
// which is case-insensitively equivalent on Windows and mirrors the documented
// Granulizer.png / PreSlicer.png / ... naming on disk for authoring tools.
inline std::string moduleAssetFileName(int index) {
    const ModuleImage* img = getModuleImage(index);
    if (!img || !img->name) return {};
    std::string base(img->name);
    if (!base.empty()) base[0] = char(base[0] - ('a' - 'A'));  // upper-case first letter
    return base + ".png";
}

// Relative path of a module layer inside the replaceable runtime skin folder:
//   GrainsDosage-skin/modules/<name>.png          (1x)
//   GrainsDosage-skin/modules/@2x/<name>.png      (retina)
inline std::string moduleSkinPath(int index, bool retina = false) {
    const ModuleImage* img = getModuleImage(index);
    if (!img || !img->name) return {};
    std::string path("modules/");
    if (retina) path += "@2x/";
    path += img->name;
    path += ".png";
    return path;
}

}  // namespace aztec::gui
