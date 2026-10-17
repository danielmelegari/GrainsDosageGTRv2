# Resource lifecycle review — 2026-10-17

Reported environment: macOS 10.14, Cubase 12 Pro, Full Unlocked edition. The host becomes progressively slow and recovers only after exit. Sample rate, buffer size, RAM, instance count and a process sample are not yet available.

## Findings

- Full Unlocked does not compile the commercial license manager. Its periodic worker cannot explain this report. No background audio worker is created by the engine.
- Cocoa editor removal calls `stop`, invalidates and clears its NSTimer, clears the owner pointer, removes the view, and releases the editor's strong reference. The text cache is capped; immutable image slices are reused. Source inspection alone does not establish that every native resource is actually released by macOS.
- `setActive(false)` currently leaves engine buffers allocated. They are owned by the processor and destroyed with it. If Cubase retains inactive objects for undo or other host purposes, these buffers remain resident. This is retention, not proof of a leak. Process sampling and host memory measurements are needed before changing this behavior.
- Prepare reserves buffers for the maximum supported history even with modules disabled or short buffer selections. Memory therefore scales with sample rate and instance count.
- The existing buffer test verifies that buffer/routing/quality changes allocate or release no C++ heap blocks on the processing path.
- A new repeated-lifetime test warms the shared wave tables, then creates, prepares, processes, prepares again and destroys an engine four times at each of 44.1, 48, 96 and 192 kHz. All watched C++ allocations are balanced by releases in each cycle. This does not measure native graphics allocations, host retention, actual RSS or thread CPU.
- A new macOS smoke scenario repeatedly attaches, paints and removes editors, then checks a weak reference after draining an autorelease pool. A surviving timer/view retention cycle fails the test. This requires a native macOS runner; it cannot be executed in the Linux workspace.
- Random-wave monitor changes currently invalidate the full Cocoa static scene. This may increase redraw cost while the editor is visible, but it does not establish the cause of persistent slowdown after removal. No rendering or sound behavior was changed during this review.

## Local allocation measurements

| Sample rate | Initial engine allocations | Allocations / releases after destruction |
| --- | ---: | ---: |
| 44.1 kHz | 92,203,752 bytes | 48 / 48 |
| 48 kHz | 97,338,224 bytes | 48 / 48 |
| 96 kHz | 194,151,736 bytes | 48 / 48 |
| 192 kHz | 387,778,744 bytes | 48 / 48 |

The second prepare at the same rate adds no allocations. These are tracked C++ heap requests, not Cubase RSS or a complete memory footprint. The shared immutable wave table was warmed before measurement.

## Next measurement on the affected Mac

Record Cubase CPU, memory, thread count, macOS memory pressure and swap at four points: before inserting the plugin, while playing, after closing the plugin window, and after removing the insert. Note sample rate, audio buffer size and instance count. Closing the window separately distinguishes rendering cost from audio processing.

While the slowdown is present, use Activity Monitor's Sample Process for Cubase and save the result before quitting. Do not keep an unusable session running just to collect a sample. A sample is needed to identify the active functions; allocation balance in an isolated engine test cannot prove the Cubase problem is resolved.

This review and its regression tests do not claim a fix for the reported freeze. Audio algorithms, GUI appearance, licensing behavior and processor deactivation behavior are unchanged.
