# Ableton Live 11 editor-stutter follow-up

Checked on 19 October 2026 against `fix/purple-rnd-host-performance`,
revision `0e4c42a08e7b1b6b21ca829049d30c6d0c64d784`.
The reported symptom is window/control stutter in Live 11 on both Windows and
macOS. Exact Live patch versions, OS versions, installed binary hashes and Mac
architecture have not been supplied in this session.

## Verified native evidence

GitHub Actions run [39242726045](https://github.com/danielmelegari/GrainsDosageGTRv2/actions/runs/39242726045)
built revision `a2802b0a5291a155acf05f7a5afaf6b87cae612d` (the predecessor of
the checked revision). Job logs were inspected, not inferred from workflow YAML.

- Windows: all 47 Steinberg validator checks, 41 regression suites, and the
  native editor smoke test passed.
- Intel macOS: all 47 Steinberg validator checks and native editor smoke test
  passed; the regression-test job and signature/deployment/package checks passed.
- Apple Silicon: the downloaded macOS package passed all 47 validator checks
  in each of three native ARM64 runs.

These checks do not execute Ableton Live 11. They cannot establish that the
reported stutter has been resolved. The current revision adds trial-helper
signing and distributed-archive checks; the above run predates those changes.

## Current local results

The current revision built successfully in Linux Release/unlocked configuration.
All 47 Steinberg VST3 validator checks and all 41 CTest regression suites passed.
The host-buffer suite covers 32/64-bit buffers and automation-only flushes, plus
bit-identical output with/without monitor delivery at 44.1/48/96 kHz and
32/256/2048-frame blocks. These results do not validate the native editor patch.

## Local Windows editor change

The purple editor still allocated a supersampled bitmap/DC for every paint and
created fonts for each text measurement and draw. The follow-up retains one
render buffer per editor (recreated on resize and released on detach) and caches
fonts per size. DPI transforms, letterboxing and supersampling remain in place.
A direct-paint fallback handles buffer-allocation failure.

The native Windows smoke test now checks that repeated repainting and resizing
do not grow the GDI handle count after warming the font cache. This test and the
changed Windows source require native Windows build/execution. The Linux build
does not compile the Windows editor. No additional macOS change was made:
the branch already caches static artwork/text and suppresses hidden-view ticks.

## Remaining verification

Build the changed source with `build-unlocked.yml` on Windows and macOS and run
its native editor tests. Then test the resulting exact bundles in Live 11:
open/close the editor repeatedly, drag controls during playback, resize/scroll,
exercise automation, and save/reopen a session. Compare editor-open and
editor-closed behavior with the same preset, audio buffer and sample rate.
Record exact Live/OS versions, Mac architecture and artifact revision. No
updated native binary was produced or installed in this session.
