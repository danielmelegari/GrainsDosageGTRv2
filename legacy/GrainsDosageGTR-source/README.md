# GrainsDosage 0.10.1

Stereo VST3 granular/glitch/repeat processor. C++17 + Steinberg SDK, no JUCE.
The archive contains sources and ten .gdspreset files, not compiled VST3 binaries.

See AGGIORNAMENTO-IT.md for the new 16-step Wet/Dry/Off Gater, per-step length,
Length RND and Step RND. Default editor size is 792×660 (60%); no scrolling.

## Presets and displays

The PRESETS menu installs ten factory files if missing and lists the preset folder.
Existing files are never overwritten during installation. Save Preset defaults to
that folder; Load Preset also opens files elsewhere. Open Preset Folder reveals:

- macOS: ~/Library/Audio/Presets/GrainsDosage/
- Windows: %APPDATA%\GrainsDosage\Presets\

The same ten files are included in Presets/. They are editable starting points;
results depend on the source audio. No audio samples or buffer history are saved.
Presets from 0.8.1 remain readable. Invalid files are rejected before editing values.

Compact pan controls free space for the grain screen. A 128-bin stereo peak
waveform comes from the actual buffer entering the Grain stage, after any earlier
modules in the chosen order. Gray = context; green = source span of the most
recently started active grain; light line = its current read position. Overlapping
grains are still audible; the screen deliberately follows one representative grain.
The view covers at least 2.5 seconds and expands for longer source spans. It is a
binned overview with visual amplitude auto-scaling, not a sample-level oscilloscope. Data is published about 30 Hz.

## Modulation and routing

Each of four LFOs has six destination menus with bipolar Amount. Destinations:
Size, Density, Pitch, Lookback, Chaos, Grain Mix, Speed, Transpose, Filter Cutoff,
Filter Resonance, Filter Drive. Duplicate assignments sum before clamping.
The 128 waveforms plus S&H/S&G retain up to 64 points. Speed multipliers 0.25x,
0.5x, 1x and 2x scale either sync or free rate without reducing the point count.
The moving cursor uses engine phase; random-wave previews use the current cycle
and retrigger seed. The host must forward read-only output parameters for live
meters, waveform and LFO cursor updates.

Input De-click precedes all wet and dry routing. ON/OFF and Sensitivity are below
the preset menu. Short impulses only; compare against bypass on percussive sources.
Fixed 32-sample lookahead is reported to the host, including when disabled/bypassed.
New instances default ON, old presets/states default OFF.

Signal path: Input De-click → reordered Grain / Glitch / Repeat → Speed / Transpose → Gater → Filter with
Drive → Reverb → Normalize → Dry/Wet → Limiter → output. This applies to all
orders, including legacy parallel mode. Filter always precedes Reverb: changing
it does not retroactively filter an existing reverb tail. The master dry path
bypasses these wet effects. Limiter stays last and can be switched off.

Reverb has five damping characters, a 16-step send sequencer (1/4–1/32) and Length
0.2–20 s (nominal decay, not a hard cutoff). Unlit steps stop new sends, not tails.
Transport jumps do not erase the tail. KILL DRY solos the reverb return, overriding
the master dry mix while Reverb is ON. Amount still controls return level. With
no lit sends and no remaining tail, Kill Dry is silent. Reverb OFF restores dry;
global Bypass bypasses everything. Switching Kill is smoothed over a few ms.

Normalize matches wet/dry stereo RMS gradually within ±12 dB; it is not LUFS
matching. Silent input freezes gain. The final sample-peak limiter has selectable
0 / −6 / −10 dBFS ceilings and a final float-rounding guard, instantaneous attack, 80 ms release and no lookahead. Not true peak.

## Builds

| Workflow | Artifact | Target |
|---|---|---|
| build-macos.yml | GrainsDosage-Mojave-Intel | x86_64, macOS 10.14+ |
| build-silicon.yml | GrainsDosage-Silicon-M1 | arm64, macOS 11+ |
| build-windows.yml | GrainsDosage-Windows10-x64 | Windows 10 x64 |

Replace the complete src/, CMakeLists.txt, test_*.cpp, assets/ and Presets/.
Existing three workflows do not need editing. SDK pinned to
3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96. Presets are also embedded in the code,
so Actions-built plugins install them without needing a separate resource copy.
Native GUI tests and screenshots run on their destination runners. Local testing
is Linux only; actual Mojave/Cubase, M1 and Windows 10 listening tests remain needed.

Plugin identities and existing automation IDs remain stable. State 0x5147313B
reads previous formats. Old LFO assignments migrate to six slots where possible;
if a legacy LFO used more than six destinations, excess old routes stay active via
hidden compatibility parameters. Loading Clean Grains clears these legacy routes.
Old host automation on those IDs remains honored. Gate and Feedback IDs stay
hidden and disabled. New parameters are appended. See validation-linux.txt.
