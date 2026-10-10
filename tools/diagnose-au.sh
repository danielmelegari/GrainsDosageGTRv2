#!/bin/bash
# Read-only installation diagnostics; does not clear caches or remove quarantine.
set -u
if [ "$(uname -s)" != Darwin ]; then
  echo "Run this diagnostic on the Mac where Ableton is installed."
  exit 1
fi
echo 'GrainsDosage AU installation report'
sw_vers
uname -m
for root in "$HOME/Library/Audio/Plug-Ins/Components" /Library/Audio/Plug-Ins/Components; do
  au="$root/GrainsDosage.component"
  echo "Checking: $au"
  if [ ! -d "$au" ]; then
    echo 'Not installed in this location.'
    continue
  fi
  /usr/libexec/PlistBuddy -c 'Print :AudioComponents' "$au/Contents/Info.plist"
  /usr/libexec/PlistBuddy -c 'Print :CFBundleVersion' "$au/Contents/Info.plist"
  file "$au/Contents/MacOS/GrainsDosage"
  file "$au/Contents/Resources/plugin.vst3/Contents/MacOS/GrainsDosage"
  codesign --verify --deep --strict --verbose=2 "$au" 2>&1
  echo 'Quarantine attribute (if present):'
  xattr -p com.apple.quarantine "$au" 2>/dev/null || true
done
echo 'Audio Unit registration (expected: aufx Gdsg Dmeg, Daniel Melegari: GrainsDosage):'
/usr/bin/auval -a 2>&1 | grep -E 'Gdsg|GrainsDosage' || true
echo 'Validation:'
/usr/bin/auval -v aufx Gdsg Dmeg
