#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
export PATH="/opt/homebrew/bin:/usr/local/bin:/Applications/CMake.app/Contents/bin:$PATH"
if ! xcode-select -p >/dev/null 2>&1; then
  echo 'Install Apple Command Line Tools first: xcode-select --install'
  exit 1
fi
if ! command -v cmake >/dev/null 2>&1; then
  echo 'Install CMake 3.25 or newer from https://cmake.org/download/ then run this file again.'
  exit 1
fi
sdk_path="$PWD/dependencies/vst3sdk"
if [ ! -d "$sdk_path/.git" ]; then
  git clone https://github.com/steinbergmedia/vst3sdk.git "$sdk_path"
fi
git -C "$sdk_path" checkout 3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96
git -C "$sdk_path" submodule update --init --recursive
grains_arch="$(uname -m)"
grains_min_os=10.14
if [ "$grains_arch" = arm64 ]; then grains_min_os=11.0; fi
cmake -S . -B build-mac -DCMAKE_OSX_ARCHITECTURES="$grains_arch" -DCMAKE_OSX_DEPLOYMENT_TARGET="$grains_min_os" -DSMTG_BUILD_UNIVERSAL_BINARY=OFF -DVST3_SDK_ROOT="$sdk_path" -DCMAKE_BUILD_TYPE=Release -DSMTG_CREATE_PLUGIN_LINK=OFF
cmake --build build-mac --config Release --parallel 4
plugin_path="$PWD/build-mac/VST3/Release/GrainsDosage.vst3"
if [ ! -d "$plugin_path" ]; then
  echo 'Build completed. Locate GrainsDosage.vst3 in build-mac and copy it to your VST3 folder.'
  exit 1
fi
mkdir -p "$HOME/Library/Audio/Plug-Ins/VST3"
ditto "$plugin_path" "$HOME/Library/Audio/Plug-Ins/VST3/GrainsDosage.vst3"
echo 'Installed GrainsDosage. Restart Cubase and insert it on a stereo audio track.'
