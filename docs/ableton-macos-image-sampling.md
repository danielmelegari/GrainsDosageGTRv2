# macOS image sampling bottleneck — 0.13.18

User capture: Live 11.3.30, macOS 10.14.6 Intel, 2026-10-22 13:37:33,
GrainsDosage VST3 0.13.17 UUID F68F2623-2DB9-3980-AEB6-18CBD99BE735.

Of 6312 main-thread samples, 4213 are below a GrainsDosage call to
NSImage drawInRect, including 1998 in img_interpolate_read and extensive
colour conversion. This identifies an actual drawing bottleneck during this
capture; it does not measure input-to-display latency or explain every audio glitch.

The static scene previously used NSImage lockFocus at logical canvas size and
was scaled to the viewport during each repaint. Replace that backing with an
explicit 8-bit bitmap sized to viewport scale times window backing scale. Render
static content and control patches into this bitmap; draw its CGImage directly
with interpolation disabled. Rebuild on pixel-dimension changes. Dynamic drawing
and parameter notification semantics are unchanged.

Existing Linux editor_refresh, gui_layout and ui_motion tests pass. They do not
compile or exercise this Objective-C++ path. Native macOS build/editor checks
are delegated to CI. Actual Ableton responsiveness and colour/orientation on
Mojave remain to be verified; no assertion that all lag is fixed.
