# Cross-DAW compatibility audit — open release blockers

This document records source-review findings, not completed DAW certifications.

## Completed on fix/host-compatibility-live11
- Processor guards missing output buffers and null inputs; do not assume every
  host exposes identical buffer arrangements during startup.
- XY parameter metadata lookup has a reserved-ID fallback.
- Host telemetry frequency reduced from 30 to 10 Hz.
- `GRAINS_DISABLE_HOST_TELEMETRY` diagnostic build flag isolates VST3 output
  parameter monitor overhead without disabling the audio DSP.
- macOS editor timer polling reduced from 30 to 15 Hz.
- Trial build candidates for Windows x64, macOS Intel and macOS ARM64.

## Critical outstanding: real-time buffer resizing
`src/engine.h::Engine::set(Settings)` calls `setBufferSeconds()` when its
requested capacity differs. `setBufferSeconds()` constructs two fresh
`std::vector<float>` buffers, copies stored samples, swaps vectors, and
reanchors playback states. This runs from `Processor::process()`, including
when an automatable buffer parameter changes. It can cause audio-thread stalls,
allocation jitter, and apparent DAW hangs. A correct fix must preallocate ring
memory and safely maintain logical capacity and frozen/read/write positions,
then test transitions at all five buffer lengths and sample rates. Simply
removing the resizing call would break the feature.

## Additional tests required
- Run VST3 validator in CI and review failures.
- Stress editor attach/detach; repeated creation and destruction in host.
- Measure DSP latency and CPU with GUI closed and open, telemetry on and off.
- Realtime allocation tracing, rapid parameter automation, block sizes 32..2048,
  sample rates 44100/48000/96000, mono/stereo and null-buffer host scenarios.
- Live 10.1+, 11, 12; Bitwig; Cubase; REAPER; FL Studio; Studio One; Logic AU
  on supported operating systems. Host installation and executable licenses
  required. No GitHub build alone proves compatibility with every host.
- Windows and macOS trial fresh-machine request/issue/install/expiry paths,
  invalid signature, clock rollback, and session restore.

## Release rule
Do not claim all-DAW compatibility until the above blockers and reproducible
host tests pass. Preserve main and do not push untested compatibility changes
to a commercial release.
