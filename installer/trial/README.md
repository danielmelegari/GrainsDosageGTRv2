# Trial installers

`GrainsDosage — Trial Installers Windows and macOS` packages existing validated
Trial artifacts. It does not compile plugins or run Licensed/Unlocked builds.
It starts when these installer files change on `work/mockup-purple-skin`, or by
manual dispatch. Optional source-run IDs select specific downloads; otherwise
it finds recent successful Trial jobs on the selected branch. Missing, expired
or unvalidated sources fail the job instead of substituting another edition.
The run summary records the source commits (Windows and macOS may differ).

Downloads after all installation checks pass:

- `GrainsDosage-Trial-Windows10-11-Installer`: x64 Setup.exe for Windows 10/11.
- `GrainsDosage-Trial-macOS-Universal-Installer`: PKG for Intel macOS 10.14+ and
  Apple Silicon (M1–M4; the Mac must run an OS supported by its hardware).

Windows installs the full VST3 bundle under
`C:\Program Files\Common Files\VST3\GrainsDosage.vst3`.
macOS installs `GrainsDosage.vst3` under `/Library/Audio/Plug-Ins/VST3/` and
`GrainsDosage.component` under `/Library/Audio/Plug-Ins/Components/`.
Close the DAW before installation, then reopen it/rescan plugins.
Administrator approval is required. These replace the plugin at the standard
location without editing personal presets, projects, Keychain or trial records.
Installing again does not restart the trial.

The workflow actually installs and verifies files on Windows, Intel macOS and
Apple Silicon, then validates the installed AU. Mojave compatibility is checked
from the universal binaries' deployment targets; CI does not run macOS 10.14 or
exercise every Mac model. Installers are currently unsigned; Apple Developer ID
Installer signing/notarization and Windows Authenticode require publisher
certificates. Plugin bundles retain their existing ad-hoc signatures.

## Installer artwork

The supplied Grains Dosage Trial artwork fills the Windows welcome panel and
appears in the macOS welcome pane. Navigation buttons and installer instructions
remain native and readable. The Windows finish page retains its normal completion
and restart messages.

- `wizard-large.bmp`: 994 × 628 Windows welcome artwork (2× native panel size).
- `resources/welcome.png`: 800 × 506 macOS welcome artwork, displayed at 400 × 253.
- `resources/welcome.html`: macOS welcome text and image placement.

No publisher key or registration app is included.
