DAVE GRAIN — granular synthesizer (VST3 / Audio Unit / Standalone) for macOS 11+

INSTALL
  1. Right-click "Install.command" and choose Open (the first time macOS asks
     for confirmation because the plug-in isn't notarised by Apple).
  2. In Ableton Live: Settings (Preferences) > Plug-Ins
       - turn on "Use VST3 Plug-in System Folders"
       - turn on "Use Audio Units"
       - click "Rescan"
  3. Browser > Plug-Ins > VST3 (or Audio Units) > Dave Grain Audio > Dave Grain.
     Drag it onto a MIDI track.

MANUAL INSTALL (instead of step 1)
  Dave Grain.vst3      -> ~/Library/Audio/Plug-Ins/VST3/
  Dave Grain.component -> ~/Library/Audio/Plug-Ins/Components/
  Dave Grain.app       -> /Applications (or ~/Applications)
  Then in Terminal:  xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"Dave Grain.vst3"
                     xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/"Dave Grain.component"

LOADING SAMPLES
  Drag a WAV / AIFF / FLAC file from Live's browser or from Finder onto the
  plug-in window, or click "Load Sample". Presets: ~/Music/Dave Grain/Presets
