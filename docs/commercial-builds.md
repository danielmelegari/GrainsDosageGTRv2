# Commercial build and validation matrix

One commit and one source tree build these five deliverables in **GrainsDosage — Commercial builds and License Generator** (`.github/workflows/build-all.yml`). The issuer is a reusable workflow. The old Windows-trial and installer dispatch entries delegate to this same pipeline rather than maintaining separate compilation recipes.

| Artifact | Contents | Target |
| --- | --- | --- |
| `GrainsDosage-Windows-VST3-trial` | VST3 ZIP, registration utility, trial Setup, editor previews | Windows 10/11 x64 |
| `GrainsDosage-Windows-VST3-licensed` | VST3 ZIP, registration utility, Licensed Setup, editor previews | Windows 10/11 x64 |
| `GrainsDosage-trial-macOS` | AU component ZIP and VST3 ZIP | Intel 10.14+, arm64 11+ |
| `GrainsDosage-licensed-macOS` | AU component ZIP and VST3 ZIP | Intel 10.14+, arm64 11+ |
| `GrainsDosage-License-Generator-macOS-Mojave-Intel` | ZIP containing `GrainsDosage License Generator.app` | Intel 10.14+ |

Additional artifacts contain the complete user/publisher manuals, Windows CPU comparison and native Apple Silicon AU validation. Download the artifact, then extract its inner ZIP. Trial and Licensed use the existing plugin IDs and installation paths; install one edition at a time. A signed activation determines the trial interval or permanent entitlement. Development remains the default CMake edition and is unrestricted; development binaries are not commercial deliverables.

## License compatibility and audio

- Mac permanent `GDS2` payload: `GRAINS-DOSAGE-V2:<32-hex-license-id>:<64-hex-device>:3`; unchanged from the existing issuer.
- Mac signed trial `GDS2` payload: `GRAINS-DOSAGE-TRIAL-V2:<64-hex-device>:<Unix-start>:<Unix-end>`. There is no trial License ID. The interval must be exactly 1,209,600 seconds.
- Windows trial: existing five-line `GRAINS-TRIAL-1` document, 64-hex Machine ID, start/end FILETIME, hex signature. The interval is exactly 14 calendar days.
- Windows permanent: five-line `GRAINS-PERMANENT-1` document, 64-hex Machine ID, 32-hex License ID, policy `3`, hex signature.
- All signatures use the same existing RSA-2048 key, SHA-256 and PKCS#1 v1.5. `GDDEV2-` is the Mac display prefix; its 64-character hash is signed unchanged. Windows retains its own hardware/OS-derived 64-character hash. The issuer recognizes these two explicit formats; it does **not** convert one machine identity into another or strip arbitrary prefixes.
- Paste the complete code into **ACTIVATE** in either plugin. Windows accepts TXT line endings from macOS or Windows. Existing Mac permanent codes and Windows trial documents remain valid. New signed Mac trials and Windows permanent codes need the new plugin build.
- Authorization is checked outside `process()`. The audio thread reads an atomic decision and applies a 20 ms wet-to-dry transition. The dry signal retains the existing 32-sample lookahead latency; parameter automation, presets and DAW sessions remain intact. A valid trial/permanent grant enables the actual DSP, not a simulated output.

The prior Windows code returned early and cleared every audio output when its per-instance trial guard was blocked. The guard could also retain a failed initial authorization until the plugin was recreated, and older versions latched maintenance storage failures. The shared licensing manager now activates immediately, retries transient protected-storage failures, keeps the signed end fixed, and controls the same dry crossfade as macOS. On Mac, the previous verifier only recognized permanent GDS2 payloads; signed GDDEV2 trials now have an explicit validated payload and protected lifecycle. No one-hour expiry rule was found in the source. These changes and simulated two-hour tests address identified paths; a customer's specific hour-long Cubase failure still needs a real project reproduction to attribute conclusively.

## Automated checks

- Full DSP/state regression suite plus actual VST3 `process()` tests in 32/64-bit format: valid authorization produces processed audio, expiry fades to dry, reactivation restores processing, and dry latency remains unchanged.
- Windows CNG tests use in-memory test keys: correct/wrong machine, valid/expired/future/tampered signature, permanent policy, protected DPAPI state, concurrent instances, rollback, temporary storage access failure and two-hour refresh simulation.
- Native Mac tests consume codes from the actual issuer backend, check them independently with OpenSSL, and exercise valid, expired, future, altered, wrong-device and wrong-duration codes. Existing permanent GDS2 remains accepted. Tests use disposable RSA keys and isolated Keychain records, never the production private key.
- SQLite tests verify customer/license reuse, a combined three-device cap, concurrent requests, fixed trial reissuance dates, missing-ledger and mismatched-key rejection, and transaction rollback if signing fails.
- Package checks require the production public key in the binary and reject private-key files, private PEM blocks and SQLite ledgers. The generator bundle uses an explicit file allowlist. No workflow accepts the production private key; no signing secrets or key material are printed in logs. Test keys live only in temporary directories and are deleted before packaging.
- Mac load commands are checked for **x86_64 minimum 10.14 and arm64 minimum 11.0**, in both the AU wrapper and embedded VST3. Newer APIs are compiler errors. Intel and Apple Silicon runners perform AU validation on the built/shipped components. VST3 SDK validation and native editor smoke tests also run.

## What CI cannot establish about Mojave

GitHub's hosted macOS runners do not run Mojave. An SDK/deployment check is not a physical 10.14 runtime test. On a real **Intel Mac running macOS 10.14**, still verify:

1. Gatekeeper first opening of the downloaded ad-hoc-signed app and plugins; install location and AU/VST3 discovery in a Mojave-compatible DAW.
2. One-time import of the actual matching publisher PEM (including its real encryption/password format), Keychain authorization, app restart and subsequent signing without selecting the PEM again.
3. Reuse of the original production ledger and License IDs, correct 1/3–3/3 count and fourth-device rejection; backups and restoration of the configured path.
4. Trial and permanent activation of the actual downloaded AU and VST3, correct GDDEV2, displayed days, full audio, closing/reopening the GUI and DAW, sleep/wake and system-clock correction.
5. Extended playback in the user's Cubase project, including at least the previously reported one-hour interval, at the user's ASIO/CoreAudio buffer and sample rate; expiry-to-dry behavior without a click. Never change the production clock just to test expiry: use an isolated test system/test signing configuration.

Mac downloads are ad-hoc signed, not Apple Developer ID notarized. Windows downloads are not Authenticode-signed by this workflow. These packaging signatures are separate from the real RSA signatures used for activation.
