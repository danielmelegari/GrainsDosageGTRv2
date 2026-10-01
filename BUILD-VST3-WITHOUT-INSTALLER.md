# GrainsDosage — Get the .vst3 WITHOUT running any installer

You never need Setup.exe or a .pkg. The plugin is a self-contained bundle:
copy it into the VST3 folder and your DAW finds it on next scan.

## Option A (recommended): GitHub cloud build, nothing installed on your PC

1. Push your code to GitHub (main branch), or open the Actions tab of
   `danielmelegari/GrainsDosageGTRv2`.
2. Go to **Actions** → pick the workflow for your machine:
   - **Build VST3 only (no installer)** (`build-vst3.yml`) — unico workflow, genera 3 artefatti:
     macOS Intel x64, macOS Apple Silicon arm64 e Windows 10/11 x64 (solo zip .vst3, niente .pkg/.exe)
3. Click **Run workflow** → **Run workflow** (green button). Wait ~5–10 min.
4. Open the finished run and under **Artifacts** download the zip for your platform:
   - Windows: `GrainsDosage-Windows10-x64-VST3.zip` → contains the plain **`GrainsDosage.vst3` bundle** (no installer).
   - macOS Intel: `GrainsDosage-macOS-x64-VST3.zip`.
   - Apple Silicon: `GrainsDosage-macOS-arm64-VST3.zip`.
5. Unzip and **copy the whole `GrainsDosage.vst3` folder** (not just the file
   inside it) to the DAW scan path shown below. Done — no installer involved.

Tip: you can also link straight to a release-style download without opening
the site: https://github.com/danielmelegari/GrainsDosageGTRv2/actions

## Option B: local build (only if you want to compile yourself)

Needs CMake ≥ 3.25 and a compiler (Visual Studio 2022 / Xcode) — these are
build tools, not plugin installers. The Steinberg VST3 SDK is downloaded by
the script automatically (git clone only).

Windows (from a "Developer Command Prompt" or PowerShell):
```powershell
git clone --no-checkout https://github.com/steinbergmedia/vst3sdk %TEMP%\vst3sdk
git -C %TEMP%\vst3sdk checkout 3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96
git -C %TEMP%\vst3sdk submodule update --init --recursive --depth 1
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
      -DVST3_SDK_ROOT=%TEMP%\vst3sdk -DSMTG_CREATE_PLUGIN_LINK=OFF
cmake --build build --config Release
:: result: build\VST3\Release\GrainsDosage.vst3
```

macOS / Linux:
```bash
git clone --no-checkout https://github.com/steinbergmedia/vst3sdk /tmp/vst3sdk
git -C /tmp/vst3sdk checkout 3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96
git -C /tmp/vst3sdk submodule update --init --recursive --depth 1
cmake -S . -B build -D CMAKE_OSX_DEPLOYMENT_TARGET=10.14 \
      -D VST3_SDK_ROOT=/tmp/vst3sdk -D SMTG_CREATE_PLUGIN_LINK=OFF
cmake --build build --config Release
# result: build/VST3/Release/GrainsDosage.vst3
```

## Where to copy the bundle (manual "installation")

| OS       | Copy `GrainsDosage.vst3` (whole folder) to            |
|----------|--------------------------------------------------------|
| Windows  | `C:\Program Files\Common Files\VST3\`                  |
| macOS    | `~/Library/Audio/Plug-Ins/VST3/`                       |
| Linux    | `~/.vst3/` or `/usr/local/lib/vst3/`                   |

Then close and reopen your DAW (or rescan the VST3 folder in its plug-in
manager). Keep only one version of the plugin in the scan path.

To remove it later: delete the `GrainsDosage.vst3` folder. Nothing else was
written anywhere — that's the advantage of using the bundle directly instead
of the installer.
