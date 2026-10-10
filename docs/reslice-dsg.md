# Procedural Reslice

These are original algorithms inspired by the generative breakbeat-cutting
concept described for BBCut/LiveCut. They do not reproduce or incorporate those
projects' source code and are not claimed to be bit-identical implementations.

- CutDSG: straight-grid cuts, bar anchors, repeated fragments and phrase-end rolls.
- WarpDSG: straight/triplet microdurations, reordered history, playback-rate shifts
  and probabilistic reverse.
- PushDSG: a syncopated push/pull skeleton with interruptions and phrase-end fills.

The expanded panel exposes every control family in the supplied LiveCut screenshot.
Controls apply to the original DSG procedures, not the original LiveCut algorithms:

| Section | Controls |
| --- | --- |
| Global | Mode, subdivision, seed, fade, min/max amplitude, pan and pitch, duty, fill duty, min/max phrase |
| CutDSG (CutProc11-inspired) | Min/max repeats, stutter, area |
| WarpDSG (WarpCut-inspired) | Straight, regular, ritard, speed |
| PushDSG (SqPusher-inspired) | Activity |
| Bitcrusher | On/off, min/max bits, min/max sample frequency |
| Comb | On/off, feedforward/feedback type, feedback, min/max delay |

Existing BUFFER, REPEAT CHANCE, VARIATION, FILL CHANCE, REVERSE, MIX and its lock
remain. NEW PHRASE reseeds without randomizing the controls. The panel is taller
and scrolls inside the Modules page; the header and page selectors stay fixed.
Numeric controls retain keyboard entry and host automation.

MIN PHRASE is 1–8 bars; MAX PHRASE selects 1, 2, 4 or 8 bars. A new integer-bar
phrase length is chosen at phrase boundaries. Reversed min/max pairs are sorted.
Amplitude, pan, pitch, repeat budget, crusher resolution/rate and comb delay are
chosen per cut. Pan centre preserves stereo unity. Pitch uses cents. Duty gates
the source; crusher holds and comb tails can continue past that gate. Mode-specific
controls affect their named algorithm only; Activity belongs to PushDSG.

Cut decisions follow host quarter-note position, tempo and time signature, with
bar-boundary alignment. Reads stay within recorded history. Fade controls cut/loop
crossfades; bypass uses a separate short ramp and settles to the original signal.
Brief startup history is required before cutting begins. There is no Manual mode.

The prepared history covers the maximum 16-quarter-note window at 20 BPM.
Stereo reads share their address calculation; power-of-two rings use masking.
The inactive path records history and skips wet processing. Crusher/comb are
skipped when disabled. Processing, automation, reseeding and transport changes
allocate no memory. Reset invalidates ring contents without clearing large arrays.

New parameter IDs are appended. State version 0x5147314B reads all earlier saves.
The preceding procedural release (0x5147314A / 1183-value preset files) retains its
fixed phrase length; new effects default off. Retired manual parameters remain
reserved and hidden. Factory banks initialize the new controls with neutral values.

## Reproducible CPU check

Build `tools/benchmark_reslice.cpp` with C++17 and `-O3`. It processes 30 seconds
of stereo audio at 48 kHz for each mode and block size (64/256/1024), with internal
effects off and on. `cpu_one_core_pct` uses process CPU time, excluding scheduler
wait; p95 is wall time per block divided by its audio deadline. Compare a saved
old header using `-DBASELINE_RESLICE -DRESLICE_HEADER='"/path/to/old/reslice.h"'`.
These measurements isolate Reslice, not DAW load, the whole plugin or GUI rendering.
At 48 kHz the prepared Reslice storage is 32.0625 MiB including comb, compared with
35.1567 MiB previously (about 8.8% less). High sample rates require more storage.

### Measured on the Linux cloud worker (2026-10-10)

Five passes, median process CPU, 48 kHz stereo, 256-sample blocks; one core = 100%. No native GUI or host was running. Absolute timings are machine-dependent.

| Mode | Previous | Expanded |
| --- | ---: | ---: |
| Off | 0.040% | 0.019% |
| CutDSG | 0.083% | 0.070% |
| WarpDSG | 0.079% | 0.072% |
| PushDSG | 0.080% | 0.070% |
| CutDSG + crusher/comb | N/A | 0.127% |
| WarpDSG + crusher/comb | N/A | 0.150% |
| PushDSG + crusher/comb | N/A | 0.155% |

GUI refresh frequency is unchanged; the expanded controls use the existing cached static scene. Native host profiling on the target Intel/Mojave machine remains necessary to assess total plugin/GUI cost.
