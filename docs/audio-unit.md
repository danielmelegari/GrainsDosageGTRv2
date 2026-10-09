# macOS Audio Unit workflow

`.github/workflows/build-au.yml` builds `GrainsDosage.component` (AUv2) for Intel
and Apple Silicon on macOS 11 or later. It runs on source changes to main and the
skin work branch, relevant pull requests, and manual workflow dispatch once the
workflow is present on the default branch.

The component embeds a real copy of the same VST3 engine and native editor; it
needs no separately installed VST3. Both SDK revisions are pinned. CI verifies
both architectures, signs the inner VST3 and outer AU ad hoc, installs the AU on
the disposable runner and requires Apple's `auval -v aufx Gdsg Dmeg` to pass.
The artifact contains the component ZIP and validation log.

To install, close the host, unzip and copy `GrainsDosage.component` to
`~/Library/Audio/Plug-Ins/Components/`, then reopen Logic or another AU host.
The component is ad-hoc signed, not Developer ID signed or notarized.
