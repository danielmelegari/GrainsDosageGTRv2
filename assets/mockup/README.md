# Purple mockup skin

`knob.png` is the user's unmodified `KNOB.png` from `mockup grainsdosage.zip`.
The editor builds the panel art, text, controls and live visualizations with
native drawing primitives. The supplied JPG is a reference, not a flattened
background: values, switches, waveforms and knob pointers remain live.

The design canvas is 1632 × 1518, initially displayed at about 60%. The SIZE
menu offers 50–100% scaling. SKIN → Purple Mockup restores the new colors if
an older saved color theme is still selected.

Both native editors use `src/mockup_ui.h` for visual geometry and
`src/editor_layout_win.inl` for editable parameter rectangles. DSP and preset
parameter IDs are unchanged. Additional existing controls (pan, modulation
routes 5/6, retrigger, reverb kill dry, audio order and sequencer enable) stay
available alongside the mockup's controls. The filter sequencer retains all
32 steps. The reverb LENGTH parameter retains its real name and behavior;
it is not relabeled as feedback because the engine has no equivalent binding.

Run `.github/workflows/mockup-skin.yml` through its branch push / PR trigger
to build universal Mac (Intel + Apple Silicon) and Windows VST3 bundles, exercise native interactions,
and produce a Mac screenshot. These builds are not installers. Installing on
a workstation requires copying the built `.vst3` bundle into its VST3 folder
and rescanning in the host. macOS builds use an ad-hoc signature.

Offline layout preview (illustrative audio and display values):

```sh
g++ -std=c++17 -O2 tools/mockup_preview.cpp -o /tmp/mockup-preview
/tmp/mockup-preview > /tmp/mockup-preview.svg
```

The native screenshot from the workflow is the authoritative rendering check.

The five module buttons directly under the preset header set the audio order. Drag a button left or right to insert its module in the chain; neighbouring buttons shift with it. A click selects that module, and its LED controls that module’s enabled state. Dropping outside cancels. The waveform uses a fixed trailing window and fixed gain, with softened updates and a subdued highlight of the dominant active grain. Each modulation scope has a live phase cursor.

Random Impulse hides the Reverb step controls; switching back to Step Sequencer restores them. The Granulizer waveform spans the widened module pane.

Module routing positions now ease between slots at a 60 Hz editor refresh. Gater bars rise with the current note phase and show its configured length. BeatRepeater steps have dedicated vertical space; Reslice uses controls, steps, and actions on three rows. Slider handles use a wider light thumb with an accent outline.

Reverb RATE adds 1/4D (1.5 beats) and 1/8D (0.75 beats) for both sources. The new appended rate parameter defaults to Preset rate, preserving the original source-specific rate and existing automation. State version 47 reads previous versions, including version 46.

## Rack update and routing bank

The rack skin follows the supplied October 8 image: dark purple bolted panels, metal side rails, brushed knobs, recessed scopes, vertical reverb faders and two live stereo RMS VU displays. The complete prior control set remains available, including 32 filter steps and Random Impulse hiding its sequencer. The canvas is now 1744 × 1860; its default native size is 872 × 930.

The preset menu contains eight categories with 16 new presets each. Every preset stores an explicit five-stage routing; the bank covers all 120 permutations. Existing old factory files appear under Legacy Presets and other files under User Presets. Installation does not overwrite files. Previous/next follows category order. The build also exports a separate bank ZIP with a routing catalog.

Six lock switches protect Granulizer, Preslicer, BeatRepeater, Reslice, Reverb Amount and Output Mix during randomisation. A closed lock preserves its current value; direct edits and automation still work. Gater has no Mix parameter. Locks are saved in presets and project state (version 48, with backward-compatible reads).
