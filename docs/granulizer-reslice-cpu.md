# Granulizer + Reslice CPU and Seed edits — 0.13.15

Granulizer previously repeated read-window wrapping, index calculation and
interpolation-position calculation for the left and right channel of every
active grain. Stereo reads now share those calculations. Positions already in
the physical ring skip fmod, and the adjacent index uses a conditional wrap.
The per-channel interpolation arithmetic, grain windows, voice limit, resampling,
modulation rate and effect algorithms are unchanged. The audio path allocates
no new storage. This covers both serial and legacy parallel routing and freeze.

## Local comparison

Baseline: 8875771 (including the user's compact Reslice and latest UI changes).
Linux, g++ -O3 -std=c++17, same machine/compiler, median of three alternating
runs. These are engine-only CPU percentages of one core, not Cubase's meter.
Block p99 is wall time, including settings updates and output hashing; scheduler
preemption affects it. The maximum observed block time is reported by the
benchmark but is too noisy for a real-time performance guarantee.

| Case | Sample rate | Block | Before CPU | After CPU | Reduction | Before/after p99 µs |
|---|---:|---:|---:|---:|---:|---:|
| Dense synchronized grains + CutDSG | 48000 | 256 | 11.246% | 7.276% | 35.3% | 1126.60 / 612.77 |
| Dense flowing grains + WarpDSG | 48000 | 64 | 20.282% | 12.861% | 36.6% | 476.38 / 356.79 |
| Dense flowing grains + PushDSG, crusher/comb | 48000 | 64 | 20.248% | 12.523% | 38.2% | 488.75 / 278.42 |
| Dense flowing grains + CutDSG, crusher/comb | 96000 | 64 | 42.514% | 24.790% | 41.7% | 539.40 / 262.81 |
| Freeze, window changes, ring wrap, serial/parallel | 8000 | 64 | 6.103% | 3.337% | 45.3% | 848.90 / 419.54 |

All output hashes matched across before/after runs. The first four cases render
six seconds each; the fifth renders forty seconds to exercise physical ring
wrapping. Exact comparison covers these scenarios, not every possible preset.
The user's Cubase preset/rate/buffer were unknown, so on-device peak behavior
still needs verification.

Reproduce by building tools/benchmark_granulizer_reslice.cpp against each source
tree (`g++ -std=c++17 -O3 -I/path/to/src ... -o benchmark`) and running
`python tools/compare_granulizer_reslice.py BEFORE_EXE AFTER_EXE`.

## Seed edit discontinuities

Seed changes used to flag an immediate cut restart, truncating a nonzero sample
when dragging the control. A seed-only edit now preserves the current cursor,
envelope and cut deadline. The newest seed is used at the next scheduled cut,
also for New Phrase. Multiple edits before the boundary collapse to the latest
seed. Timing therefore depends on the current cut length. No additional
smoothing or reduction in sound quality is applied to steady-state audio.

Regression tests check rapid seed automation in all three algorithms at 8, 48
and 96 kHz with zero audio-thread allocations, bounded discontinuities on a DC
input, identical current-cut output and changed subsequent output after reseeding.

Validation: full local Release build and VST3 validator passed (47 checks),
all 31 CTest suites passed, and Windows/Mojave syntax/API checks passed.
Packaged native GUI and Cubase behavior still require the native builds.
