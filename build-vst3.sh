#!/bin/bash
# ============================================================
#  GrainsDosage - build the .vst3 bundle locally, NO installer.
#  Requirements: CMake >= 3.25, Xcode command line tools, git.
#  The Steinberg VST3 SDK is cloned into a temp dir; nothing is
#  installed on your system. Result:
#    build/VST3/Release/GrainsDosage.vst3
# ============================================================
set -euo pipefail
cd "$(dirname "$0")"

command -v cmake >/dev/null || { echo "[ERROR] cmake not found (brew install cmake)"; exit 1; }
command -v git   >/dev/null || { echo "[ERROR] git not found"; exit 1; }

SDK="${TMPDIR:-/tmp}/vst3sdk"
if [ ! -d "$SDK/.git" ]; then
  echo "[1/3] Downloading pinned Steinberg VST3 SDK to $SDK ..."
  git clone --no-checkout https://github.com/steinbergmedia/vst3sdk.git "$SDK"
fi
git -C "$SDK" checkout 3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96
git -C "$SDK" submodule update --init --recursive --depth 1

ARCH="$(uname -m)"   # arm64 or x86_64
DEPLOY=11.0
[ "$ARCH" = "x86_64" ] && DEPLOY=10.14

echo "[2/3] Configuring ($ARCH, macOS $DEPLOY+)..."
cmake -S . -B build \
  -DVST3_SDK_ROOT="$SDK" \
  -DCMAKE_OSX_ARCHITECTURES="$ARCH" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="$DEPLOY" \
  -DSMTG_CREATE_PLUGIN_LINK=OFF \
  -DSMTG_RUN_VST_VALIDATOR=OFF

echo "[3/3] Building Release..."
cmake --build build --config Release

# Ad-hoc sign so local DAWs load the bundle without a developer certificate.
codesign --force --sign - --timestamp=none "build/VST3/Release/GrainsDosage.vst3" 2>/dev/null || true

echo
echo "============ DONE - no installation needed ============"
echo "Plugin bundle: $(pwd)/build/VST3/Release/GrainsDosage.vst3"
echo "Copy that whole folder to: ~/Library/Audio/Plug-Ins/VST3/"
echo "then rescan plugins in your DAW. To remove later, delete it."
echo "========================================================"
