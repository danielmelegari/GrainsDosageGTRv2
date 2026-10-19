# Purple-skin RND and VST3 beta investigation

Base: `work/mockup-purple-skin` at `531c6ca`.

## RND status

The fifth RND modulator is already integrated in `6d53afa`, with randomization,
Gater trigger and display fixes in `8240023`. Preserve these commits and the
0x51473151 state format. See [RND behavior](rnd-modulator.md).

## Current changes

- Waveform/sequencer/modulation monitor publication: 30 Hz to 10 Hz. This lowers
  publication frequency by about 67%, not total plugin CPU by 67%. Output meters are capped at 30 Hz, accumulating RMS energy and peak
  values across the entire display interval. Large host blocks quantize the
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
BeatRepeater captures now keep a logical snapshot of the recording ring. Before
an incoming sample overwrites held audio, at most one stereo sample is preserved
in preallocated capture storage. Starting a capture no longer copies up to
16 seconds of stereo audio in one audio callback. No extra buffer is allocated.
A frozen copy of the previous algorithm is the sample-for-sample test oracle for
physical wraps, offsets, recaptures, pitch, reverse, length changes and reset.

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

## Follow-up hardening

The waveform countdown now saturates at zero when no host monitor sink exists;
it cannot run into signed integer underflow during long headless sessions.
Meter cadence and accumulated RMS/peak values are tested at all three sample
rates and host buffer sizes alongside bit-identical audio.

The macOS automatic-trial helper path is resolved on the initializing thread,
before launching its worker. The worker no longer calls `dladdr`, removing a
possible dynamic-loader-lock cycle when unload joins the worker. This does not
claim to identify the previous AU timeout's cause; the earlier candidate's
Apple Silicon AU validation succeeded before this additional hardening.

A local 96 kHz synthetic 16-second capture benchmark (200 recaptures) measured
455 microseconds mean / 736 microseconds maximum before, versus 0.073 / 0.390
after. These are capture-call timings on this Linux runner, not total DAW CPU
savings. The new capture algorithm produced bit-identical samples to the old
implementation at 8/44.1/96 kHz through the dedicated 65-second scenarios.

Follow-up Linux validation: all 40 CTest suites passed and all 47 VST3 validator
checks passed. Fresh native builds are required for this follow-up; the successful
run 39177410084 and its packages contain only the preceding candidate c271876.

The final steady-state engine comparison against c271876 used 48 kHz stereo,
256-frame blocks and medians of three 10-second runs. All eight audio hashes
were identical. Seven scenarios used 0.5–15.4% less engine CPU; one used 2.9%
more. This is an isolated Linux engine benchmark, not a DAW CPU guarantee.
The snapshot reader shares stereo interpolation, caches unchanged pitch rates
and keeps the per-sample path inline to avoid the sustained CPU regression
observed in the first snapshot prototype.
