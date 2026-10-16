# GrainsDosage 0.13.15 — charcoal three-page edition

This branch builds the **Modules / Modulation / Morph** interface. The audio rack contains Granulizer, Beat Repeater, generative Reslice (CutDSG, WarpDSG and PushDSG), and Gater, with per-module capture loops. The Modulation page adds four six-slot LFOs, a 19-model filter, a dedicated filter sequencer, reverb and master controls.

Read the [complete user manual](USER-MANUAL.md) for installation in Cubase, every control, modulation, Morph, presets, offline registration and troubleshooting. A printable, offline HTML copy is included in the Windows installer.

## Current fixes

- Output Quality offers Draft / Normal / High CPU budgets.
- Sparse modulation routing and cached DSP work reduce processing cost; grain-overlap gain smoothing addresses a reproduced bass-click case.
- Charcoal panels, orange controls and cyan readouts match the supplied reference across all three pages.

- Windows graphics and text share one scale; host attachment and VST3 DPI changes update editor dimensions.
- Full current Reslice sound/capture controls are available as LFO and Morph destinations.
- The registered Windows trial has a signed 14-calendar-day period, no one-hour session limit, and recovery from temporary protected-storage failures.
- Audio CPU improvements cache unchanged calculations and protect x86 processing from denormal slowdowns. See [CPU-PERFORMANCE.md](CPU-PERFORMANCE.md).

## Windows installer

Run [Windows 14-day trial](../../actions/workflows/windows-trial.yml) on `work/mockup-purple-skin`.

| Artifact | Contents |
|---|---|
| `GrainsDosage-Windows-x64-ThreePage-14-Day-Trial` | Windows 10/11 x64 Setup executable |
| `GrainsDosage-Windows-ThreePage-Previews` | Native Windows screenshots of all three pages |
| `Windows-audio-CPU-comparison` | Same-runner before/after engine benchmark and audio checks |

The workflow builds and tests the licensing code, verifies the native interface, builds the actual trial with the publisher's public key, and packages it using Inno Setup. It starts automatically on relevant pushes to this branch and can also be dispatched manually.

The [multi-platform workflow](../../actions/workflows/build-all.yml) builds the other platform/edition packages. The registered Windows trial installer comes from the dedicated Windows trial workflow.

## Offline licensing

The plugin requires a signed license for the registered Windows machine. All processing is available during the signed period; outside it, the effect fades to latency-aligned dry without interrupting the host. The registration tool shows the installed license's start/end dates. No server connection is needed.

See [OFFLINE-TRIAL.md](OFFLINE-TRIAL.md) for publisher signing and machine registration. Keep the private key outside the repository. Installing the full edition replaces the trial at the same VST3 location.

## Presets and compatibility

The factory collection contains three 128-preset banks: Legacy, Circuit Fractures and Forest Escape. Windows user presets live under `%APPDATA%\GrainsDosage\Presets\`; macOS uses `~/Library/Audio/Presets/GrainsDosage/`.

Older plugin states and preset files migrate their selected destinations to the expanded lists. New presets use format 2. Check old DAW automation that changes destination selectors themselves after upgrading, because the expanded lists use new normalized positions. See the manual's automation section.

## Building and checking

Use CMake with the pinned VST3 SDK and `-DGRAINS_BUILD_TESTS=ON`. Run `ctest --test-dir BUILD_DIR -C Release --output-on-failure`. Windows trial configuration additionally needs `-DGRAINS_TRIAL=ON` and a public verification header; the workflow generates it with `tools/configure_trial_key.py`.

Local DSP tests complement the native platform checks. A native CI smoke test is not a substitute for testing the final installer in Cubase with the user's display, audio driver and project.
