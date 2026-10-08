# Approved rack artwork

Backplate and transparent control atlas derived from the user-approved October 8, 2026 visual reference. Native Cocoa and GDI+ render these PNG resources; control labels, needles, waveforms, steps and values are live.

- `backplate.png`: 1083 × 1452 pixels; fixed panel geometry and headings.
- `controls.png`: 1254 × 1254 pixels; source rectangles are defined by `spriteRect` in `src/mockup_ui.h`.
- Editor geometry uses the backplate pixel coordinates. Default window scale is 70%; Size permits 50–100%.
- Artwork is bundled under distinct resource names to avoid stale user skin assets overriding this design.

The shared offline SVG renderer embeds the same artwork. Native smoke tests verify resource loading, interactions, routing, locks, and capture all five module views.

The Cocoa editor caches static artwork and text. Monitor changes paint the live layer at up to 30 Hz, and repainting stops after animation settles or the window is hidden. Resizes always use one uniform scale for drawing and hit testing, even when a host restores a stale aspect ratio. Module selection is controller-local; only routing edits enter host automation.
