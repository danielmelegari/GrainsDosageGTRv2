#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/../.."
python3 tools/check-publisher-key.py
output_dir="${1:-build-license-generator}"
app="$output_dir/GrainsDosage License Generator.app"
mkdir -p "$app/Contents/MacOS" "$app/Contents/Resources"
clang++ -std=c++17 -O2 -fobjc-arc -arch x86_64 -mmacosx-version-min=10.14 \
  -Werror=unguarded-availability -Werror=unguarded-availability-new \
  tools/license-generator/main.mm -framework Cocoa -framework Security -lsqlite3 \
  -o "$app/Contents/MacOS/GrainsDosage License Generator"
cp tools/license-generator/Info.plist "$app/Contents/Info.plist"
cp docs/license-generator.md "$app/Contents/Resources/Guida.md"
plutil -lint "$app/Contents/Info.plist"
codesign --force --sign - --timestamp=none "$app"
codesign --verify --deep --strict "$app"
python3 tools/license-generator/check_bundle.py "$app"
python3 tools/check-public-only.py "$app"
ditto -c -k --sequesterRsrc --keepParent "$app" "$output_dir/GrainsDosage-License-Generator-Mojave-Intel.zip"
