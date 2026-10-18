Apprehension for macOS - Designed by Reyaan.

EASIEST: use Apprehension-macOS.pkg
  1. Double-click it. If macOS says it cannot be opened because it is from an unidentified developer, right-click
     (or Control-click) the file, choose Open, then Open again. (Or: System Settings > Privacy & Security > Open Anyway.)
  2. Follow the installer. It puts:
       Apprehension.vst3       in /Library/Audio/Plug-Ins/VST3
       Apprehension.component  in /Library/Audio/Plug-Ins/Components   (Audio Unit, for Logic / GarageBand)
       the user manual         in /Library/Application Support/Reyaan/Apprehension
  3. In FL Studio: Options > Manage plugins > Find installed plugins. Then add Apprehension from a mixer effect slot.

MANUAL INSTALL (if you only have the two plugin files):
  Copy Apprehension.vst3 to /Library/Audio/Plug-Ins/VST3 and Apprehension.component to /Library/Audio/Plug-Ins/Components,
  then open Terminal and run:
       xattr -cr /Library/Audio/Plug-Ins/VST3/Apprehension.vst3
       xattr -cr /Library/Audio/Plug-Ins/Components/Apprehension.component
  (This removes macOS's "downloaded from the internet" block.) Then rescan plugins in your host.

Works on Apple Silicon and Intel Macs (macOS 10.13 or newer).
