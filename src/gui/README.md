# GrainsDosage GUI Module Split (Serum-Style)

This is the modularized GUI layout for GrainsDosage v0.14+. Each section is a self-contained class that can be edited independently.

## Module List

| Module | X | Y | W | H | Purpose |
|--------|---|---|---|---|----------|
| **Granulizer** | 16 | 98 | 416 | 404 | Grain synthesis, size, density, pitch, chaos |
| **PreSlicer** | 452 | 98 | 416 | 404 | Glitch/slice buffer, variation, chance |
| **BeatRepeater** | 888 | 98 | 416 | 404 | 16-step sequencer for rhythmic repeats |
| **Modulation** | 16 | 516 | 836 | 236 | 4 LFO sections with waveform preview |
| **Morph** | 864 | 516 | 440 | 236 | XY pad for morph/stretch morphing |
| **Reslice** | 16 | 766 | 1288 | 108 | 16 re-slice steps with random button |
| **Gater** | 16 | 886 | 1288 | 108 | 16 gate steps (WET/OFF/RELEASE states) |
| **Filter** | 16 | 1006 | 540 | 136 | Filter type, cutoff, resonance, drive |
| **Reverb** | 568 | 1006 | 736 | 136 | Reverb model, mix, length, grid, random |
| **FilterSeq** | 16 | 1154 | 1288 | 128 | 32-step filter sequencer pattern |
| **MasterOut** | 16 | 1294 | 1288 | 56 | Mix, normalize, output meter (24 segments) |

## Canvas Coordinates

- **Logical canvas**: 1320 × 1360 (design space)
- **Physical scaling**: Applied by platform renderers (`editor_mac.mm` / `editor_win.cpp`)
- **All positions**: Fixed in this layout; modules do not reorder except Granulizer/PreSlicer/BeatRepeater (draggable headers)

## Architecture

Each module class:
- Inherits from `ModuleUI` (pure virtual interface)
- Owns its `ModuleRect` (static constexpr)
- Implements `draw()`, `hitTest()`, mouse handlers, and scroll
- Is completely independent; changes to one module don't affect others

The **EditorModuleRack** centralizes:
- All 11 module instances
- A module registry (array of `ModuleUI*`)
- `drawAll()` for rendering the entire editor
- `hitTest()` for dispatching input to the correct module

## How to Customize

### Modify a Single Module

Edit `src/gui/<module_name>_ui.h` and implement:
- `draw()` — Render the panel using your drawing API (HDC on Windows, NSGraphicsContext on macOS)
- `onMouseDown()`, `onMouseDrag()`, `onMouseUp()`, `onScroll()` — Handle user input
- `hitTest()` — Determine if a click is inside this module

Example (Granulizer):

```cpp
void draw() override {
    // Render the Granulizer panel background
    // Draw knobs, labels, readouts, buttons, waveform display
}

bool onMouseDown(double x, double y, bool doubleClick) override {
    // Detect which control was clicked
    // Update parameter values via controller->setParamNormalized()
    return true;  // consumed
}
```

### Add a New Control to a Module

1. Add the parameter ID to `src/parameters.h` (if new DSP)
2. In the module's `draw()`, render the control at its canvas position
3. In `onMouseDown()` / `onMouseDrag()`, detect hits and apply the parameter

### Change Module Positions

Edit the `static constexpr ModuleRect kRect` in each module header. The registry automatically picks up the new positions.

## Integration with Editor

The existing platform editors (`editor_mac.mm` / `editor_win.cpp`) will be refactored to:

1. Create an `EditorModuleRack` instance
2. Call `rack.drawAll()` instead of a monolithic paint function
3. Dispatch mouse events via `rack.hitTest()` to the correct module
4. Forward scroll events to the active module

## Why Split?

- **Maintainability**: Each module is ~100–200 lines; no 1000+ line paint function
- **Reusability**: Modules can be moved, cloned, or extended without side effects
- **Customization**: Users can reskin individual sections by editing one header file
- **Scalability**: Adding new features (e.g., a new sequencer row) affects only one module
- **Serum 2 pattern**: Same architecture Xfer uses for Serum 2's modular UI design

## Files

- `module_ui.h` — Base interface (ModuleUI, ModuleRect)
- `granulizer_ui.h` — Granulizer module
- `preslicer_ui.h` — PreSlicer module
- `beatrepeater_ui.h` — BeatRepeater module
- `modulation_ui.h` — Modulation module
- `morph_ui.h` — Morph XY module
- `reslice_ui.h` — Reslice module
- `gater_ui.h` — Gater module
- `filter_ui.h` — Filter module
- `reverb_ui.h` — Reverb module
- `filterseq_ui.h` — Filter Sequencer module
- `masterout_ui.h` — Master Output module
- `editor_module_rack.h` — Central registry and orchestration

## Next Steps

1. Implement the drawing logic for each module by copying the relevant code from `editor_mac.mm` / `editor_win.cpp`
2. Test each module independently
3. Refactor the platform editors to use `EditorModuleRack` instead of the monolithic draw function
4. Update the asset pipeline to support per-module skin folders
