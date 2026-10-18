# Build and download

The single `.github/workflows/build-all.yml` workflow runs on pushes to `main`
and `work/mockup-purple-skin`, or manual dispatch. It has exactly five jobs:

- `macos`: universal development VST3, regression/native GUI tests, three preset banks.
- `windows`: development VST3, regression/native GUI tests, three preset banks.
- `au`: universal development AU, shipped archive validation on Apple Silicon.
- `trial`: universal macOS AU/VST3 with 14-day trial and offline activation.
- `licensed`: universal macOS AU/VST3 requiring the signed offline serial.

The macOS job also installs and validates the AU job's exact archive on Intel,
so it depends on `au`. All other jobs start independently. The AU and VST3 binary
minimum remains Intel macOS 10.14 and Apple Silicon macOS 11. Trial/Licensed run
license lifecycle tests, deployment/signature checks and native AU validation.
The workflow retains preset exports and native GUI screenshot artifacts.

A newer run on the same branch cancels an older run of this unified workflow.
Legacy separate build workflow files have been replaced to prevent duplicate builds.
