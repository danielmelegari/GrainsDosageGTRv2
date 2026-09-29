# GrainsDosage 0.13.0

## Glitch Random Trigger (0.12.1)
The Glitch Buffer step grid is replaced by a tempo-synced random trigger.
TRIGGER EVERY offers 1/4, 1/2, 1/1 and 2/1: 1, 2, 4 and 8 quarter-note beats.
CHANCE is the probability per interval (100% = every boundary; 0% = none).
SLICE controls cut length. Each successful trigger plays a two-cut burst,
capped at half the trigger interval to leave a gap. MOVE, VARIATION, REVERSE
and MIX still shape the glitch. The module Random button also randomizes
the interval. Playback waits for a grid boundary when enabled mid-interval.
The previous Glitch grid/refresh parameter IDs are retained but hidden and
ignored by the new Glitch scheduling. Old presets load with interval 1/4;
their Glitch sequencing now follows this new behavior. The new interval is
saved in presets and VST state and can be automated.


GrainsDosage is a VST3 effect for macOS (Intel Mojave target and Apple Silicon) and Windows 10/11 x64. This source archive contains the plugin code, 30 factory preset files, extracted GUI sprites, and GitHub Actions workflows; it does not contain prebuilt installers.

## 0.13.0

See AGGIORNAMENTO-IT.md for the dedicated filter sequencer, 64 original patterns, tuned minor Comb, 101 modulation destinations and Reslice automatic/one-shot random. Native platform builds still require GitHub Actions.

## What's new

- **Reslice:** records the audio after the three rearrangeable modules, then replays selected regions on a 16-step sequencer. Loop lengths: 4/1, 2/1, 1/1, and 1/2. Step controls select which captured region is played.
- **Gater Latch:** empty steps preserve the last gate state; Release steps close it. Normal mode remains the default.
- **Gater update:** Dry Step is removed. Each Wet step has a Sustain tail control; Minimum Length clamps fixed and randomized steps; Tie joins adjacent Wet steps into one continuous note, including across the loop. Legacy Dry values load as Wet and old parameter IDs remain unchanged.
- **Filter bank:** 19 original filter models and combinations, including ladder, transistor, notch, resonant peak, comb, formant, phaser, and split-band families. The ladder and multimode sounds are original approximations inspired by classic synth filter behavior, not exact Nord, Moog, or Serum reproductions.
- **Reverb:** existing sequenced reverb remains available alongside Plate, Cosmic Space, Dark Space, and Bloom Space algorithms. The latter are original diffused, modulated reverbs rather than copies of Eventide Blackhole.
- **30 presets:** ten original factory patches, nine imported user patches from the upgrade discussion, and eleven patches demonstrating the new modules. Imported parameter values retain their original IDs; new modules default off in those nine patches.
- **GUI:** one resizable window with brighter text and extracted skin sprites for the knobs, steps, and headers. The full layout fits at the default scale without scrolling.

## Presets

The plugin installs any missing factory presets when its preset menu opens and does not overwrite files already present. The 30 `.gdspreset` files are also included in `Presets/` in this archive. On first use, the nine imported user patches are available as presets 11–19.

- macOS: `~/Library/Audio/Presets/GrainsDosage/`
- Windows: `%APPDATA%\GrainsDosage\Presets\`

Save custom patches from the plugin's preset menu. Presets store parameters only, not recorded audio.

## Build workflows

Upload the extracted archive contents to the root of a GitHub repository, or place the project folder `GrainsDosage/` in the repository. In Actions, run the matching workflow:

| Workflow | Output |
|---|---|
| `build-macos.yml` | Intel x86_64 VST3 targeting macOS 10.14 (Mojave) |
| `build-silicon.yml` | Apple Silicon arm64 VST3 |
| `build-windows.yml` | Windows 10/11 x64 VST3 and installer |

Each workflow builds against its pinned Steinberg VST3 SDK, runs the regression tests and platform GUI smoke test, and publishes installable artifacts. The Mac builds and Windows build must run on their respective GitHub-hosted runners; a Linux test run is not a substitute for installing in Cubase on each target system.

## Audio pipeline and compatibility

Reslice is after the three existing modules and before the Gater and master effects. Existing filter, reverb, and parameter IDs are kept stable. Legacy presets and saved states default the newly added Reslice and Latch behavior to off. The added filter/reverb models are separate choices, so the prior models remain accessible.

Run the eleven standalone suites with `ctest --test-dir build -C Release --output-on-failure` after configuring with `-DGRAINS_BUILD_TESTS=ON`. The build workflow also runs Steinberg's VST3 validator and the native platform GUI smoke test.
