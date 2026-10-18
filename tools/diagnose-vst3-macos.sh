#!/bin/bash
# Read-only report: no cache deletion, signature changes, or quarantine removal.
set -u
if [ "$(uname -s)" != Darwin ]; then
  echo 'Run this diagnostic on the affected Mac.' >&2
  exit 1
fi
sw_vers
uname -m
for bundle in "/Library/Audio/Plug-Ins/VST3/GrainsDosage.vst3" "$HOME/Library/Audio/Plug-Ins/VST3/GrainsDosage.vst3"; do
  [ -e "$bundle" ] || continue
  printf '\nBundle: %s\n' "$bundle"
  plist="$bundle/Contents/Info.plist"
  if [ ! -f "$plist" ]; then
    echo 'ERROR: missing Contents/Info.plist; this is not a complete macOS bundle.'
    continue
  fi
  for key in CFBundlePackageType CFBundleExecutable CFBundleIdentifier CFBundleShortVersionString LSMinimumSystemVersion; do
    printf '%s: ' "$key"
    /usr/libexec/PlistBuddy -c "Print :$key" "$plist" || true
  done
  executable=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$plist")
  binary="$bundle/Contents/MacOS/$executable"
  file "$binary"
  shasum -a 256 "$binary"
  codesign --verify --deep --strict --verbose=2 "$bundle" 2>&1 || true
  xattr -p com.apple.quarantine "$bundle" 2>/dev/null || true
  if [ -f "$bundle/Contents/PkgInfo" ]; then
    printf 'PkgInfo: '; cat "$bundle/Contents/PkgInfo"; printf '\n'
  fi
done
printf '\nAbleton scan-log locations (send relevant GrainsDosage/error lines):\n'
find "$HOME/Library/Preferences/Ableton" -type f \( -name 'Log.txt' -o -iname '*scan*log*' \) -print 2>/dev/null || true
