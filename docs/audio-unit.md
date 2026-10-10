# macOS Audio Unit workflow

`.github/workflows/build-au.yml` builds `GrainsDosage.component` (AUv2) for Intel
and Apple Silicon on macOS 11 or later. It runs on source changes to main and the
skin work branch, relevant pull requests, and manual workflow dispatch once the
workflow is present on the default branch.

The component embeds a real copy of the same VST3 engine and native editor; it
needs no separately installed VST3. Both SDK revisions are pinned. CI verifies
both architectures, signs the inner VST3 and outer AU ad hoc, installs the AU on
the disposable runner and requires Apple's `auval -v aufx Gdsg Dmeg` to pass.
The exact ZIP is then extracted on two clean runners and must be discovered and
pass native `auval` on both Intel and Apple Silicon. The final
`GrainsDosage-macOS-AU` artifact is published only after both checks succeed.
Separate `GrainsDosage-AU-validation-*` artifacts contain each platform's logs.
These checks do not replace testing inside Ableton Live itself.

To install, close the host, unzip and copy `GrainsDosage.component` to
`~/Library/Audio/Plug-Ins/Components/`, then reopen Logic or another AU host.
The component is ad-hoc signed, not Developer ID signed or notarized.

## Ableton Live installation

1. Download **GrainsDosage-macOS-AU**, extract the artifact ZIP, then extract
   **GrainsDosage-macOS-Universal.component.zip** inside it. The file to install
   is **GrainsDosage.component**, not either ZIP or the inner plugin.vst3.
2. Quit Live. In Finder use Go > Go to Folder and enter
   `~/Library/Audio/Plug-Ins/Components/` (create Components if needed). Copy the
   component directly there, replacing the previous GrainsDosage.component.
   Avoid installing duplicate copies in both the user and system Components
   folders. AU components do not belong in Live's VST custom folder.
3. Reopen Live, open Settings/Preferences > Plug-Ins, and enable **Use Audio
   Units** (called **Use Audio Units v2** in some versions).
4. Hold **Option** while clicking **Rescan**. Look under Plug-Ins > Audio Units
   > **Daniel Melegari > GrainsDosage**. It is an audio effect, not an instrument.

If absent, restart the Mac once so the Audio Unit registry refreshes, then rescan.
If still absent, run `bash /path/to/diagnose-au.sh > ~/Desktop/GrainsDosage-AU-report.txt 2>&1`
in Terminal using the script included with the artifact. Send that report with
your macOS version, Live version, and whether Live runs natively or with Rosetta.
The diagnostic only reads installation details and runs Apple's validator; it
does not change security settings, clear other plug-ins' caches, or reinstall files.

Successful `auval` plus missing Live registration points to Live's scan/settings;
failed `auval` provides the loader error to investigate. Browser downloads may
be quarantined by macOS because this development build is not notarized. If
macOS explicitly blocks it, check Privacy & Security for its specific message;
do not disable Gatekeeper globally.
