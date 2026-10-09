#!/bin/bash
# Installs Dave Grain for the current user (no admin password needed).
# Double-click this file in Finder. If macOS blocks it, right-click > Open.

set -e
HERE="$(cd "$(dirname "$0")" && pwd)"

VST3_DIR="$HOME/Library/Audio/Plug-Ins/VST3"
AU_DIR="$HOME/Library/Audio/Plug-Ins/Components"
APP_DIR="$HOME/Applications"

mkdir -p "$VST3_DIR" "$AU_DIR" "$APP_DIR"

echo "Installing Dave Grain..."
rm -rf "$VST3_DIR/Dave Grain.vst3" "$AU_DIR/Dave Grain.component" "$APP_DIR/Dave Grain.app"
cp -R "$HERE/Dave Grain.vst3"      "$VST3_DIR/"
cp -R "$HERE/Dave Grain.component" "$AU_DIR/"
cp -R "$HERE/Dave Grain.app"       "$APP_DIR/"

# The build is not notarised by Apple, so remove the download quarantine flag;
# otherwise macOS refuses to load the plug-in inside Ableton Live.
xattr -dr com.apple.quarantine "$VST3_DIR/Dave Grain.vst3" "$AU_DIR/Dave Grain.component" "$APP_DIR/Dave Grain.app" 2>/dev/null || true

# Make macOS re-scan Audio Units.
killall -9 AudioComponentRegistrar 2>/dev/null || true

mkdir -p "$HOME/Music/Dave Grain/Presets"

echo ""
echo "Done!"
echo "  VST3:       $VST3_DIR/Dave Grain.vst3"
echo "  AU:         $AU_DIR/Dave Grain.component"
echo "  Standalone: $APP_DIR/Dave Grain.app"
echo "  Presets:    $HOME/Music/Dave Grain/Presets"
echo ""
echo "In Ableton Live: Settings > Plug-Ins > turn on 'Use VST3 Plug-in System Folders'"
echo "and 'Use Audio Units', then click Rescan. Dave Grain appears under Plug-Ins."
echo ""
read -n 1 -s -r -p "Press any key to close."
