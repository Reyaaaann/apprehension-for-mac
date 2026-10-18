// Apprehension - Designed by Reyaan.
// User interface: cinematic animated backdrop, neon-pink knobs, cursive type, filter graph, chop editor, themes.
#pragma once
#include <BinaryData.h>
#include <cmath>
#include <memory>
#include "PluginProcessor.h"
#include "Theme.h"
#include "HelpText.h"
#include "presets.h"

#ifndef APPREHENSION_CUSTOM_FONT
#define APPREHENSION_CUSTOM_FONT 0
#endif

namespace ui {

using namespace juce;

constexpr int kW = 1440, kH = 996;   // design size; the window scales this to whatever size you drag it to

// ------------------------------------------------------------------------------ fonts
// Cursive face: the first installed one from a classic-to-readable list (or the theme's own choice).
// Numbers use a clean italic serif so values stay easy to read.
inline String gFontName;
inline float gFontScale = 1.f;

inline void chooseFont(const String& themeFont) {
    const StringArray all = Font::findAllTypefaceNames();
    auto has = [&](const String& n) { for (auto& a : all) if (a.equalsIgnoreCase(n)) return true; return false; };
    struct C { const char* name; float scale; };
    static const C cands[] = {{"Apple Chancery", 1.12f}, {"Snell Roundhand", 1.15f},                    // macOS
                              {"Monotype Corsiva", 1.12f}, {"Palace Script MT", 1.38f}, {"Edwardian Script ITC", 1.3f},
                              {"Brush Script MT", 1.2f}, {"Segoe Script", 0.86f}, {"Lucida Handwriting", 0.8f}};
    gFontName = String();
    gFontScale = 1.f;
    if (themeFont.isNotEmpty() && has(themeFont)) { gFontName = themeFont; return; }
    for (auto& c : cands)
        if (has(c.name)) { gFontName = c.name; gFontScale = c.scale; return; }
}

#if APPREHENSION_CUSTOM_FONT
inline Typeface::Ptr customFace() {
    static Typeface::Ptr t = Typeface::createSystemTypefaceFor(BinaryData::font_ttf, (size_t)BinaryData::font_ttfSize);
    return t;
}
#endif

inline Font F(float h, bool bold = false) {
#if APPREHENSION_CUSTOM_FONT
    if (customFace() != nullptr) return Font(FontOptions(customFace())).withHeight(h * 1.15f);
#endif
    if (gFontName.isNotEmpty()) return Font(FontOptions(gFontName, h * gFontScale, bold ? Font::bold : Font::plain));
    return Font(FontOptions(h, Font::italic | (bold ? Font::bold : 0)));
}
inline Font FV(float h) { return Font(FontOptions("Georgia", h, Font::italic)); }

inline const char* const kNoteNames[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

inline String noteName(double hz) {
    if (hz <= 0.0) return "-";
    double m = 69.0 + 12.0 * std::log2(hz / 440.0);
    int r = (int)std::lround(m);
    int cents = (int)std::lround((m - r) * 100.0);
    String s = String(kNoteNames[((r % 12) + 12) % 12]) + String((int)std::floor(r / 12.0) - 1);
    if (cents != 0) s << (cents > 0 ? " +" : " ") << cents << "c";
    return s;
}

// ------------------------------------------------------------------------------ layout
inline Rectangle<int> panel(int i) {
    static const int r[9][4] = {{16, 104, 864, 150}, {892, 104, 532, 150}, {16, 264, 864, 150}, {892, 264, 532, 150},
                                {16, 424, 864, 150}, {892, 424, 532, 150}, {16, 584, 864, 150}, {892, 584, 532, 150},
                                {16, 744, 1408, 236}};
    return Rectangle<int>(r[i][0], r[i][1], r[i][2], r[i][3]);
}
inline const char* panelTitle(int i) {
    static const char* t[9] = {"Time", "Pitch", "Space & Modulation", "Ducking", "Reverb", "Key Detector", "Filter", "Output", "@Reyaan. Signature Chop"};
    return t[i];
}

// the 16 chop steps currently shown (what is playing / what a click will edit)
inline void displayedPattern(ApprehensionProcessor& proc, char out[16]) {
    const int mode = juce::jlimit(0, 9, (int)std::lround(proc.get(P_CHOPPATTERN)));
    for (int k = 0; k < 16; ++k) {
        if (mode == 9) out[k] = proc.chopCell(k);
        else if (mode >= 1) out[k] = apprehension::kChop[mode - 1][k];
        else out[k] = apprehension::kChop[proc.chopPattern() & 7][k];
    }
}

// ------------------------------------------------------------------------------ look & feel
class NeonLook final : public LookAndFeel_V4 {
public:
    explicit NeonLook(const Theme& t) : th(t) { refresh(); }

    void refresh() {
        setColour(Label::textColourId, th.value);
        setColour(TextEditor::backgroundColourId, th.panel);
        setColour(TextEditor::textColourId, th.value);
        setColour(TextEditor::highlightColourId, th.accent.withAlpha(0.4f));
        setColour(TextEditor::outlineColourId, th.accent.withAlpha(0.5f));
        setColour(TextEditor::focusedOutlineColourId, th.accent);
        setColour(PopupMenu::backgroundColourId, th.panel.darker(0.3f));
        setColour(PopupMenu::textColourId, th.text);
        setColour(PopupMenu::headerTextColourId, th.label);
        setColour(PopupMenu::highlightedBackgroundColourId, th.accent.withAlpha(0.55f));
        setColour(PopupMenu::highlightedTextColourId, Colours::white);
        setColour(TooltipWindow::backgroundColourId, th.panel.darker(0.4f).withAlpha(0.96f));
        setColour(TooltipWindow::textColourId, th.text);
        setColour(TooltipWindow::outlineColourId, th.accent.withAlpha(0.7f));
    }

    Font getPopupMenuFont() override { return F(18.f); }
    Font getTextButtonFont(TextButton&, int) override { return F(18.f); }

    void drawRotarySlider(Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, Slider& s) override {
        auto b = Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(8.f);
        const float r = std::min(b.getWidth(), b.getHeight()) * 0.5f;
        const auto c = b.getCentre();
        const float ang = a0 + pos * (a1 - a0);
        const bool hot = s.isMouseOverOrDragging();

        g.setGradientFill(ColourGradient(th.panel.brighter(0.35f), c.x - r * 0.4f, c.y - r * 0.6f, Colours::black,
                                         c.x + r * 0.5f, c.y + r * 0.7f, false));
        g.fillEllipse(c.x - r * 0.76f, c.y - r * 0.76f, r * 1.52f, r * 1.52f);
        g.setColour(th.accent2.withAlpha(hot ? 0.45f : 0.18f));
        g.drawEllipse(c.x - r * 0.76f, c.y - r * 0.76f, r * 1.52f, r * 1.52f, 1.2f);

        const float tr = r * 0.94f;
        Path track;
        track.addCentredArc(c.x, c.y, tr, tr, 0.f, a0, a1, true);
        g.setColour(Colours::white.withAlpha(0.10f));
        g.strokePath(track, PathStrokeType(3.5f, PathStrokeType::curved, PathStrokeType::rounded));

        float from = a0;
        if (s.getMinimum() < 0.0 && s.getMaximum() > 0.0)
            from = a0 + (float)s.valueToProportionOfLength(0.0) * (a1 - a0);
        Path val;
        val.addCentredArc(c.x, c.y, tr, tr, 0.f, std::min(from, ang), std::max(from, ang), true);
        g.setColour(th.accent.withAlpha(hot ? 0.42f : 0.26f));
        g.strokePath(val, PathStrokeType(9.f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setGradientFill(ColourGradient(th.accent2, c.x - tr, c.y, th.accent, c.x + tr, c.y, false));
        g.strokePath(val, PathStrokeType(3.5f, PathStrokeType::curved, PathStrokeType::rounded));

        const float sn = std::sin(ang), cs = std::cos(ang);
        g.setColour(th.value);
        g.drawLine(Line<float>(c.x + r * 0.28f * sn, c.y - r * 0.28f * cs, c.x + r * 0.68f * sn, c.y - r * 0.68f * cs), 2.6f);
    }

    void drawToggleButton(Graphics& g, ToggleButton& b, bool over, bool) override {
        auto r = b.getLocalBounds().toFloat().reduced(3.f);
        const bool on = b.getToggleState();
        const String nm = b.getName();
        const float rad = r.getHeight() * 0.5f;
        const Colour base = nm == "freeze" ? th.accent2 : th.accent;
        if (on) {
            g.setColour(base.withAlpha(0.22f));
            g.drawRoundedRectangle(r.expanded(3.f), rad + 3.f, 5.f);
        }
        if (on && nm == "sig")
            g.setGradientFill(ColourGradient(th.accent.withAlpha(0.55f), r.getX(), r.getY(), th.accent2.withAlpha(0.45f), r.getRight(), r.getBottom(), false));
        else
            g.setColour(on ? base.withAlpha(0.32f) : Colours::white.withAlpha(over ? 0.12f : 0.06f));
        g.fillRoundedRectangle(r, rad);
        g.setColour(on ? base : th.textDim.withAlpha(0.7f));
        g.drawRoundedRectangle(r, rad, on ? 2.f : 1.2f);
        g.setColour(on ? Colours::white : th.text.withAlpha(0.75f));
        g.setFont(F(nm == "sig" ? 24.f : (nm == "freeze" ? 26.f : (nm == "align" ? 17.f : 19.f))));
        g.drawText(b.getButtonText(), r.toNearestInt(), Justification::centred);
    }

    Label* createSliderTextBox(Slider& s) override {
        auto* l = LookAndFeel_V4::createSliderTextBox(s);
        l->setFont(FV(13.f));
        l->setColour(Label::textColourId, th.value.withAlpha(0.92f));
        l->setColour(Label::backgroundColourId, Colours::transparentBlack);
        l->setColour(Label::outlineColourId, Colours::transparentBlack);
        l->setJustificationType(Justification::centred);
        return l;
    }

private:
    const Theme& th;
};

// ------------------------------------------------------------------------ cinematic backdrop
// Slow, filmic and moody: drifting bokeh lights, soft light leaks, dust and film grain over a deep gradient.
// A theme can add its own still picture or a frame sequence (your own footage) underneath.
// Signature Mode: the scene goes black with hot-pink flashes that follow the chop steps.
class Backdrop final : public Component, private Timer {
public:
    Backdrop(ApprehensionProcessor& p, const Theme& t) : proc(p), th(t) {
        setOpaque(true);
        setInterceptsMouseClicks(false, false);
        makeGrain();
        startTimerHz(30);
    }
    ~Backdrop() override { stopTimer(); loader.reset(); }

    void themeChanged() {
        loader.reset();
        frames.clear();
        still = Image();
        build();
        if (th.image.existsAsFile() || th.frames.isDirectory()) {
            loader = std::make_unique<Loader>(SafePointer<Backdrop>(this), th.image, th.frames);
            loader->startThread();
        }
    }

    void setFootage(const std::vector<Image>& f, const Image& s) { frames = f; still = s; }

    void resized() override { build(); }

    void paint(Graphics& g) override {
        const float W = (float)getWidth(), H = (float)getHeight();
        if (statics.isNull()) { g.fillAll(th.bgTop); return; }
        g.drawImageAt(statics, 0, 0);

        // optional footage / still picture
        const bool footage = !frames.empty(), picture = !footage && still.isValid();
        if (footage) {
            g.setImageResamplingQuality(Graphics::lowResamplingQuality);
            const double f = (double)t * th.fps;
            const int n = (int)frames.size(), i0 = ((int)std::floor(f)) % n, i1 = (i0 + 1) % n;
            const float a = (float)(f - std::floor(f));
            auto& A = frames[(size_t)i0]; auto& B = frames[(size_t)i1];
            g.setOpacity(1.f);
            g.drawImage(A, 0, 0, getWidth(), getHeight(), 0, 0, A.getWidth(), A.getHeight());
            g.setOpacity(a);
            g.drawImage(B, 0, 0, getWidth(), getHeight(), 0, 0, B.getWidth(), B.getHeight());
        } else if (picture) {
            g.setImageResamplingQuality(Graphics::mediumResamplingQuality);
            const float s = 1.10f + 0.05f * std::sin(t * 0.07f);
            const float dw = W * s, dh = H * s;
            const float dx = (W - dw) * 0.5f + std::sin(t * 0.045f) * W * 0.03f, dy = (H - dh) * 0.5f + std::cos(t * 0.038f) * H * 0.025f;
            g.setOpacity(1.f);
            g.drawImage(still, (int)dx, (int)dy, (int)dw, (int)dh, 0, 0, still.getWidth(), still.getHeight());
        }
        if (footage || picture) {
            g.setColour(th.bgTop.withAlpha(th.dim));
            g.fillAll();
            g.setColour(th.glowA.withAlpha(0.10f));
            g.fillAll();
        }

        // drifting haze, breathing with the output level
        for (int k = 0; k < 2; ++k) {
            const float cx = W * (0.30f + 0.42f * (float)k) + std::sin(t * 0.10f + (float)k * 2.1f) * W * 0.16f;
            const float cy = H * (0.35f + 0.25f * (float)k) + std::cos(t * 0.08f + (float)k) * H * 0.06f;
            const Colour c = k ? th.glowB : th.glowA;
            g.setGradientFill(ColourGradient(c.withAlpha(0.10f + 0.14f * lvl), cx, cy, c.withAlpha(0.f), cx + W * 0.40f, cy, true));
            g.fillRect(0, 0, getWidth(), getHeight());
        }

        // soft light leaks
        for (int k = 0; k < 2; ++k) {
            const float x0 = W * (0.15f + 0.5f * (float)k) + std::sin(t * 0.06f + (float)k * 3.f) * W * 0.12f;
            Path beam;
            beam.startNewSubPath(x0, 0.f);
            beam.lineTo(x0 + 140.f, 0.f);
            beam.lineTo(x0 + 420.f, H);
            beam.lineTo(x0 + 200.f, H);
            beam.closeSubPath();
            const Colour c = k ? th.glowB : th.glowA;
            g.setGradientFill(ColourGradient(c.withAlpha(0.00f), x0, 0.f, c.withAlpha(0.07f + 0.07f * lvl), x0 + 300.f, H * 0.6f, false));
            g.fillPath(beam);
        }

        // bokeh
        for (auto& b : bokeh) {
            const float breath = 0.65f + 0.35f * std::sin(t * b.rate + b.phase);
            const float a = b.alpha * breath * (1.f + 0.9f * lvl);
            const Colour c = b.warm ? th.glowB : th.glowA;
            g.setGradientFill(ColourGradient(c.withAlpha(a), b.x, b.y, c.withAlpha(a * 0.15f), b.x + b.r, b.y, true));
            g.fillEllipse(b.x - b.r, b.y - b.r, b.r * 2.f, b.r * 2.f);
            g.setColour(c.withAlpha(a * 0.9f));
            g.drawEllipse(b.x - b.r, b.y - b.r, b.r * 2.f, b.r * 2.f, 1.3f);
        }

        // dust motes
        for (auto& d : dust) {
            const float a = 0.15f + 0.5f * (0.5f + 0.5f * std::sin(t * d.rate + d.phase));
            g.setColour(th.accent2.withAlpha(a * 0.55f));
            g.fillEllipse(d.x, d.y, d.r, d.r);
        }

        // Signature Mode: black out the scene, leave a faint pink edge glow, flash on chop hits
        if (sig > 0.01f) {
            g.setColour(Colours::black.withAlpha(0.92f * sig));
            g.fillAll();
            g.setGradientFill(ColourGradient(Colour(0xffff1f8f).withAlpha(0.f), W * 0.5f, H * 0.5f, Colour(0xffff1f8f).withAlpha(0.20f * sig), 0.f, 0.f, true));
            g.fillAll();
            if (flash > 0.01f) {
                g.setColour(Colour(0xffff1f8f).withAlpha(0.26f * flash));
                g.fillAll();
                g.setGradientFill(ColourGradient(Colour(0xffff4da6).withAlpha(0.50f * flash), W * 0.5f, H * 0.38f, Colour(0xffff1f8f).withAlpha(0.f), W * 0.5f, H * 1.0f, true));
                g.fillAll();
            }
        }

        // film grain
        g.setTiledImageFill(grain[(size_t)grainIdx], gx, gy, 1.f);
        g.fillAll();

        // freeze frosts the picture
        if (frz > 0.01f) {
            g.setColour(Colours::white.withAlpha(0.07f * frz));
            g.fillAll();
            g.setGradientFill(ColourGradient(Colours::white.withAlpha(0.f), W * 0.5f, H * 0.5f, th.accent2.withAlpha(0.35f * frz), 0.f, 0.f, true));
            g.fillAll();
        }
    }

private:
    struct Bokeh { float x, y, r, vx, vy, phase, rate, alpha; bool warm; };
    struct Dust { float x, y, r, vy, phase, rate; };

    class Loader final : public Thread {
    public:
        Loader(SafePointer<Backdrop> t, File img, File frm)
            : Thread("ApprehensionFootage"), target(t), imageFile(img), framesDir(frm) {}
        ~Loader() override { stopThread(5000); }

        void run() override {
            auto out = std::make_shared<std::vector<Image>>();
            Image stillImg;
            if (imageFile.existsAsFile()) stillImg = prep(ImageFileFormat::loadFrom(imageFile), 1400);
            if (framesDir.isDirectory()) {
                auto files = framesDir.findChildFiles(File::findFiles, false, "*.jpg;*.jpeg;*.png");
                std::sort(files.begin(), files.end(), [](const File& a, const File& b) { return a.getFileName().compareNatural(b.getFileName()) < 0; });
                const int total = files.size(), maxFrames = 72;
                const double stride = std::max(1.0, (double)total / maxFrames);   // very long sequences are thinned out
                for (double i = 0; i < total; i += stride) {
                    if (threadShouldExit()) return;
                    Image f = prep(ImageFileFormat::loadFrom(files[(int)i]), 640);
                    if (f.isValid()) out->push_back(f);
                }
            }
            if (threadShouldExit()) return;
            MessageManager::callAsync([t = target, out, stillImg]() { if (t != nullptr) t->setFootage(*out, stillImg); });
        }

    private:
        static Image prep(const Image& src, int outW) {
            if (src.isNull()) return {};
            const double aspect = (double)kW / (double)kH;
            int w = src.getWidth(), h = src.getHeight(), cw = w, ch = h;
            if ((double)w / h > aspect) cw = (int)(h * aspect); else ch = (int)(w / aspect);
            Image c = src.getClippedImage(Rectangle<int>((w - cw) / 2, (h - ch) / 2, cw, ch));
            Image out(Image::RGB, outW, (int)(outW / aspect), false, SoftwareImageType());
            Graphics g(out);
            g.setImageResamplingQuality(Graphics::mediumResamplingQuality);
            g.drawImage(c, 0, 0, out.getWidth(), out.getHeight(), 0, 0, c.getWidth(), c.getHeight());
            return out;
        }
        SafePointer<Backdrop> target;
        File imageFile, framesDir;
    };

    void timerCallback() override {
        const float dt = 1.f / 30.f;
        t += dt;
        const float target = std::min(1.f, proc.outLevel.load() * 2.f);
        lvl += (target - lvl) * 0.25f;
        frz += ((proc.get(P_FREEZE) >= 0.5f ? 1.f : 0.f) - frz) * 0.08f;
        const bool sigOn = proc.get(P_SIGNATURE) >= 0.5f;
        sig += ((sigOn ? 1.f : 0.f) - sig) * 0.12f;

        // pink flashes: tied to chop hits, rate-limited to roughly 3 per second at most
        flash *= 0.80f;
        if (sigOn) {
            const int step = proc.chopStep();
            if (step >= 0 && step != lastStep) {
                lastStep = step;
                if (proc.chopAppliedAction() != 'N' && t - lastFlashT > 0.34f && rng.nextFloat() < 0.45f) {
                    flash = 0.6f + 0.4f * rng.nextFloat(); lastFlashT = t;
                }
            }
            if (t - lastFlashT > 0.7f && rng.nextFloat() < 0.01f) { flash = 0.5f + 0.5f * rng.nextFloat(); lastFlashT = t; }
        } else lastStep = -1;

        const float speed = (1.f + lvl * 1.2f) * (1.f - 0.95f * frz);
        const float W = (float)getWidth(), H = (float)getHeight();
        for (auto& b : bokeh) {
            b.x += b.vx * dt * speed; b.y += b.vy * dt * speed;
            if (b.x < -b.r) b.x = W + b.r;
            if (b.x > W + b.r) b.x = -b.r;
            if (b.y < -b.r) b.y = H + b.r;
            if (b.y > H + b.r) b.y = -b.r;
        }
        for (auto& d : dust) {
            d.y -= d.vy * dt * speed;
            d.x += std::sin(t * 0.5f + d.phase) * 4.f * dt * speed;
            if (d.y < -4.f) { d.y = H + 4.f; d.x = rng.nextFloat() * W; }
        }
        if (++grainTick >= 2) { grainTick = 0; grainIdx = rng.nextInt(4); gx = rng.nextInt(128); gy = rng.nextInt(128); }
        repaint();
    }

    float rf(float a, float b) { return a + rng.nextFloat() * (b - a); }

    void makeGrain() {
        for (auto& gi : grain) {
            gi = Image(Image::ARGB, 128, 128, true, SoftwareImageType());
            Image::BitmapData bd(gi, Image::BitmapData::writeOnly);
            for (int y = 0; y < 128; ++y)
                for (int x = 0; x < 128; ++x) {
                    const int a = rng.nextInt(26);
                    bd.setPixelColour(x, y, rng.nextBool() ? Colour::fromRGBA(255, 255, 255, (uint8)a) : Colour::fromRGBA(0, 0, 0, (uint8)(a * 2)));
                }
        }
    }

    void build() {
        const int Wi = getWidth(), Hi = getHeight();
        if (Wi <= 0 || Hi <= 0) return;
        const float W = (float)Wi, H = (float)Hi;
        rng.setSeed(20241016);
        bokeh.clear(); dust.clear();
        statics = Image(Image::ARGB, Wi, Hi, true);
        Graphics g(statics);

        ColourGradient bg(th.bgTop, 0.f, 0.f, th.bgBottom, 0.f, H, false);
        bg.addColour(0.55, th.bgMid);
        g.setGradientFill(bg);
        g.fillAll();
        g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.f), W * 0.5f, H * 0.5f, Colours::black.withAlpha(0.62f), 0.f, 0.f, true));
        g.fillAll();

        for (int i = 0; i < 22; ++i)
            bokeh.push_back({rf(0.f, W), rf(0.f, H), rf(26.f, 130.f), rf(-6.f, 6.f), rf(-5.f, 3.f), rf(0.f, 6.28f), rf(0.2f, 0.7f), rf(0.05f, 0.17f), rng.nextBool()});
        for (int i = 0; i < 60; ++i)
            dust.push_back({rf(0.f, W), rf(0.f, H), rf(1.f, 2.6f), rf(4.f, 14.f), rf(0.f, 6.28f), rf(0.4f, 1.6f)});
    }

    ApprehensionProcessor& proc;
    const Theme& th;
    Random rng;
    Image statics, still, grain[4];
    std::vector<Image> frames;
    std::vector<Bokeh> bokeh;
    std::vector<Dust> dust;
    std::unique_ptr<Loader> loader;
    float t = 0.f, lvl = 0.f, frz = 0.f, sig = 0.f, flash = 0.f, lastFlashT = -10.f;
    int grainIdx = 0, gx = 0, gy = 0, grainTick = 0, lastStep = -1;
};

// ------------------------------------------------------------------------ filter response graph
class FilterGraph final : public Component, public SettableTooltipClient {
public:
    FilterGraph(ApprehensionProcessor& p, const Theme& t) : proc(p), th(t) { setInterceptsMouseClicks(false, false); }

    void paint(Graphics& g) override {
        const auto r = getLocalBounds().toFloat().reduced(1.f);
        g.setColour(Colours::black.withAlpha(0.45f));
        g.fillRoundedRectangle(r, 8.f);
        g.setColour(th.accent.withAlpha(0.25f));
        g.drawRoundedRectangle(r, 8.f, 1.f);

        const double lowCut = proc.get(P_LOWCUT), highCut = proc.get(P_HIGHCUT);
        const double lowSlope = proc.get(P_LOWSLOPE), highSlope = proc.get(P_HIGHSLOPE);
        const double lowRes = proc.get(P_LOWRES), highRes = proc.get(P_HIGHRES);
        const float L = r.getX() + 8.f, R = r.getRight() - 8.f, T = r.getY() + 8.f, B = r.getBottom() - 16.f;
        auto fx = [&](double f) { return L + (float)(std::log(f / 20.0) / std::log(1000.0)) * (R - L); };
        auto dy = [&](double db) { return T + (float)((15.0 - db) / 63.0) * (B - T); };   // +15 .. -48 dB

        g.setFont(FV(10.f));
        for (double f : {20.0, 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 20000.0}) {
            const float x = fx(f);
            g.setColour(Colours::white.withAlpha(0.07f));
            g.drawVerticalLine((int)x, T, B);
            g.setColour(th.textDim.withAlpha(0.8f));
            g.drawText(f >= 1000 ? String((int)(f / 1000)) + "k" : String((int)f), (int)x - 14, (int)B + 2, 28, 12, Justification::centred);
        }
        for (double db : {12.0, 0.0, -12.0, -24.0, -36.0, -48.0}) {
            g.setColour(Colours::white.withAlpha(db == 0.0 ? 0.18f : 0.07f));
            g.drawHorizontalLine((int)dy(db), L, R);
            g.setColour(th.textDim.withAlpha(0.7f));
            g.drawText(String((int)db), (int)L + 2, (int)dy(db) - 11, 24, 11, Justification::left);
        }

        Path curve, fill;
        bool first = true;
        for (float x = L; x <= R; x += 2.f) {
            const double f = 20.0 * std::pow(1000.0, (double)(x - L) / (double)(R - L));
            const double db = juce::jlimit(-48.0, 15.0, apprehension::cutResponseDb(f, lowCut, lowSlope, lowRes, highCut, highSlope, highRes));
            const float y = dy(db);
            if (first) { curve.startNewSubPath(x, y); fill.startNewSubPath(x, dy(-48.0)); fill.lineTo(x, y); first = false; }
            else { curve.lineTo(x, y); fill.lineTo(x, y); }
        }
        fill.lineTo(R, dy(-48.0));
        fill.closeSubPath();
        g.setGradientFill(ColourGradient(th.accent.withAlpha(0.45f), 0.f, T, th.accent.withAlpha(0.04f), 0.f, B, false));
        g.fillPath(fill);
        g.setColour(th.accent.withAlpha(0.35f));
        g.strokePath(curve, PathStrokeType(6.f));
        g.setColour(th.accent2);
        g.strokePath(curve, PathStrokeType(2.2f, PathStrokeType::curved, PathStrokeType::rounded));

        auto hz = [](double f) { return f >= 1000.0 ? String(f / 1000.0, 2) + " kHz" : String((int)std::lround(f)) + " Hz"; };
        auto marker = [&](double f, const String& txt, bool alignRight) {
            const float x = fx(f);
            g.setColour(th.accent2.withAlpha(0.75f));
            for (float y = T; y < B; y += 6.f) g.fillRect(x, y, 1.2f, 3.f);
            g.fillEllipse(x - 3.5f, dy(-3.0) - 3.5f, 7.f, 7.f);
            g.setFont(F(15.f));
            g.setColour(th.label);
            g.drawText(txt, (int)x + (alignRight ? -150 : 6), (int)T + 2, 144, 18, alignRight ? Justification::right : Justification::left);
        };
        auto slopeName = [](double s) { return String(apprehension::kSlopeNames[juce::jlimit(0, apprehension::kNumSlopes - 1, (int)std::lround(s))]); };
        if (lowCut > 20.5) marker(lowCut, "Low " + hz(lowCut) + "  " + slopeName(lowSlope), false);
        if (highCut < 19999.5) marker(highCut, "High " + hz(highCut) + "  " + slopeName(highSlope), true);
    }

private:
    ApprehensionProcessor& proc;
    const Theme& th;
};

// ------------------------------------------------------------------------ clickable chop step grid
class ChopGrid final : public Component, public SettableTooltipClient {
public:
    ChopGrid(ApprehensionProcessor& p, const Theme& t) : proc(p), th(t) {}

    void paint(Graphics& g) override {
        const bool on = proc.get(P_SIGNATURE) >= 0.5f;
        const int step = proc.chopStep();
        char chars[16];
        displayedPattern(proc, chars);
        const float globalA = on ? 1.f : 0.45f;
        for (int i = 0; i < 16; ++i) {
            const char a = chars[i];
            Colour c;
            String glyph;
            bool allowed = true;
            switch (a) {
                case 'G': c = Colours::black; glyph = "gate"; allowed = proc.get(P_CGATE) >= 0.5f; break;
                case 'S': c = th.accent2; glyph = "stutter"; allowed = proc.get(P_CSTUT) >= 0.5f; break;
                case 'H': c = th.accent; glyph = "half"; allowed = proc.get(P_CHALF) >= 0.5f; break;
                case 'T': c = th.glowA; glyph = "tape"; allowed = proc.get(P_CTAPE) >= 0.5f; break;
                case 'R': c = th.value; glyph = "reverse"; allowed = proc.get(P_CREV) >= 0.5f; break;
                default:  c = th.textDim; glyph = "-"; break;
            }
            const float aa = globalA * (allowed ? 1.f : 0.35f);
            Rectangle<float> r((float)i * 86.f + 2.f, 2.f, 82.f, (float)getHeight() - 4.f);
            const bool playing = on && i == step;
            g.setColour(c.withAlpha((a == 'N' ? 0.16f : (a == 'G' ? 0.60f : 0.38f)) * aa));
            g.fillRoundedRectangle(r, 7.f);
            g.setColour((playing ? th.accent2 : c).withAlpha(playing ? 1.f : 0.55f * aa));
            g.drawRoundedRectangle(r, 7.f, playing ? 2.6f : 1.f);
            if (playing) { g.setColour(th.accent.withAlpha(0.35f)); g.drawRoundedRectangle(r.expanded(2.f), 9.f, 4.f); }
            g.setColour(th.text.withAlpha(0.92f * aa));
            g.setFont(FV(13.f));
            g.drawText(glyph, r.toNearestInt(), Justification::centred);
        }
    }

    void mouseDown(const MouseEvent& e) override {
        const int i = e.x / 86;
        if (i < 0 || i > 15) return;
        char chars[16];
        displayedPattern(proc, chars);
        static const char order[6] = {'N', 'G', 'S', 'H', 'T', 'R'};
        int idx = 0;
        for (int k = 0; k < 6; ++k) if (order[k] == chars[i]) idx = k;
        const int next = (idx + (e.mods.isRightButtonDown() ? 5 : 1)) % 6;
        for (int k = 0; k < 16; ++k) proc.setChopCell(k, chars[k]);   // start editing from what you see
        proc.setChopCell(i, order[next]);
        proc.setChopMode(9);                                            // Pattern = Custom
        repaint();
    }

private:
    ApprehensionProcessor& proc;
    const Theme& th;
};

// ------------------------------------------------- panels, title, logo, readouts
class Overlay final : public Component {
public:
    Overlay(ApprehensionProcessor& p, const Theme& t) : proc(p), th(t) {
        setInterceptsMouseClicks(false, false);
        logo = ImageFileFormat::loadFrom(BinaryData::logo_png, (size_t)BinaryData::logo_pngSize);
        for (auto& d : disp) d = 0.f;
    }

    void paint(Graphics& g) override {
        sigS += ((proc.get(P_SIGNATURE) >= 0.5f ? 1.f : 0.f) - sigS) * 0.12f;
        paintHeader(g);
        for (int i = 0; i < 9; ++i) {
            auto r = panel(i).toFloat();
            g.setGradientFill(ColourGradient(th.panel.withAlpha(th.panelAlpha), r.getX(), r.getY(),
                                             th.panel.darker(0.6f).withAlpha(juce::jmin(1.f, th.panelAlpha + 0.08f)), r.getX(), r.getBottom(), false));
            g.fillRoundedRectangle(r, 14.f);
            g.setColour(th.accent.withAlpha(0.22f));
            g.drawRoundedRectangle(r, 14.f, 1.f);
            g.setColour(th.accent.withAlpha(0.8f));
            g.fillRoundedRectangle(r.getX() + 14.f, r.getY() + 10.f, 3.f, 16.f, 1.5f);
            g.setColour(th.label);
            g.setFont(F(22.f));
            g.drawText(panelTitle(i), (int)r.getX() + 24, (int)r.getY() + 5, 400, 26, Justification::centredLeft);
        }
        paintPitchReadout(g);
        paintDuckText(g);
        paintKey(g);
        paintChopText(g);
        paintLevel(g);
    }

private:
    void txt(Graphics& g, const String& s, int x, int y, int w, int h, Font f, Colour c, Justification j = Justification::centredLeft) {
        g.setColour(c);
        g.setFont(f);
        g.drawText(s, x, y, w, h, j);
    }

    // thin gradient rule with a small diamond - a classic flourish
    void flourish(Graphics& g, float x, float y, float w) {
        g.setGradientFill(ColourGradient(th.accent.withAlpha(0.f), x, y, th.accent2, x + w * 0.5f, y, false));
        g.fillRect(x, y, w * 0.5f - 7.f, 1.2f);
        g.setGradientFill(ColourGradient(th.accent2, x + w * 0.5f, y, th.accent.withAlpha(0.f), x + w, y, false));
        g.fillRect(x + w * 0.5f + 7.f, y, w * 0.5f - 7.f, 1.2f);
        Path d;
        d.addRectangle(-3.2f, -3.2f, 6.4f, 6.4f);
        g.setColour(th.accent2);
        g.fillPath(d, AffineTransform::rotation(MathConstants<float>::pi * 0.25f).translated(x + w * 0.5f, y + 0.6f));
    }

    void paintHeader(Graphics& g) {
        if (logo.isValid()) {
            g.setImageResamplingQuality(Graphics::highResamplingQuality);
            const Rectangle<float> dst(14.f, 4.f, 90.f, 90.f);
            g.setOpacity(1.f - sigS);
            g.drawImage(logo, dst, RectanglePlacement::centred);
            if (sigS > 0.01f) {   // Signature Mode: the logo turns pink
                g.setColour(Colour(0xffff2d95).withAlpha(sigS));
                g.drawImage(logo, dst, RectanglePlacement::centred, true);
            }
        }
        g.setGradientFill(ColourGradient(Colours::white, 116.f, 0.f, th.accent2, 420.f, 0.f, false));
        g.setFont(F(58.f));
        g.drawText("Apprehension", 114, 4, 520, 66, Justification::centredLeft);
        g.setColour(th.accent2);
        g.setFont(F(24.f));
        g.drawText("Designed by Reyaan.", 122, 66, 300, 28, Justification::centredLeft);
        flourish(g, 112.f, 96.f, 330.f);
    }

    void paintPitchReadout(Graphics& g) {
        const double ov = proc.get(P_GPITCH), rev = proc.get(P_PITCH), ref = proc.get(P_REFHZ);
        const double dryHz = ref * std::pow(2.0, ov / 12.0), revHz = ref * std::pow(2.0, (ov + rev) / 12.0);
        const int x = 1244, y = panel(1).getY();
        auto sgn = [](double v) { return String(v >= 0 ? "+" : "") + String(v, 2); };
        txt(g, "Overall pitch", x, y + 26, 180, 22, F(19.f), th.label);
        txt(g, sgn(ov) + " st", x, y + 44, 180, 30, FV(24.f), th.value);
        txt(g, "Reversed layer total  " + sgn(ov + rev) + " st", x, y + 76, 180, 14, FV(11.f), th.textDim);
        txt(g, "Ref   " + String(ref, 2) + " Hz  (" + noteName(ref) + ")", x, y + 94, 180, 14, FV(11.f), th.text);
        txt(g, "Dry  " + String(dryHz, 2) + " Hz  (" + noteName(dryHz) + ")  " + sgn(dryHz - ref), x, y + 110, 180, 14, FV(11.f), th.accent2);
        txt(g, "Rev  " + String(revHz, 2) + " Hz  (" + noteName(revHz) + ")  " + sgn(revHz - ref), x, y + 126, 180, 14, FV(11.f), th.accent);
    }

    void paintDuckText(Graphics& g) {
        const bool pump = proc.get(P_DUCKMODE) >= 0.5f;
        const int idx = juce::jlimit(0, apprehension::kNumDuckDivs - 1, (int)std::lround(proc.get(P_DUCKDIV)));
        const int y = panel(3).getY();
        txt(g, pump ? String("Pump on every ") + apprehension::kDuckNames[idx] : String("Ducks while you play"),
            1180, y + 84, 240, 22, F(19.f), pump ? th.accent2 : th.label);
        txt(g, pump ? "follows the host tempo" : "from your input signal", 1180, y + 106, 240, 16, FV(11.f), th.textDim);
    }

    void paintKey(Graphics& g) {
        const int root = proc.keys.root.load();
        const bool minor = proc.keys.minor.load();
        const double ov = proc.get(P_GPITCH), rev = proc.get(P_PITCH);
        const String mode = minor ? " minor" : " major";
        const String now = root < 0 ? String("listening...") : String(kNoteNames[root]) + mode;
        auto shifted = [&](double st, int& rootOut) {
            if (root < 0) return String("-");
            const int n = (int)std::lround(st);
            rootOut = (((root + n) % 12) + 12) % 12;
            String r = String(kNoteNames[rootOut]) + mode;
            const double rem = (st - n) * 100.0;
            if (std::fabs(rem) >= 1.0) r << (rem > 0 ? "  +" : "  ") << (int)std::lround(rem) << " cents";
            return r;
        };
        int dryRoot = -1, revRoot = -1;
        const String after = shifted(ov, dryRoot);
        const String afterRev = shifted(ov + rev, revRoot);
        auto sgn = [](double v) { return String(v >= 0 ? "+" : "") + String(v, 2); };
        const int py = panel(5).getY();
        txt(g, "Key of your sample", 910, py + 28, 220, 22, F(19.f), th.label);
        if (root >= 0)
            txt(g, "confidence " + String((int)std::lround(proc.keys.confidence.load() * 100.f)) + "%", 1050, py + 32, 120, 14, FV(10.f), th.textDim);
        txt(g, now, 910, py + 48, 240, 34, FV(root < 0 ? 17.f : 27.f), th.value);
        txt(g, "After overall " + sgn(ov) + " st", 910, py + 86, 240, 22, F(19.f), th.accent2);
        txt(g, after, 910, py + 106, 240, 28, FV(root < 0 ? 17.f : 22.f), th.accent2);
        if (root >= 0 && std::fabs(rev) > 0.004)
            txt(g, "Reversed layer: " + afterRev + "  (" + sgn(ov + rev) + " st)", 910, py + 134, 300, 14, FV(10.5f), th.textDim);

        const int bx = 1170, base = py + 112, bw = 15, gap = 3, maxH = 78;
        for (int i = 0; i < 12; ++i) {
            const float target = proc.keys.chroma[i].load();
            disp[i] += (target - disp[i]) * 0.25f;
            const float h = std::max(2.f, disp[i] * (float)maxH);
            const float x = (float)(bx + i * (bw + gap));
            const bool isRoot = (i == root), isNew = (i == dryRoot);
            const Colour c = isRoot ? th.accent2 : th.accent;
            g.setGradientFill(ColourGradient(c.withAlpha(0.95f), x, (float)base - h, c.withAlpha(0.22f), x, (float)base, false));
            g.fillRoundedRectangle(x, (float)base - h, (float)bw, h, 3.f);
            g.setColour(isNew || isRoot ? th.accent2 : th.textDim);
            g.setFont(FV(10.f));
            g.drawText(kNoteNames[i], (int)x - 3, base + 2, bw + 6, 12, Justification::centred);
            if (isNew && !isRoot) { g.setColour(th.accent2); g.fillRect(x, (float)base + 15.f, (float)bw, 2.f); }
        }
    }

    void paintChopText(Graphics& g) {
        const bool on = proc.get(P_SIGNATURE) >= 0.5f;
        const auto r = panel(8);
        txt(g, on ? "Chopping live - tweak anything while it plays" : "Switch on @Reyaan. Signature Mode to start chopping",
            r.getX() + 330, r.getY() + 5, 1060, 26, F(20.f), on ? th.accent2 : th.textDim, Justification::centredRight);
        txt(g, "Click a step to change it (right-click goes backwards). Editing a step switches Pattern to Custom.",
            984, r.getY() + 96, 330, 60, F(17.f), th.textDim);
        txt(g, "gate = silence    stutter = repeats a slice    half = half-time    tape = tape-stop    reverse = plays the last step backwards",
            28, r.getY() + 206, 1370, 24, F(18.f), th.textDim);
    }

    void paintLevel(Graphics& g) {
        const int py = panel(7).getY();
        txt(g, "Output level", 1100, py + 80, 200, 22, F(19.f), th.label);
        const float db = Decibels::gainToDecibels(proc.outLevel.load(), -60.f);
        const float pos = jlimit(0.f, 1.f, (db + 60.f) / 60.f);
        Rectangle<float> bar(1100.f, (float)py + 108.f, 300.f, 10.f);
        g.setColour(Colours::white.withAlpha(0.10f));
        g.fillRoundedRectangle(bar, 5.f);
        g.setGradientFill(ColourGradient(th.accent, bar.getX(), 0.f, th.accent2, bar.getRight(), 0.f, false));
        g.fillRoundedRectangle(bar.withWidth(std::max(4.f, bar.getWidth() * pos)), 5.f);
        txt(g, String(db, 1) + " dB", 1100, py + 122, 120, 16, FV(12.f), th.textDim);
    }

    ApprehensionProcessor& proc;
    const Theme& th;
    Image logo;
    float disp[12];
    float sigS = 0.f;
};

// ----------------------------------------------------------------- main view (fixed design size)
class MainView final : public Component, private Timer {
public:
    explicit MainView(ApprehensionProcessor& p)
        : proc(p), look(live), backdrop(p, live), overlay(p, live), graph(p, live), grid(p, live) {
        setSize(kW, kH);
        setLookAndFeel(&look);

        // restore the theme this project was saved with
        const String saved = proc.apvts.state.getProperty("theme", "").toString();
        themeList = themes::loadAll();
        if (saved.isNotEmpty())
            for (auto& t : themeList) if (t.name == saved) { theme = t; break; }
        live = theme;
        chooseFont(theme.font);

        backdrop.setBounds(0, 0, kW, kH);
        overlay.setBounds(0, 0, kW, kH);
        addAndMakeVisible(backdrop);
        addAndMakeVisible(overlay);
        graph.setBounds(566, 614, 304, 118);
        addAndMakeVisible(graph);
        grid.setBounds(28, 892, 1376, 50);
        addAndMakeVisible(grid);

        // Time
        addToggle("sync", P_SYNC, "Sync", 32, 140, 80, 34);
        addToggle("align", P_TIMEALIGN, "Time Align", 32, 182, 80, 34);
        addKnob(P_DIVISION, "Division", 116, 134);
        addKnob(P_TIME, "Time", 204, 134);
        addKnob(P_ATTACK, "Attack", 292, 134);
        addKnob(P_CUTOFF, "Cutoff", 380, 134);
        addKnob(P_RELEASE, "Release", 468, 134);
        addKnob(P_SMOOTH, "Smooth", 556, 134);
        // Pitch
        addKnob(P_GPITCH, "Overall Pitch", 904, 130, 116, 124);
        auto& kp = addKnob(P_PITCH, "Reverse Pitch", 1024, 130, 116, 124);
        kp.s.setVelocityBasedMode(true);
        kp.s.setVelocityModeParameters(1.0, 1, 0.0, false);
        addKnob(P_REFHZ, "Ref Hz", 1148, 134);
        // Space & modulation
        addKnob(P_FEEDBACK, "Feedback", 28, 294);
        addKnob(P_DIFFUSE, "Diffusion", 116, 294);
        addKnob(P_WIDTH, "Width", 204, 294);
        addKnob(P_CHORUS, "Chorus", 292, 294);
        addKnob(P_CHORUSMODE, "Chorus Type", 380, 294);
        addKnob(P_FLANGUS, "Flangus", 468, 294);
        addKnob(P_FLANGUSMODE, "Flangus Type", 556, 294);
        // Ducking
        addKnob(P_DUCKAMT, "Amount", 904, 294);
        addKnob(P_DUCKREL, "Release", 992, 294);
        addKnob(P_DUCKDIV, "Division", 1080, 294);
        addToggle("pump", P_DUCKMODE, "Pump", 1180, 304, 160, 36);
        // Reverb
        addKnob(P_RVTYPE, "Type", 28, 454);
        addKnob(P_RVMIX, "Mix", 116, 454);
        addKnob(P_RVSIZE, "Size", 204, 454);
        addKnob(P_RVDECAY, "Decay", 292, 454);
        addKnob(P_RVPRE, "Pre-Delay", 380, 454);
        addKnob(P_RVDAMP, "Damping", 468, 454);
        addKnob(P_RVWIDTH, "Width", 556, 454);
        addKnob(P_RVMOD, "Modulation", 644, 454);
        // Filter
        addKnob(P_LOWCUT, "Low Cut", 28, 614);
        addKnob(P_LOWSLOPE, "Low Shape", 116, 614);
        addKnob(P_LOWRES, "Low Reso", 204, 614);
        addKnob(P_HIGHCUT, "High Cut", 292, 614);
        addKnob(P_HIGHSLOPE, "High Shape", 380, 614);
        addKnob(P_HIGHRES, "High Reso", 468, 614);
        // Output
        addKnob(P_MIX, "Mix", 904, 614);
        addKnob(P_OUTPUT, "Output", 992, 614);
        addToggle("soft", P_SOFTCLIP, "Soft Clip", 1100, 624, 200, 40);
        // Signature chop
        addKnob(P_CHOPINTENSITY, "Intensity", 28, 774);
        addKnob(P_CHOPMIX, "Chop Mix", 116, 774);
        addKnob(P_CHOPSEED, "Seed", 204, 774);
        addKnob(P_CHOPRATE, "Rate", 292, 774);
        addKnob(P_CHOPPATTERN, "Pattern", 380, 774);
        addKnob(P_CHOPSTUTTER, "Stutter Size", 468, 774);
        addToggle("c", P_CGATE, "Gate", 600, 792, 120, 36);
        addToggle("c", P_CSTUT, "Stutter", 728, 792, 120, 36);
        addToggle("c", P_CHALF, "Half-time", 856, 792, 120, 36);
        addToggle("c", P_CTAPE, "Tape-stop", 600, 838, 120, 36);
        addToggle("c", P_CREV, "Reverse", 728, 838, 120, 36);

        addButton(bRoll, "Roll seed", 856, 838, 120, 36);
        bRoll.setTooltip(kTipRoll);
        bRoll.onClick = [this] {
            if (auto* sp = proc.apvts.getParameter(kIds[P_CHOPSEED])) sp->setValueNotifyingHost(Random::getSystemRandom().nextFloat());
        };
        addButton(bClear, "Clear steps", 984, 792, 140, 36);
        bClear.setTooltip(kTipClear);
        bClear.onClick = [this] { for (int i = 0; i < 16; ++i) proc.setChopCell(i, 'N'); proc.setChopMode(9); grid.repaint(); };
        grid.setTooltip(kTipGrid);
        graph.setInterceptsMouseClicks(true, false);
        graph.setTooltip(kTipGraph);

        // header: presets, mode buttons, theme, manual
        addButton(bPrev, "<", 560, 8, 40, 36);
        addButton(bPresetName, "Init", 604, 8, 340, 36);
        addButton(bNext, ">", 948, 8, 40, 36);
        bPrev.setTooltip(kTipPreset); bNext.setTooltip(kTipPreset);
        bPrev.onClick = [this] { stepPreset(-1); };
        bNext.onClick = [this] { stepPreset(1); };
        bPresetName.onClick = [this] { showPresetMenu(); };
        addToggle("sig", P_SIGNATURE, "@Reyaan. Signature Mode", 560, 50, 350, 44);
        addToggle("freeze", P_FREEZE, "Freeze", 926, 50, 150, 44);
        addButton(bTheme, "Theme", 1344, 8, 80, 28);
        bTheme.setTooltip(kTipTheme);
        bTheme.onClick = [this] { showThemeMenu(); };
        addButton(bManual, "Manual", 1344, 44, 80, 28);
        bManual.setTooltip(kTipManual);
        bManual.onClick = [this] {
            auto f = File::getSpecialLocation(File::tempDirectory).getChildFile("Apprehension Manual.html");
            if (f.replaceWithData(BinaryData::manual_html, (size_t)BinaryData::manual_htmlSize)) f.startAsProcess();
        };
        updatePresetLabel(true);

        refreshTheme();
        startTimerHz(30);
    }

    ~MainView() override { stopTimer(); setLookAndFeel(nullptr); }

private:
    using SA = AudioProcessorValueTreeState::SliderAttachment;
    using BA = AudioProcessorValueTreeState::ButtonAttachment;

    struct Knob {
        Slider s;
        Label l;
        std::unique_ptr<SA> a;   // declared last so it is destroyed first
    };
    struct Toggle {
        ToggleButton b;
        std::unique_ptr<BA> a;
    };

    Knob& addKnob(Pid id, const char* name, int x, int y, int w = 88, int h = 114) {
        auto k = std::make_unique<Knob>();
        k->s.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
        k->s.setTextBoxStyle(Slider::TextBoxBelow, false, 88, 18);
        k->s.setRotaryParameters(MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, true);
        k->s.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
        k->s.setColour(Slider::textBoxBackgroundColourId, Colours::transparentBlack);
        k->a = std::make_unique<SA>(proc.apvts, kIds[id], k->s);
        k->l.setText(name, dontSendNotification);
        k->l.setJustificationType(Justification::centred);
        k->l.setInterceptsMouseClicks(false, false);
        k->s.setTooltip(kTips[id]);
        k->s.setBounds(x, y, w, h - 16);
        k->l.setBounds(x, y + h - 18, w, 20);
        addAndMakeVisible(k->s);
        addAndMakeVisible(k->l);
        knobs.push_back(std::move(k));
        return *knobs.back();
    }

    void addToggle(const char* name, Pid id, const char* text, int x, int y, int w, int h) {
        auto t = std::make_unique<Toggle>();
        t->b.setName(name);
        t->b.setButtonText(text);
        t->b.setClickingTogglesState(true);
        t->b.setTooltip(kTips[id]);
        t->b.setBounds(x, y, w, h);
        addAndMakeVisible(t->b);
        t->a = std::make_unique<BA>(proc.apvts, kIds[id], t->b);
        toggles.push_back(std::move(t));
    }

    void addButton(TextButton& b, const char* text, int x, int y, int w, int h) {
        b.setButtonText(text);
        b.setBounds(x, y, w, h);
        addAndMakeVisible(b);
    }

    // ---- live theme: the chosen theme, blended toward the hot-pink Signature look while that mode is on
    void timerCallback() override {
        const float target = proc.get(P_SIGNATURE) >= 0.5f ? 1.f : 0.f;
        sigAmt += (target - sigAmt) * 0.12f;
        if (std::fabs(sigAmt - target) < 0.003f) sigAmt = target;
        live = themes::blend(theme, themes::signatureTheme(), sigAmt);
        updatePresetLabel(false);
        if (std::fabs(sigAmt - lastLabelSig) > 0.02f || (sigAmt == target && lastLabelSig != target)) {
            lastLabelSig = sigAmt;
            updateKnobColours();
        }
    }

    void updateKnobColours() {
        for (auto& k : knobs) {
            k->l.setColour(Label::textColourId, live.label);
            k->s.setColour(Slider::textBoxTextColourId, live.value);
        }
        bTheme.setColour(TextButton::buttonColourId, live.panel.brighter(0.25f));
        bTheme.setColour(TextButton::textColourOffId, live.label);
        for (auto* b : {&bRoll, &bClear, &bPrev, &bNext, &bPresetName, &bManual}) {
            b->setColour(TextButton::buttonColourId, live.panel.brighter(0.25f));
            b->setColour(TextButton::textColourOffId, live.label);
        }
    }

    // ---- presets
    void updatePresetLabel(bool force) {
        const int cur = proc.currentPreset();
        const bool ed = proc.presetEdited();
        if (!force && cur == shownPreset && ed == shownEdited) return;
        shownPreset = cur; shownEdited = ed;
        String name = "Init - default sound";
        if (cur >= 0) name = String(cur + 1).paddedLeft('0', 2) + "  " + presets()[(size_t)cur].name + (ed ? "  (edited)" : "");
        else if (ed) name = "Init  (edited)";
        bPresetName.setButtonText(name);
        bPresetName.setTooltip(cur >= 0 ? String(presets()[(size_t)cur].desc) : String(kTipPreset));
    }

    void loadPreset(int i) {
        proc.applyPreset(i);
        updatePresetLabel(true);
    }

    void stepPreset(int d) {
        const int n = (int)presets().size(), cur = proc.currentPreset();
        loadPreset(cur < 0 ? (d > 0 ? 0 : n - 1) : (cur + d + n) % n);
    }

    void showPresetMenu() {
        PopupMenu m;
        m.addItem(1, "Init - default sound");
        m.addSeparator();
        m.addSectionHeader("Sample flips");
        for (int i = 0; i < (int)presets().size(); ++i)
            m.addItem(100 + i, String(i + 1).paddedLeft('0', 2) + "   " + presets()[(size_t)i].name, true, proc.currentPreset() == i);
        m.showMenuAsync(PopupMenu::Options().withTargetComponent(bPresetName).withMinimumWidth(340),
                        [safe = SafePointer<MainView>(this)](int r) {
                            if (safe == nullptr || r == 0) return;
                            safe->loadPreset(r == 1 ? -1 : r - 100);
                        });
    }

    // ---- themes
    void refreshTheme() {
        chooseFont(theme.font);
        live = themes::blend(theme, themes::signatureTheme(), sigAmt);
        look.refresh();
        for (auto& k : knobs) k->l.setFont(F(18.f));
        updateKnobColours();
        backdrop.themeChanged();
        sendLookAndFeelChange();
        repaint();
    }

    void applyTheme(const Theme& t) {
        theme = t;
        proc.apvts.state.setProperty("theme", t.name == Theme().name ? String() : t.name, nullptr);
        refreshTheme();
    }

    void showThemeMenu() {
        themeList = themes::loadAll();
        PopupMenu m;
        m.addItem(1, "Black & Pink (default)", true, theme.name == Theme().name);
        if (!themeList.empty()) {
            m.addSeparator();
            for (size_t i = 0; i < themeList.size(); ++i)
                m.addItem(100 + (int)i, themeList[i].name, true, theme.name == themeList[i].name);
        }
        m.addSeparator();
        m.addItem(1000, "Open themes folder");
        m.addItem(1001, "Reload themes");
        m.addItem(1002, "Create a theme template");
        m.showMenuAsync(PopupMenu::Options().withTargetComponent(bTheme).withMinimumWidth(260),
                        [safe = SafePointer<MainView>(this)](int r) { if (safe != nullptr) safe->themeChoice(r); });
    }

    void themeChoice(int r) {
        if (r == 0) return;
        if (r == 1) { applyTheme(Theme()); return; }
        if (r >= 100 && r < 1000) {
            const size_t i = (size_t)(r - 100);
            if (i < themeList.size()) applyTheme(themeList[i]);
            return;
        }
        auto dir = themes::folder();
        dir.createDirectory();
        if (r == 1000) { dir.startAsProcess(); return; }
        if (r == 1001) {
            themeList = themes::loadAll();
            for (auto& t : themeList) if (t.name == theme.name) { applyTheme(t); return; }
            return;
        }
        if (r == 1002) {
            auto tpl = dir.getChildFile("My Theme.json");
            if (!tpl.existsAsFile()) tpl.replaceWithText(themes::templateText());
            tpl.revealToUser();
        }
    }

    ApprehensionProcessor& proc;
    Theme theme, live;      // theme = the one picked; live = what is drawn (picked theme blended with the Signature look)
    float sigAmt = 0.f, lastLabelSig = -1.f;
    std::vector<Theme> themeList;
    NeonLook look;
    Backdrop backdrop;
    Overlay overlay;
    FilterGraph graph;
    ChopGrid grid;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<Toggle>> toggles;
    TextButton bTheme, bRoll, bClear, bPrev, bPresetName, bNext, bManual;
    int shownPreset = -2;
    bool shownEdited = false;
    TooltipWindow tooltipWindow { this, 650 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainView)
};

// ----------------------------------------------------------------------------- editor
// Wraps the fixed-size view and scales it, so the window can be dragged to any size that fits your screen.
class ApprehensionEditor final : public AudioProcessorEditor {
public:
    explicit ApprehensionEditor(ApprehensionProcessor& p) : AudioProcessorEditor(&p), view(p) {
        addAndMakeVisible(view);
        setResizable(true, true);
        setResizeLimits(680, 500, 1920, 1410);
        getConstrainer()->setFixedAspectRatio((double)kW / (double)kH);
        setSize((int)(kW * 0.8), (int)(kH * 0.8));
    }

    void resized() override {
        const float s = (float)getWidth() / (float)kW;
        view.setTransform(AffineTransform::scale(s));
        view.setBounds(0, 0, kW, kH);
    }

private:
    MainView view;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ApprehensionEditor)
};

}  // namespace ui

using ApprehensionEditor = ui::ApprehensionEditor;
