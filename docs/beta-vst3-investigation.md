# Purple-skin RND and VST3 beta investigation

Base: `work/mockup-purple-skin` at `531c6ca`.

## RND status

The fifth RND modulator is already integrated in `6d53afa`, with randomization,
Gater trigger and display fixes in `8240023`. Preserve these commits and the
0x51473151 state format. See [RND behavior](rnd-modulator.md).

## Current changes

- Waveform/sequencer/modulation monitor publication: 30 Hz to 10 Hz. This lowers
  publication frequency by about 67%, not total plugin CPU by 67%. Per-block
  output meters retain their existing cadence. Large host blocks quantize the
  display interval. Audio algorithms and parameter IDs remain unchanged.
- `GRAINS_DISABLE_HOST_TELEMETRY=ON` disables monitor output in a diagnostic build
  without disabling DSP. Meter, waveform and sequencer displays will stop updating.
- macOS editor polling: 30 Hz to 15 Hz.
- Safely handle automation-only flushes and absent 32/64-bit host channel buffers;
  missing input channels are silence, missing output storage returns an error.
- Extend scoped denormal flushing to ARM64 (including Apple Silicon), restoring
  the host's FPCR afterwards, as the x86 path already restores MXCSR.

The older compatibility branch predates RND, newer state formats and existing
buffer optimizations. It must not replace the purple-skin source. Its buffer
resize finding does not describe this base: selectable windows already use
preallocated storage. Existing allocation tests cover live/frozen size changes.
Rhythmic captures still copy bounded audio slices; absence of allocation alone
does not establish freedom from worst-case audio-thread stalls.

## Ableton cannot find the VST3 / Finder says Application

The base already includes `eacde6e`: the VST3 plist declares `BNDL` instead of
`APPL`. Commercial CI checks both bundle types, universal Intel/ARM binaries and
deployment targets; trial packaging rejects application-classified bundles.
This source fix cannot repair a tester's older installed binary. Finder's label
alone does not prove the reason for an Ableton scan failure.

Run `bash tools/diagnose-vst3-macos.sh` on the affected Mac. The report is read-only
and records both standard VST3 locations, version, architecture, binary SHA-256,
bundle type, signature validation, quarantine attribute and Ableton log locations.
Send the report with Ableton's exact version and the scan-log lines mentioning
GrainsDosage or the load error. Review paths before sharing.

Use a rebuilt **macOS** VST3 bundle under `/Library/Audio/Plug-Ins/VST3/` or
`~/Library/Audio/Plug-Ins/VST3/`. Double-clicking a VST3 is not a load test. Enable
VST3 system folders in Live's Plug-Ins preferences and use Option-click Rescan
for a full scan after installing the corrected build. Check for duplicate old
copies in the two locations. Do not rename an application to `.vst3`, edit a
signed plist in place, or remove signatures to mask a scan problem.

## Host verification still needed

Compare the same preset/input, sample rate and audio buffer in Ableton, Cubase
and Bitwig, with the editor open and closed. Record average and peak CPU,
dropouts, build hash, OS and CPU architecture. Compare a diagnostic telemetry-off
build if needed. Reducing telemetry frequency does not reduce the worst-case
cost of an individual waveform snapshot. Linux regression tests and Steinberg
validation do not certify native macOS/Windows DAW behavior or commercial trial
startup. Do not publish this candidate as an all-host compatibility fix until
native builds and affected DAW sessions pass.

## Validation for this candidate

Linux Release: all 39 CTest suites passed, including RND, state/preset migration,
real-time buffer allocation checks and the new host-buffer/telemetry regression.
The Steinberg VST3 validator passed all 47 checks. Telemetry with/without a host
monitor sink produced bit-identical audio at 44.1/48/96 kHz and 32/256/2048-frame
blocks. These are development-edition Linux results; ARM64 FPCR and macOS editor
changes require native execution.

Repository history also shows installer run `39080630247` built both installers
successfully but failed **Apple Silicon Audio Unit validation**. That AU failure
is separate from the reported VST3 scan problem. The previous macOS Trial build
at `eacde6e` passed, but predates RND and this candidate. Do not assume a previous
successful artifact contains the latest source.
