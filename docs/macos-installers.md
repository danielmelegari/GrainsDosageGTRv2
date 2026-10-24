# macOS Trial and Licensed installers

Run **GrainsDosage — macOS Trial and Licensed installers (Mojave+)** in GitHub
Actions on the desired source branch. It builds both commercial editions from
that exact commit and packages them only after Intel and Apple Silicon bundle
validation succeeds. It does not select older artifacts from another run.

Before the new workflow is registered on the default branch, run the existing
**Build commercial installers** workflow on the source branch with
`macos_installers=true`; it calls this same workflow.

Artifacts:
- `GrainsDosage-trial-macOS-Installer`: 14-day automatic trial `.pkg`.
- `GrainsDosage-licensed-macOS-Installer`: full edition `.pkg`, requiring device activation through ACTIVATE.

Both install AU and VST3 in `/Library/Audio/Plug-Ins/Components` and
`/Library/Audio/Plug-Ins/VST3`. Close the DAW before installing. Install one edition
at a time: these editions replace each other and retain the same plugin identity.
Presets and saved sessions are not removed. Remove any manually installed duplicate
from your user plugin folder to avoid the host loading that copy instead.

Intel binaries target macOS Mojave 10.14 or later; Apple Silicon targets macOS 11
or later. CI checks the actual Mach-O deployment targets of the wrapper, embedded
VST3 and trial helper, rejects unguarded newer API use at compilation, installs
both packages and runs AU/VST3 validation on Intel and Apple Silicon macOS 15.
GitHub-hosted CI does not run Mojave: this is not a claim of runtime testing on
10.14 or every later macOS release. Check the final Apple Silicon jobs before
distributing artifacts; uploaded packages may be visible before those jobs finish.

The plugins currently use ad-hoc code signatures. The installer is not Developer
ID signed or Apple-notarized, so a downloaded package may require explicit approval
in macOS Security settings. For a public release without that warning, Developer
ID Application/Installer certificates and Apple notarization credentials must be
configured separately. Never disable Gatekeeper system-wide.

Each artifact includes the manual and SHA-256 checksum of its package. Package
versions come from the built AU/VST3 metadata, not a hardcoded installer version.
No private license signing key is included. See `docs/licensing.md` for activation.
