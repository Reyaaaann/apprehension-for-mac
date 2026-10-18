# Apprehension (VST3) - Designed by Reyaan.

Reverse effect for FL Studio and any VST3 host, on Windows and macOS (Apple Silicon + Intel; VST3 and Audio Unit). The window is resizable (drag a corner). Hover over any control for help;
the full user manual is in `docs/` (HTML and PDF), is built into the plugin (Manual button) and is installed with it.

## Build
Push to GitHub; Actions builds both:
- Windows: `Apprehension-Setup.exe` (artifact "Apprehension-Windows").
- macOS: `Apprehension-macOS.pkg` plus a zip of the two plugins (artifact "Apprehension-macOS"). See `mac/README_MAC.txt`.
  Unsigned builds work, but macOS asks you to right-click > Open the installer once. For a smooth install you need a paid Apple
  Developer account: add the secrets named in `.github/workflows/build.yml` (APPLE_CERT_BASE64, APPLE_CERT_PASSWORD,
  APPLE_APP_IDENTITY, APPLE_INSTALLER_IDENTITY, APPLE_ID, APPLE_TEAM_ID, APPLE_APP_PASSWORD) and the workflow signs and notarizes.
Then in FL Studio: Options > Manage plugins > Find installed plugins.

## What is in it
- Reverse engine: Sync/Division/Time, **Time Align** (locks chunks to the host beat grid), Attack, Cutoff + Release, Smooth, Feedback, Freeze.
- **Overall Pitch** (+-12 st, whole signal, fixed 25 ms latency reported to the host) and Reverse Pitch (+-24 st, reversed layer only)
  with Hz reference readout and key detector.
- 20 sample-flipping presets (preset bar in the header), Reverb section, filters with 6 dB/oct to Brickwall shapes and resonance,
  Chorus (5 types) and Flangus (5 types), ducking (input or beat-synced pump), @Reyaan. Signature Mode auto-chop with a clickable
  16-step editor, Soft Clip, themes.

## Maintaining
- Parameters (ids, ranges, defaults, choices) live in `src/params.h`; presets in `src/presets.h`.
- After changing parameters or presets, regenerate the manual and tooltips:
  `g++ -std=c++17 tools/dump_specs.cpp -o /tmp/dump && python3 tools/gen_docs.py /tmp/dump`
  (writes `src/HelpText.h`, `assets/manual.html`, `docs/Apprehension Manual.html`).
- Tests (any OS): `g++ -std=c++17 -O2 test/test_dsp.cpp -o t && ./t`

## Adding themes (nothing extra ships with the plugin)
Theme button > "Create a theme template", edit the .json in `%APPDATA%\Reyaan\Apprehension\Themes` (Windows) or `~/Library/Application Support/Reyaan/Apprehension/Themes` (Mac), then Theme > "Reload themes".
A theme can set colours, panel opacity, a font name and an optional picture / frame sequence (your own footage; use only what you own).

## Cursive font
The plugin uses the first installed of: Apple Chancery, Snell Roundhand (Mac), Monotype Corsiva, Palace Script MT, Edwardian Script ITC, Brush Script MT, Segoe Script,
Lucida Handwriting. To ship one font inside the plugin, put a .ttf with a licence that allows it at `assets/font.ttf` and rebuild.
