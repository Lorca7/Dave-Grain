DAVE GRAIN — granular synthesizer (VST3 / Audio Unit / Standalone) for macOS 11+

INSTALL (the reliable way, works even if macOS says "can't be opened")
  1. Open Terminal (Applications > Utilities > Terminal).
  2. Type  bash  followed by a space, then drag "Install.command" from this
     folder into the Terminal window, and press Return.
     (Alternative: right-click "Install.command" > Open > Open.)
  3. In Ableton Live: Settings (Preferences) > Plug-Ins
       - turn on "Use VST3 Plug-in System Folders"
       - turn on "Use Audio Units"
       - click "Rescan"
  4. Browser > Plug-Ins > VST3 (or Audio Units) > Dave Grain Audio > Dave Grain.
     Drag it onto a MIDI track.

  Why: this build isn't notarised by Apple, so macOS quarantines it when it is
  downloaded. The installer copies everything into place and removes that flag.
  Don't double-click the files inside this folder directly.

MANUAL INSTALL (instead of step 1)
  Dave Grain.vst3      -> ~/Library/Audio/Plug-Ins/VST3/
  Dave Grain.component -> ~/Library/Audio/Plug-Ins/Components/
  Dave Grain.app       -> /Applications (or ~/Applications)
  Then in Terminal:  xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"Dave Grain.vst3"
                     xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/"Dave Grain.component"

LOADING SAMPLES
  Drag a WAV / AIFF / FLAC file from Live's browser or from Finder onto the
  plug-in window, or click "Load Sample". Presets: ~/Music/Dave Grain/Presets
