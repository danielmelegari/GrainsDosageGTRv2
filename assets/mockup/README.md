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
to build Mac Intel and Windows VST3 bundles, exercise native interactions,
and produce a Mac screenshot. These builds are not installers. Installing on
a workstation requires copying the built `.vst3` bundle into its VST3 folder
and rescanning in the host. macOS builds use an ad-hoc signature.

Offline layout preview (illustrative audio and display values):

```sh
g++ -std=c++17 -O2 tools/mockup_preview.cpp -o /tmp/mockup-preview
/tmp/mockup-preview > /tmp/mockup-preview.svg
```

The native screenshot from the workflow is the authoritative rendering check.
