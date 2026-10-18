// Apprehension - themes. One built-in theme (black & pink); everything else is added by users as small .json files.
#pragma once
#include <algorithm>
#include <vector>
#include <juce_gui_basics/juce_gui_basics.h>

struct Theme {
    juce::String name = "Black & Pink";
    juce::Colour bgTop = juce::Colour(0xff050205), bgMid = juce::Colour(0xff1a0612), bgBottom = juce::Colour(0xff4a0f2e);
    juce::Colour glowA = juce::Colour(0xffff2e93), glowB = juce::Colour(0xffff8cc2);   // bokeh / haze tints
    juce::Colour accent = juce::Colour(0xffff3d9a), accent2 = juce::Colour(0xffffa6d2); // knob arcs, highlights
    juce::Colour panel = juce::Colour(0xff12050d);
    float panelAlpha = 0.74f;
    juce::Colour text = juce::Colour(0xfff8e8f0), textDim = juce::Colour(0xffb9879f);
    juce::Colour label = juce::Colour(0xffff86bd), value = juce::Colour(0xffffffff);
    juce::String font;            // optional installed typeface name
    juce::File image;             // optional still picture (slow cinematic pan/zoom)
    juce::File frames;            // optional folder of numbered pictures, played as a loop
    int fps = 24;
    float dim = 0.55f;            // how much to darken picture/footage so the controls stay readable
};

namespace themes {

// Signature Mode look: the whole UI turns hot pink (the logo's pink) on black.
inline Theme signatureTheme() {
    Theme t;
    t.name = "Signature";
    t.bgTop = juce::Colour(0xff000000); t.bgMid = juce::Colour(0xff060004); t.bgBottom = juce::Colour(0xff160010);
    t.glowA = juce::Colour(0xffff1f8f); t.glowB = juce::Colour(0xffff5cb0);
    t.accent = juce::Colour(0xffff1f8f); t.accent2 = juce::Colour(0xffff6ec0);
    t.panel = juce::Colour(0xff050003); t.panelAlpha = 0.86f;
    t.text = juce::Colour(0xffffe3f1); t.textDim = juce::Colour(0xffc4528f);
    t.label = juce::Colour(0xffff4da6); t.value = juce::Colour(0xffffffff);
    return t;
}

// Smooth blend of two themes' colours (everything else comes from a).
inline Theme blend(const Theme& a, const Theme& b, float t) {
    Theme r = a;
    auto m = [&](juce::Colour x, juce::Colour y) { return x.interpolatedWith(y, t); };
    r.bgTop = m(a.bgTop, b.bgTop); r.bgMid = m(a.bgMid, b.bgMid); r.bgBottom = m(a.bgBottom, b.bgBottom);
    r.glowA = m(a.glowA, b.glowA); r.glowB = m(a.glowB, b.glowB);
    r.accent = m(a.accent, b.accent); r.accent2 = m(a.accent2, b.accent2);
    r.panel = m(a.panel, b.panel); r.panelAlpha = a.panelAlpha + (b.panelAlpha - a.panelAlpha) * t;
    r.text = m(a.text, b.text); r.textDim = m(a.textDim, b.textDim);
    r.label = m(a.label, b.label); r.value = m(a.value, b.value);
    return r;
}

inline juce::File folder() {
    // Windows: %APPDATA%\Reyaan\Apprehension\Themes     macOS: ~/Library/Application Support/Reyaan/Apprehension/Themes
    auto base = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
   #if JUCE_MAC
    base = base.getChildFile("Application Support");
   #endif
    return base.getChildFile("Reyaan").getChildFile("Apprehension").getChildFile("Themes");
}

inline juce::Colour parseColour(const juce::var& v, juce::Colour def) {
    juce::String s = v.toString().trim();
    if (s.startsWithChar('#')) s = s.substring(1);
    if (s.isEmpty() || !s.containsOnly("0123456789abcdefABCDEF")) return def;
    if (s.length() == 6) s = "ff" + s;
    if (s.length() != 8) return def;
    return juce::Colour::fromString(s);
}

inline Theme parse(const juce::File& file) {
    Theme t;
    juce::var v = juce::JSON::parse(file);
    if (!v.isObject()) { t.name = file.getFileNameWithoutExtension(); return t; }
    t.name = v.getProperty("name", file.getFileNameWithoutExtension()).toString();
    auto pair = [&](const char* key, juce::Colour& a, juce::Colour& b, juce::Colour* c = nullptr) {
        juce::var arr = v.getProperty(key, juce::var());
        if (arr.isArray()) {
            if (arr.size() > 0) a = parseColour(arr[0], a);
            if (arr.size() > 1) b = parseColour(arr[1], b);
            if (c && arr.size() > 2) *c = parseColour(arr[2], *c);
        }
    };
    pair("background", t.bgTop, t.bgMid, &t.bgBottom);
    pair("glow", t.glowA, t.glowB);
    pair("accent", t.accent, t.accent2);
    t.panel = parseColour(v.getProperty("panel", juce::var()), t.panel);
    t.text = parseColour(v.getProperty("text", juce::var()), t.text);
    t.textDim = parseColour(v.getProperty("textDim", juce::var()), t.textDim);
    t.label = parseColour(v.getProperty("label", juce::var()), t.label);
    t.value = parseColour(v.getProperty("value", juce::var()), t.value);
    if (v.hasProperty("panelAlpha")) t.panelAlpha = juce::jlimit(0.2f, 1.0f, (float)(double)v["panelAlpha"]);
    if (v.hasProperty("dim")) t.dim = juce::jlimit(0.0f, 0.95f, (float)(double)v["dim"]);
    if (v.hasProperty("fps")) t.fps = juce::jlimit(1, 60, (int)v["fps"]);
    t.font = v.getProperty("font", juce::var()).toString();
    auto resolve = [&](const char* key) {
        juce::String p = v.getProperty(key, juce::var()).toString().trim();
        if (p.isEmpty()) return juce::File();
        return file.getParentDirectory().getChildFile(p);   // absolute paths also work with getChildFile
    };
    t.image = resolve("image");
    t.frames = resolve("frames");
    return t;
}

inline std::vector<Theme> loadAll() {
    std::vector<Theme> out;
    auto dir = folder();
    if (!dir.isDirectory()) return out;
    for (auto& f : dir.findChildFiles(juce::File::findFiles, false, "*.json")) {
        if (f.getFileName().startsWithIgnoreCase("_")) continue;   // files starting with _ are ignored
        out.push_back(parse(f));
    }
    std::sort(out.begin(), out.end(), [](const Theme& a, const Theme& b) { return a.name.compareIgnoreCase(b.name) < 0; });
    return out;
}

inline const char* templateText() {
    return R"({
  "name": "My Theme",
  "_help": "Colours are hex like #RRGGBB. Delete any line you do not need. 'image' = a still picture that slowly pans and zooms. 'frames' = a folder of numbered pictures (for example exported from your own video) played as a loop at 'fps'. Paths are relative to this file. Save, then press Reload themes in the plugin.",
  "background": ["#050205", "#1a0612", "#4a0f2e"],
  "glow": ["#ff2e93", "#ff8cc2"],
  "accent": ["#ff3d9a", "#ffa6d2"],
  "panel": "#12050d",
  "panelAlpha": 0.74,
  "text": "#f8e8f0",
  "textDim": "#b9879f",
  "label": "#ff86bd",
  "value": "#ffffff",
  "font": "",
  "image": "",
  "frames": "",
  "fps": 24,
  "dim": 0.55
}
)";
}

}  // namespace themes
