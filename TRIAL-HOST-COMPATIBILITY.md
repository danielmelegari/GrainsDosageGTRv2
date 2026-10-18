# Trial host compatibility (candidate)

This branch includes Windows x64, macOS Intel, and macOS Apple Silicon VST3 trial
targets, alongside the full-version compatibility changes. The trial retains the
same 14-day, publisher-signed machine-bound license file on both systems.

## Build candidates

GitHub Actions -> **Ableton-compatible trial VST3 (macOS and Windows)** -> Run workflow.
Set repository Actions variable `GRAINS_TRIAL_PUBLIC_KEY` using the existing
publisher public RSA key (see OFFLINE-TRIAL.md). Without it the workflow skips
trial builds rather than shipping a test-signed edition.

The macOS build uses Security.framework and IOKit, and the Windows build keeps
the existing Cryptography API / DPAPI implementation. Do not distribute a CI
artifact until all platform jobs, Steinberg validator and real DAW loading tests
pass. These are bare VST3 ZIPs, not notarized macOS installers or Windows Setup EXEs.

## macOS registration

On a Mac with Python 3, from the extracted source/support package:

```sh
python3 tools/mac_trial_registration.py request
```

Send the Desktop machine-request JSON to the publisher. The publisher issues a
license with the existing private signing key and `tools/issue_trial_license.py`.
Then install the issued license:

```sh
python3 tools/mac_trial_registration.py install /path/to/trial.license
```

Reopen the DAW; no network access is needed. The trial library stores its license
under `~/Library/Application Support/GrainsDosage-Trial/`. The machine ID uses
the macOS platform UUID rather than Windows MachineGuid, so a Windows license is
not transferable to a Mac. The file format/signing key are shared.

**Security limitation:** The macOS state file currently has a high-water time
marker with OS file permissions, not a Keychain-protected dual-store rollback
defense equivalent to Windows DPAPI. Offline trial protection cannot be
tamper-proof; do not claim equal anti-reset strength until a hardened macOS store
has been built and independently tested.

## Host-test matrix (manual, required)

- Ableton Live 10.1.x, Live 11.0.x, Live 11.1+, Live 12.x when OS-compatible;
  VST3 is not supported in older Live releases.
- Windows x64 and macOS Intel; Live 11.1+ for native Apple Silicon.
- Scan plugin; insert/remove repeatedly; open/close GUI repeatedly.
- Process 32-bit/64-bit buffers at 44.1/48/96kHz and common buffer sizes.
- Start/stop transport, automate parameters, change project tempo.
- Save/reopen sessions, test invalid/expired/mismatched licenses, and trial
  registration across fresh accounts.

Automated Steinberg tests do not launch Ableton or certify all DAWs. Continue to
ship the existing production release until those manual tests pass.
