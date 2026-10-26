# Full Unlocked VST3 — no activation

The **GrainsDosage — Full Unlocked VST3 (no activation)** workflow builds Release VST3 bundles from the same source with `GRAINS_EDITION=unlocked`:

- **Windows:** Windows 10/11 x64.
- **macOS:** universal Intel 10.14+ / Apple Silicon 11+.

These builds run with all features enabled on any computer. They have **no activation, Machine ID restriction, trial timer or expiry**. The header shows **LICENSE ACTIVE - UNLOCKED** and has no Activate action. They do not need a license TXT or the keygen. Whoever receives this build can run it on another computer too.

Download the `GrainsDosage-Windows-VST3-Full-Unlocked` or `GrainsDosage-macOS-VST3-Full-Unlocked` artifact and extract both ZIP layers. Copy the entire `GrainsDosage.vst3` bundle to:

- Windows: `C:\Program Files\Common Files\VST3\`
- macOS: `~/Library/Audio/Plug-Ins/VST3/` (or `/Library/Audio/Plug-Ins/VST3/` for all users).

Close the DAW before replacing the plugin, then rescan if needed. The plugin IDs, preset format and session identity are preserved, so this edition replaces the Trial/Licensed VST3 at the same location. Install one edition at a time. These downloads contain VST3 only; the existing AU/Trial/Licensed workflows remain available separately.

The manual also describes activation for the machine-bound editions; those steps do not apply to **Full Unlocked**. macOS bundles are ad-hoc signed, not notarized; Windows bundles are not Authenticode signed. Minimum targets are checked in CI, while actual Mojave DAW runtime remains a physical-machine check.

To run manually, open Actions, select this workflow, click **Run workflow**, and choose `fix/purple-rnd-host-performance` for the latest RND and compatibility candidate. This workflow runs manually. No public/private signing-key configuration is required. Separate artifact names distinguish these downloads from machine-bound Licensed builds.

The macOS package is also extracted and checked on an Apple Silicon runner.
The shipped VST3 runs through the Steinberg validator natively on ARM64 three
times; this complements Intel validation and bundle/signature checks. It does
not replace testing in the affected Ableton, Cubase or Bitwig versions.
