// Apprehension - reverse engine (header-only, no host/JUCE dependencies)
#pragma once
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <vector>

namespace apprehension {

constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 2.0 * kPi;

// ---------------------------------------------------------------- tables
constexpr int kNumDivisions = 8;
inline const char* const kDivisionNames[kNumDivisions] = {"1/16", "1/8T", "1/8", "1/4T", "1/4", "1/2", "1 bar", "2 bars"};
inline const double kDivisionBeats[kNumDivisions] = {0.25, 1.0 / 3.0, 0.5, 2.0 / 3.0, 1.0, 2.0, 4.0, 8.0};

constexpr int kNumDuckDivs = 5;
inline const char* const kDuckNames[kNumDuckDivs] = {"1/16", "1/8", "1/4", "1/2", "1/1"};
inline const double kDuckBeats[kNumDuckDivs] = {0.25, 0.5, 1.0, 2.0, 4.0};

constexpr int kNumSlopes = 9;
inline const char* const kSlopeNames[kNumSlopes] = {"6 dB", "12 dB", "18 dB", "24 dB", "36 dB", "48 dB", "72 dB", "96 dB", "Brickwall"};
inline const int kSlopeOrders[kNumSlopes] = {1, 2, 3, 4, 6, 8, 12, 16, 32};   // Butterworth order (6 dB/oct per order)

constexpr int kNumChorusModes = 5;
inline const char* const kChorusNames[kNumChorusModes] = {"Classic", "Ensemble", "Dimension", "Vibrato", "Tape"};
struct ChorusMode { int voices; double base[4], depth[4], rate[4], phase[4]; double stereo, level; bool wetOnly; };
inline const ChorusMode kChorusModes[kNumChorusModes] = {
    {2, {17, 23, 0, 0}, {3.0, 3.0, 0, 0}, {0.70, 0.93, 0, 0}, {0.0, 0.15, 0, 0}, 0.25, 1.0, false},        // Classic
    {3, {14, 19, 25, 0}, {2.6, 3.2, 3.8, 0}, {0.45, 0.58, 0.71, 0}, {0.0, 0.33, 0.66, 0}, 0.20, 0.9, false}, // Ensemble
    {4, {8, 10, 12, 14}, {0.9, 0.9, 0.9, 0.9}, {0.25, 0.31, 0.37, 0.43}, {0, 0.25, 0.5, 0.75}, 0.50, 0.8, false}, // Dimension
    {1, {6, 0, 0, 0}, {2.2, 0, 0, 0}, {5.2, 0, 0, 0}, {0, 0, 0, 0}, 0.15, 1.0, true},                       // Vibrato (wet only)
    {2, {9, 12, 0, 0}, {1.4, 0.7, 0, 0}, {0.35, 6.2, 0, 0}, {0.0, 0.4, 0, 0}, 0.30, 1.0, false}};            // Tape wow & flutter

constexpr int kNumFlangerModes = 5;
inline const char* const kFlangerNames[kNumFlangerModes] = {"Fruity", "Jet", "Metallic", "Soft", "Wide"};
struct FlangerMode { double minMs, maxMs, rate, fb, stereo, level; };
inline const FlangerMode kFlangerModes[kNumFlangerModes] = {
    {0.6, 4.6, 0.27, 0.55, 0.30, 1.0},    // Fruity: the classic sweep
    {0.15, 8.0, 0.12, 0.78, 0.20, 1.0},   // Jet: deep, slow, strong resonance
    {0.3, 3.0, 0.40, -0.70, 0.25, 1.0},   // Metallic: negative feedback
    {0.8, 2.8, 0.20, 0.25, 0.30, 0.8},    // Soft: gentle
    {0.5, 6.0, 0.18, 0.50, 0.50, 1.0}};   // Wide: left/right sweep opposite ways

constexpr int kNumReverbTypes = 5;
inline const char* const kReverbNames[kNumReverbTypes] = {"Room", "Hall", "Plate", "Chamber", "Cathedral"};

constexpr int kNumChopRates = 3;
inline const char* const kChopRateNames[kNumChopRates] = {"1/8", "1/16", "1/32"};
inline const int kChopStepsPerBeat[kNumChopRates] = {2, 4, 8};
constexpr int kNumStutter = 3;
inline const char* const kStutterNames[kNumStutter] = {"1/2", "1/4", "1/8"};
inline const int kStutterDiv[kNumStutter] = {2, 4, 8};
constexpr int kNumChopPatterns = 10;   // Auto, 1..8, Custom
inline const char* const kChopPatternNames[kNumChopPatterns] = {"Auto", "1", "2", "3", "4", "5", "6", "7", "8", "Custom"};

// Auto-chop ("Signature Mode") step patterns, one character per step, 16 steps each.
// N normal   G gate (silence)   S stutter   H half-time   T tape-stop   R reverse slice
inline constexpr char kChop[8][17] = {"NNGNNNGNNNSSNNRR", "NNNNNNNNSSSSHHHH", "NGNGNNSSNGNGTTTT", "NNNNGGNNNNNNSSRR",
                                      "HHHHNNNNHHHHSSSS", "NSSNNGNSSNNGNRRR", "NNNNNNNNNNNNTTTT", "GNNGNNGNNGNNSSSS"};
inline uint32_t hash32(uint32_t x) { x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16; return x; }
inline int chopPatternFor(int64_t bar) { return (int)(hash32((uint32_t)bar * 2654435761u) % 8u); }
inline char chopAction(int pattern, int step) { return kChop[pattern & 7][step & 15]; }

struct Params {
    double mix = 0.5, sync = 1.0, div = 4.0, timeMs = 500.0, attackMs = 0.0, cutoff = 1.0, releaseMs = 30.0, smooth = 0.5;
    double feedback = 0.25, diffuse = 0.2, width = 1.0, outputDb = 0.0;
    double lowCut = 20.0, highCut = 20000.0, lowSlope = 3, highSlope = 3, lowRes = 0.0, highRes = 0.0;
    double chorus = 0.0, chorusMode = 0, flangus = 0.0, flangusMode = 0;
    double pitchSt = 0.0, freeze = 0.0;
    double duckAmt = 0.0, duckRelMs = 200.0, duckMode = 0.0, duckDiv = 2.0;
    double rvType = 1, rvMix = 0.0, rvSize = 0.5, rvDecay = 2.5, rvPreMs = 20.0, rvDamp = 0.4, rvWidth = 1.0, rvMod = 0.2;
    double signature = 0.0, softClip = 0.0;
    double chopPattern = 0, chopRate = 1, chopIntensity = 1.0, chopMix = 1.0, chopSeed = 0, chopStutter = 1;
    double cGate = 1, cStut = 1, cHalf = 1, cTape = 1, cRev = 1;
    double gPitchSt = 0.0;   // overall pitch of the whole output, semitones
    double timeAlign = 0.0;  // >=0.5 = lock reversed chunks to the host beat grid
};

// ---------------------------------------------------------------- filter shapes
inline int slopeOrder(double idx) { return kSlopeOrders[std::min(std::max((int)std::lround(idx), 0), kNumSlopes - 1)]; }

inline double cutMag2(double f, double fc, int order, double resDb, bool hp) {
    const double x = f / fc;
    double m2 = 1.0;
    const int n = order / 2;
    for (int k = 0; k < n; ++k) {
        double q = 1.0 / (2.0 * std::sin((2 * k + 1) * kPi / (2.0 * order)));
        if (k == 0 && resDb > 0.01) q *= std::pow(10.0, resDb / 20.0);
        const double den = (1 - x * x) * (1 - x * x) + (x / q) * (x / q);
        m2 *= (hp ? x * x * x * x : 1.0) / den;
    }
    if (order % 2) m2 *= hp ? x * x / (1 + x * x) : 1.0 / (1 + x * x);
    return m2;
}

// Magnitude (dB) of the Low Cut / High Cut pair with their slopes and resonances. Used by the UI graph.
inline double cutResponseDb(double f, double lowCut, double lowSlope, double lowRes, double highCut, double highSlope, double highRes) {
    double m2 = 1.0;
    if (lowCut > 20.5)     m2 *= cutMag2(f, lowCut, slopeOrder(lowSlope), lowRes, true);
    if (highCut < 19999.5) m2 *= cutMag2(f, highCut, slopeOrder(highSlope), highRes, false);
    return 10.0 * std::log10(std::max(m2, 1e-12));
}

inline float softClipSample(float x) {   // transparent below 0.7, smoothly rounds off to a 0 dBFS ceiling
    const float a = std::fabs(x);
    if (a <= 0.7f) return x;
    const float y = 0.7f + 0.3f * std::tanh((a - 0.7f) / 0.3f);
    return x < 0.f ? -y : y;
}

struct Biquad {
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void setLowpass(double fs, double f, double q = 0.70710678) { design(fs, f, q, false); }
    void setHighpass(double fs, double f, double q = 0.70710678) { design(fs, f, q, true); }
    void reset() { z1 = z2 = 0; }
    float process(float xf) {
        double x = xf, y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return (float)y;
    }
    void design(double fs, double f, double q, bool hp) {
        f = std::min(std::max(f, 10.0), fs * 0.45);
        double w = kTwoPi * f / fs, c = std::cos(w), al = std::sin(w) / (2.0 * q), a0 = 1.0 + al;
        if (hp) { b0 = (1 + c) / 2; b1 = -(1 + c); b2 = (1 + c) / 2; }
        else    { b0 = (1 - c) / 2; b1 = 1 - c;    b2 = (1 - c) / 2; }
        b0 /= a0; b1 /= a0; b2 /= a0; a1 = -2.0 * c / a0; a2 = (1.0 - al) / a0;
    }
    void design1(double fs, double f, bool hp) {   // first-order (6 dB/oct)
        f = std::min(std::max(f, 10.0), fs * 0.45);
        const double K = std::tan(kPi * f / fs), n = 1.0 / (1.0 + K);
        if (hp) { b0 = n; b1 = -n; } else { b0 = K * n; b1 = K * n; }
        b2 = 0; a1 = (K - 1.0) * n; a2 = 0;
    }
};

// Butterworth cut filter of any order up to 32 (6 dB/oct .. brickwall), with optional resonance at the corner.
struct CutFilter {
    Biquad bq[16], one;
    int nBq = 0;
    bool hasOne = false;
    void design(bool hp, double fs, double fc, int order, double resDb) {
        nBq = order / 2; hasOne = (order % 2) == 1;
        for (int k = 0; k < nBq; ++k) {
            double q = 1.0 / (2.0 * std::sin((2 * k + 1) * kPi / (2.0 * order)));
            if (k == 0 && resDb > 0.01) q *= std::pow(10.0, resDb / 20.0);
            bq[k].design(fs, fc, q, hp);
        }
        if (hasOne) one.design1(fs, fc, hp);
    }
    void reset() { for (auto& b : bq) b.reset(); one.reset(); }
    float process(float x) {
        if (hasOne) x = one.process(x);
        for (int k = 0; k < nBq; ++k) x = bq[k].process(x);
        return x;
    }
};

struct Allpass {
    std::vector<float> b; int i = 0;
    void init(int n) { b.assign((size_t)std::max(n, 2), 0.f); i = 0; }
    void clear() { std::fill(b.begin(), b.end(), 0.f); i = 0; }
    float process(float x, float g) {
        float d = b[(size_t)i], v = x + g * d;
        b[(size_t)i] = v;
        if (++i >= (int)b.size()) i = 0;
        return d - g * v;
    }
};

struct ModDelay {   // short fractional delay line for chorus / flangus
    std::vector<float> b; int w = 0, mask = 0;
    void init(int n) { int s = 1; while (s < n + 4) s <<= 1; b.assign((size_t)s, 0.f); mask = s - 1; w = 0; }
    void clear() { std::fill(b.begin(), b.end(), 0.f); w = 0; }
    float read(double d) const {
        double rp = (double)w - d;
        int i0 = (int)std::floor(rp);
        float fr = (float)(rp - i0);
        float a = b[(size_t)(i0 & mask)], c = b[(size_t)((i0 + 1) & mask)];
        return a + (c - a) * fr;
    }
    void write(float x) { b[(size_t)w] = x; w = (w + 1) & mask; }
};

struct Smoothed {
    double cur = 0, tgt = 0, coef = 0.001;
    void setTime(double fs, double ms) { coef = 1.0 - std::exp(-1.0 / (fs * ms * 0.001)); }
    void snap(double v) { cur = tgt = v; }
    double next() { cur += (tgt - cur) * coef; return cur; }
};

// ---------------------------------------------------------------- reverb (8-line feedback delay network)
class Reverb {
public:
    static constexpr int N = 8;

    void prepare(double sr) {
        sr_ = sr;
        size_t n = 1; while (n < (size_t)(sr * 0.8) + 8) n <<= 1;
        for (auto& l : line_) l.assign(n, 0.f);
        mask_ = (int64_t)n - 1;
        size_t pn = 1; while (pn < (size_t)(sr * 0.3) + 8) pn <<= 1;
        for (auto& p : pd_) p.assign(pn, 0.f);
        pmask_ = (int64_t)pn - 1;
        static const int apl[3] = {142, 107, 379};
        for (int c = 0; c < 2; ++c) for (int k = 0; k < 3; ++k) ap_[c][k].init((int)((apl[k] + c * 23) * sr / 44100.0));
        coef_ = 1.0 - std::exp(-1.0 / (sr * 0.15));
        clear();
    }

    void clear() {
        for (auto& l : line_) std::fill(l.begin(), l.end(), 0.f);
        for (auto& p : pd_) std::fill(p.begin(), p.end(), 0.f);
        for (auto& c : ap_) for (auto& a : c) a.clear();
        for (int i = 0; i < N; ++i) { lp_[i] = 0.f; ph_[i] = 0.0; }
        w_ = 0; pw_ = 0; snap_ = true;
    }

    void setParams(int type, double size01, double decaySec, double preMs, double damp01, double width, double mod01) {
        static const double base[N] = {29.7, 37.1, 41.1, 43.7, 53.3, 61.7, 71.3, 79.9};
        static const double tscale[kNumReverbTypes] = {0.55, 1.0, 0.72, 0.85, 1.8};
        static const double tap[kNumReverbTypes] = {0.45, 0.60, 0.72, 0.50, 0.62};
        static const double tdamp[kNumReverbTypes] = {0.10, 0.0, -0.12, 0.05, 0.10};
        type = std::min(std::max(type, 0), kNumReverbTypes - 1);
        const double mul = tscale[type] * (0.5 + 1.5 * std::min(std::max(size01, 0.0), 1.0));
        const double rt = std::max(decaySec, 0.1);
        for (int i = 0; i < N; ++i) {
            double len = base[i] * mul * 0.001 * sr_;
            len = std::min(std::max(len, 16.0), (double)(mask_ - 800));
            lenTgt_[i] = len;
            gain_[i] = (float)std::pow(10.0, -3.0 * (len / sr_) / rt);
        }
        damp_ = (float)std::min(std::max(0.03 + 0.92 * damp01 + tdamp[type], 0.0), 0.97);
        preTgt_ = std::min(std::max(preMs, 0.0), 250.0) * 0.001 * sr_;
        width_ = (float)width;
        modDepth_ = mod01 * 0.0016 * sr_;
        apg_ = (float)tap[type];
        if (snap_) { for (int i = 0; i < N; ++i) lenSm_[i] = lenTgt_[i]; preSm_ = preTgt_; snap_ = false; }
    }

    void process(float inL, float inR, float& oL, float& oR) {
        pd_[0][(size_t)(pw_ & pmask_)] = inL; pd_[1][(size_t)(pw_ & pmask_)] = inR;
        preSm_ += (preTgt_ - preSm_) * coef_;
        const int pr = (int)preSm_;
        float xl = pd_[0][(size_t)((pw_ - pr) & pmask_)], xr = pd_[1][(size_t)((pw_ - pr) & pmask_)];
        ++pw_;
        for (int k = 0; k < 3; ++k) { xl = ap_[0][k].process(xl, apg_); xr = ap_[1][k].process(xr, apg_); }

        float v[N];
        for (int i = 0; i < N; ++i) {
            lenSm_[i] += (lenTgt_[i] - lenSm_[i]) * coef_;
            double len = lenSm_[i];
            if (i < 4) len += modDepth_ * std::sin(kTwoPi * ph_[i]);
            const double rp = (double)w_ - len;
            const int64_t i0 = (int64_t)std::floor(rp);
            const float fr = (float)(rp - (double)i0);
            const float a = line_[i][(size_t)(i0 & mask_)], b = line_[i][(size_t)((i0 + 1) & mask_)];
            const float d = a + (b - a) * fr;
            lp_[i] = d * (1.f - damp_) + lp_[i] * damp_;
            v[i] = lp_[i] * gain_[i];
        }
        for (int i = 0; i < 4; ++i) { ph_[i] += (0.23 + 0.11 * i) / sr_; if (ph_[i] >= 1.0) ph_[i] -= 1.0; }
        for (int h = 1; h < N; h *= 2)
            for (int i = 0; i < N; i += 2 * h)
                for (int j = i; j < i + h; ++j) { const float a = v[j], b = v[j + h]; v[j] = a + b; v[j + h] = a - b; }
        for (int i = 0; i < N; ++i)
            line_[i][(size_t)(w_ & mask_)] = v[i] * 0.35355339f + ((i & 1) ? xr : xl) * 0.5f;
        ++w_;

        float l = (lp_[0] - lp_[2] + lp_[4] - lp_[6]) * kOut, r = (lp_[1] - lp_[3] + lp_[5] - lp_[7]) * kOut;
        const float m = 0.5f * (l + r), s = 0.5f * (l - r) * width_;
        oL = m + s; oR = m - s;
    }

private:
    static constexpr float kOut = 0.5f;
    double sr_ = 44100.0, lenTgt_[N] = {}, lenSm_[N] = {}, preTgt_ = 0, preSm_ = 0, modDepth_ = 0, coef_ = 0.001, ph_[N] = {};
    std::vector<float> line_[N], pd_[2];
    int64_t mask_ = 0, w_ = 0, pmask_ = 0, pw_ = 0;
    Allpass ap_[2][3];
    float gain_[N] = {}, lp_[N] = {}, damp_ = 0.4f, width_ = 1.f, apg_ = 0.6f;
    bool snap_ = true;
};

// ---------------------------------------------------------------- overall pitch shifter (two-tap granular, constant latency)
class PitchShifter {
public:
    void prepare(double sr) {
        sr_ = sr;
        W_ = std::max(64, (int)(0.050 * sr)) & ~1;
        size_t n = 1; while (n < (size_t)W_ * 2 + 16) n <<= 1;
        for (auto& b : buf_) b.assign(n, 0.f);
        mask_ = (int64_t)n - 1;
        coef_ = 1.0 - std::exp(-1.0 / (sr * 0.03));
        clear();
    }
    void clear() { for (auto& b : buf_) std::fill(b.begin(), b.end(), 0.f); w_ = 0; ph_ = 0.0; active_ = 0.0; }
    // The signal always passes through a fixed delay of half a window, so the plugin has constant latency
    // (reported to the host for automatic delay compensation) whether or not the pitch is changed.
    int latencySamples() const { return W_ / 2; }

    void process(float inL, float inR, double ratio, float& oL, float& oR) {
        buf_[0][(size_t)(w_ & mask_)] = inL;
        buf_[1][(size_t)(w_ & mask_)] = inR;
        const bool want = std::fabs(ratio - 1.0) > 1e-4;
        active_ += ((want ? 1.0 : 0.0) - active_) * coef_;
        const int half = W_ / 2;
        const float bL = buf_[0][(size_t)((w_ - half) & mask_)], bR = buf_[1][(size_t)((w_ - half) & mask_)];
        if (!want && active_ < 1e-4) { ph_ = 0.0; active_ = 0.0; oL = bL; oR = bR; ++w_; return; }
        const double span = (double)(W_ - 4);
        ph_ += (1.0 - ratio) / span;
        ph_ -= std::floor(ph_);
        double p2 = ph_ + 0.5; p2 -= std::floor(p2);
        const double s1 = std::sin(kPi * ph_), s2 = std::sin(kPi * p2);
        const float g1 = (float)(s1 * s1), g2 = (float)(s2 * s2);   // g1 + g2 = 1
        const double d1 = 2.0 + ph_ * span, d2 = 2.0 + p2 * span;
        const float sL = g1 * tap(0, d1) + g2 * tap(0, d2), sR = g1 * tap(1, d1) + g2 * tap(1, d2);
        oL = bL + (sL - bL) * (float)active_;
        oR = bR + (sR - bR) * (float)active_;
        ++w_;
    }

private:
    float tap(int c, double d) const {
        const double pos = (double)w_ - d;
        const int64_t i0 = (int64_t)std::floor(pos);
        const float t = (float)(pos - (double)i0);
        const float* b = buf_[c].data();
        const float xm1 = b[(size_t)((i0 - 1) & mask_)], x0 = b[(size_t)(i0 & mask_)], x1 = b[(size_t)((i0 + 1) & mask_)], x2 = b[(size_t)((i0 + 2) & mask_)];
        const float c1 = 0.5f * (x1 - xm1), c2 = xm1 - 2.5f * x0 + 2.f * x1 - 0.5f * x2, c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
        return ((c3 * t + c2) * t + c1) * t + x0;
    }
    double sr_ = 44100.0, ph_ = 0.0, active_ = 0.0, coef_ = 0.001;
    int W_ = 2205;
    std::vector<float> buf_[2];
    int64_t mask_ = 0, w_ = 0;
};

// ---------------------------------------------------------------- main engine
class Engine {
public:
    static constexpr double kMaxSeconds = 10.0;    // longest reverse chunk
    static constexpr double kBufferSeconds = 20.0; // history kept (room for pitch-up reads)

    std::atomic<int> uiStep{-1};      // current step (0-15) while Signature Mode is on, for the UI
    std::atomic<int> uiPattern{0};    // pattern in use: 0-7 built-in, 8 custom
    std::atomic<int> uiAct{'N'};      // action actually applied on the current step
    std::atomic<char> customPat[16];  // user-editable pattern

    Engine() { for (int i = 0; i < 16; ++i) customPat[i].store(kChop[0][i]); }

    int latencySamples() const { return ps_.latencySamples(); }

    void setCustom(int i, char c) { customPat[i & 15].store(c); }
    char custom(int i) const { return customPat[i & 15].load(); }

    void prepare(double sampleRate) {
        sr_ = sampleRate;
        maxLen_ = (int)(sr_ * kMaxSeconds);
        size_t n = 1;
        while (n < (size_t)(sr_ * kBufferSeconds) + 16) n <<= 1;
        for (auto& b : buf_) b.assign(n, 0.f);
        mask_ = (int64_t)n - 1;
        size_t cn = 1;
        while (cn < (size_t)(sr_ * 4.0)) cn <<= 1;
        for (auto& b : cb_) b.assign(cn, 0.f);
        cmask_ = (int64_t)cn - 1;
        static const int dl[4] = {113, 167, 223, 331}, dr[4] = {131, 181, 241, 347};
        for (int k = 0; k < 4; ++k) {
            ap_[0][k].init((int)(dl[k] * sr_ / 44100.0));
            ap_[1][k].init((int)(dr[k] * sr_ / 44100.0));
        }
        for (int c = 0; c < 2; ++c) {
            for (auto& d : ch_[c]) d.init((int)(0.040 * sr_));
            fl_[c].init((int)(0.012 * sr_));
        }
        rv_.prepare(sr_);
        ps_.prepare(sr_);
        gp_.setTime(sr_, 20.0);
        for (auto* s : {&mix_, &fb_, &diff_, &width_, &out_, &duckAmt_, &chorus_, &flangus_, &rvMix_}) s->setTime(sr_, 20.0);
        spd_.setTime(sr_, 25.0);
        duckAtk_ = 1.0 - std::exp(-1.0 / (sr_ * 0.002));
        envAtk_ = std::exp(-1.0 / (sr_ * 0.003));
        lastLow_ = lastHigh_ = -1;
        reset();
    }

    void reset() {
        for (auto& b : buf_) std::fill(b.begin(), b.end(), 0.f);
        for (auto& b : cb_) std::fill(b.begin(), b.end(), 0.f);
        for (auto& g : grains_) g.active = false;
        for (auto& c : ap_) for (auto& a : c) a.clear();
        for (auto& f : hpf_) f.reset();
        for (auto& f : lpf_) f.reset();
        for (auto& c : ch_) for (auto& d : c) d.clear();
        for (auto& d : fl_) d.clear();
        rv_.clear();
        ps_.clear();
        alignIdx_ = INT64_MIN;
        wpos_ = 0; hop_ = 0; first_ = true; env_ = 0; dg_ = 1.0; beatPos_ = 0;
        cw_ = 0; curStep_ = INT64_MIN; action_ = prevAct_ = nextAct_ = 'N'; tStep_ = 0; stepLen_ = 1; slice_ = 1; stepStart_ = 0;
        for (auto& p : chPh_) p = 0.0;
        fgPh_ = 0.0;
        uiStep.store(-1);
    }

    int grainLength(const Params& p, double bpm) const {
        double sec;
        if (p.sync >= 0.5 && bpm > 1.0) {
            int d = std::min(std::max((int)std::lround(p.div), 0), kNumDivisions - 1);
            sec = kDivisionBeats[d] * 60.0 / bpm;
        } else {
            sec = p.timeMs * 0.001;
        }
        return std::min(std::max((int)std::lround(sec * sr_), 64), maxLen_);
    }

    // Which chop action runs on this step, honouring pattern choice, seed, intensity and the allowed-action switches.
    char pickAction(const Params& p, int64_t stepIdx, int& patOut) const {
        const int s = (int)(((stepIdx % 16) + 16) % 16);
        const int64_t bar = (stepIdx >= 0) ? stepIdx / 16 : -((-stepIdx + 15) / 16);
        const int mode = std::min(std::max((int)std::lround(p.chopPattern), 0), kNumChopPatterns - 1);
        const int seed = (int)std::lround(p.chopSeed);
        char a;
        if (mode == 9) { a = customPat[s].load(); patOut = 8; }
        else {
            patOut = mode == 0 ? chopPatternFor(bar + (int64_t)seed * 131) : mode - 1;
            a = chopAction(patOut, s);
        }
        const double u = (double)(hash32((uint32_t)stepIdx * 2654435761u ^ ((uint32_t)seed * 40503u + 977u)) % 1000u) / 1000.0;
        if (u >= p.chopIntensity) return 'N';
        switch (a) {
            case 'G': return p.cGate >= 0.5 ? 'G' : 'N';
            case 'S': return p.cStut >= 0.5 ? 'S' : 'N';
            case 'H': return p.cHalf >= 0.5 ? 'H' : 'N';
            case 'T': return p.cTape >= 0.5 ? 'T' : 'N';
            case 'R': return p.cRev >= 0.5 ? 'R' : 'N';
            default: return 'N';
        }
    }

    // ppq < 0 means "no transport position available" (free-running beat clock).
    // Returns the block's output peak (for metering).
    float process(const float* inL, const float* inR, float* outL, float* outR, int n,
                  const Params& p, double bpm, double ppq) {
        const double spTarget = std::pow(2.0, std::min(std::max(p.pitchSt, -24.0), 24.0) / 12.0);
        mix_.tgt = p.mix; fb_.tgt = p.feedback; diff_.tgt = p.diffuse; width_.tgt = p.width;
        out_.tgt = std::pow(10.0, p.outputDb / 20.0); duckAmt_.tgt = p.duckAmt; spd_.tgt = spTarget;
        chorus_.tgt = p.chorus; flangus_.tgt = p.flangus; rvMix_.tgt = p.rvMix;
        gp_.tgt = std::pow(2.0, std::min(std::max(p.gPitchSt, -12.0), 12.0) / 12.0);
        if (first_) {
            for (auto* s : {&mix_, &fb_, &diff_, &width_, &out_, &duckAmt_, &spd_, &chorus_, &flangus_, &rvMix_, &gp_}) s->snap(s->tgt);
            first_ = false;
        }
        const int loSlope = std::min(std::max((int)std::lround(p.lowSlope), 0), kNumSlopes - 1);
        const int hiSlope = std::min(std::max((int)std::lround(p.highSlope), 0), kNumSlopes - 1);
        if (p.lowCut != lastLow_ || p.highCut != lastHigh_ || loSlope != lastLoS_ || hiSlope != lastHiS_ ||
            p.lowRes != lastLoR_ || p.highRes != lastHiR_) {
            for (auto& f : hpf_) f.design(true, sr_, p.lowCut, kSlopeOrders[loSlope], p.lowRes);
            for (auto& f : lpf_) f.design(false, sr_, p.highCut, kSlopeOrders[hiSlope], p.highRes);
            if (loSlope != lastLoS_) for (auto& f : hpf_) f.reset();
            if (hiSlope != lastHiS_) for (auto& f : lpf_) f.reset();
            lastLow_ = p.lowCut; lastHigh_ = p.highCut; lastLoS_ = loSlope; lastHiS_ = hiSlope; lastLoR_ = p.lowRes; lastHiR_ = p.highRes;
            hpOn_ = p.lowCut > 20.5;       // minimum = filter off
            lpOn_ = p.highCut < 19999.5;   // maximum = filter off
        }
        rv_.setParams((int)std::lround(p.rvType), p.rvSize, p.rvDecay, p.rvPreMs, p.rvDamp, p.rvWidth, p.rvMod);
        const ChorusMode& cm = kChorusModes[std::min(std::max((int)std::lround(p.chorusMode), 0), kNumChorusModes - 1)];
        const FlangerMode& fm = kFlangerModes[std::min(std::max((int)std::lround(p.flangusMode), 0), kNumFlangerModes - 1)];

        int L = grainLength(p, bpm);
        L = std::max(std::min(L, (int)((double)(mask_ - 64) / std::max(1.0, spTarget))), 64);
        const int attSamples = (int)(p.attackMs * 0.001 * sr_);
        const int relSamples = std::max(1, (int)(p.releaseMs * 0.001 * sr_));
        const double cutFrac = std::min(std::max(p.cutoff, 0.05), 1.0);
        const double fadeFrac = 0.02 + 0.48 * std::min(std::max(p.smooth, 0.0), 1.0);
        const bool frozen = p.freeze >= 0.5;
        const bool chop = p.signature >= 0.5;
        const int rateIdx = std::min(std::max((int)std::lround(p.chopRate), 0), kNumChopRates - 1);
        const int spb = kChopStepsPerBeat[rateIdx];
        const int stutDiv = kStutterDiv[std::min(std::max((int)std::lround(p.chopStutter), 0), kNumStutter - 1)];
        const double chopMix = std::min(std::max(p.chopMix, 0.0), 1.0);
        const double bps = (bpm > 1.0 ? bpm : 120.0) / 60.0, inc = bps / sr_;
        if (ppq >= 0.0) beatPos_ = ppq;
        const double duckBeats = kDuckBeats[std::min(std::max((int)std::lround(p.duckDiv), 0), kNumDuckDivs - 1)];
        const double relFrac = std::min(std::max(p.duckRelMs / (duckBeats * 1000.0 / bps), 0.02), 1.0);
        const double relCoef = std::exp(-1.0 / (sr_ * std::max(p.duckRelMs, 1.0) * 0.001));
        const bool align = p.timeAlign >= 0.5;
        const double Lbeats = (double)L / sr_ * bps;
        if (!align) alignIdx_ = INT64_MIN;
        if (!chop) { curStep_ = INT64_MIN; action_ = prevAct_ = nextAct_ = 'N'; uiStep.store(-1); }
        float peak = 0.f;

        for (int i = 0; i < n; ++i) {
            double mix = mix_.next(), fb = fb_.next(), diff = diff_.next(), width = width_.next();
            double og = out_.next(), duckAmt = duckAmt_.next(), spd = spd_.next();
            const double chAmt = chorus_.next(), flAmt = flangus_.next(), rvAmt = rvMix_.next(), gRatio = gp_.next();
            beatPos_ += inc;

            if (align) {   // Time Align: chunk boundaries sit exactly on the host's beat grid
                const int64_t gi = (int64_t)std::floor(beatPos_ / Lbeats);
                if (alignIdx_ == INT64_MIN) alignIdx_ = gi;
                else if (gi != alignIdx_) { alignIdx_ = gi; spawnAligned(L, fadeFrac, attSamples, cutFrac); }
            } else {
                if (hop_ <= 0) spawn(L, fadeFrac, attSamples, cutFrac);
                --hop_;
            }

            float wet[2] = {0.f, 0.f};
            for (auto& g : grains_) {
                if (!g.active) continue;
                float w = windowAt(g);
                if (g.att > 0 && g.t < g.att) { float e = (float)g.t / (float)g.att; w *= e * e * (3.f - 2.f * e); }
                if (g.t >= g.cutLen) w = 0.f;                                   // cutoff: the sound has ended
                else if (g.cutLen < g.len) {
                    const int rel = std::min(relSamples, g.cutLen);
                    if (g.t > g.cutLen - rel) { float x = (float)(g.cutLen - g.t) / (float)rel; w *= x * x * (3.f - 2.f * x); }
                }
                if (w > 0.f) {
                    const int64_t i0 = (int64_t)std::floor(g.pos);
                    const float fr = (float)(g.pos - (double)i0);
                    for (int c = 0; c < 2; ++c) {
                        const float* b = buf_[c].data();
                        wet[c] += w * hermite(b[(size_t)((i0 - 1) & mask_)], b[(size_t)(i0 & mask_)],
                                              b[(size_t)((i0 + 1) & mask_)], b[(size_t)((i0 + 2) & mask_)], fr);
                    }
                }
                g.pos -= spd;
                if (++g.t >= g.len) g.active = false;
            }

            const float in[2] = {inL[i], inR[i]};
            float fbSig[2];
            for (int v = 0; v < cm.voices; ++v) { chPh_[v] += cm.rate[v] / sr_; if (chPh_[v] >= 1.0) chPh_[v] -= 1.0; }
            fgPh_ += fm.rate / sr_; if (fgPh_ >= 1.0) fgPh_ -= 1.0;
            for (int c = 0; c < 2; ++c) {
                float x = wet[c];
                if (diff > 1e-4) {
                    float y = x;
                    for (auto& a : ap_[c]) y = a.process(y, 0.55f);
                    x = (float)(x * (1.0 - diff) + y * diff);
                }
                if (hpOn_) x = hpf_[c].process(x);
                if (lpOn_) x = lpf_[c].process(x);
                fbSig[c] = (float)std::tanh(fb * x);

                if (chAmt > 1e-3) {   // chorus (mode picks voices / rates / depth)
                    float sum = 0.f;
                    const double depScale = 0.35 + 0.65 * chAmt;
                    for (int v = 0; v < cm.voices; ++v) {
                        const double ph = chPh_[v] + cm.phase[v] + (c ? cm.stereo : 0.0);
                        const double d = (cm.base[v] + cm.depth[v] * depScale * std::sin(kTwoPi * ph)) * 0.001 * sr_;
                        sum += ch_[c][v].read(d);
                        ch_[c][v].write(x);
                    }
                    sum /= (float)cm.voices;
                    if (cm.wetOnly) x = (float)((1.0 - chAmt) * x + chAmt * sum);
                    else x += (float)(chAmt * cm.level) * sum * 1.2f;
                }
                if (flAmt > 1e-3) {   // flangus (mode picks sweep range, speed, feedback polarity)
                    const double lfo = 0.5 + 0.5 * std::sin(kTwoPi * (fgPh_ + (c ? fm.stereo : 0.0)));
                    const double d = (fm.minMs + (fm.maxMs - fm.minMs) * lfo) * 0.001 * sr_;
                    const float dv = fl_[c].read(d);
                    fl_[c].write(x + (float)(fm.fb * flAmt) * dv);
                    x += (float)(flAmt * fm.level) * dv;
                }
                wet[c] = x;
            }

            { // reverb (always runs so its tail keeps ringing even if the wet signal stops)
                float rl, rr;
                rv_.process(wet[0], wet[1], rl, rr);
                wet[0] = wet[0] * (float)(1.0 - 0.35 * rvAmt) + rl * (float)rvAmt;
                wet[1] = wet[1] * (float)(1.0 - 0.35 * rvAmt) + rr * (float)rvAmt;
            }

            if (!frozen) {   // freeze = stop recording; the last chunk keeps looping backwards
                buf_[0][(size_t)(wpos_ & mask_)] = in[0] + fbSig[0];
                buf_[1][(size_t)(wpos_ & mask_)] = in[1] + fbSig[1];
                ++wpos_;
            }

            // ducking (applies to the wet signal only)
            double a = std::fabs(0.5 * ((double)in[0] + (double)in[1]));
            env_ = a > env_ ? envAtk_ * env_ + (1.0 - envAtk_) * a : relCoef * env_ + (1.0 - relCoef) * a;
            double tg = 1.0;
            if (duckAmt > 1e-4) {
                if (p.duckMode < 0.5) {
                    tg = 1.0 - duckAmt * std::min(env_ * 8.0, 1.0);
                } else {
                    double cyc = beatPos_ / duckBeats, ph = cyc - std::floor(cyc);
                    double x = std::min(ph / relFrac, 1.0), s = x * x * (3.0 - 2.0 * x);
                    tg = 1.0 - duckAmt * (1.0 - s);
                }
            }
            dg_ += (tg - dg_) * duckAtk_;

            float m = 0.5f * (wet[0] + wet[1]);
            float s = 0.5f * (wet[0] - wet[1]) * (float)width;
            wet[0] = (m + s) * (float)dg_; wet[1] = (m - s) * (float)dg_;

            double wg = std::sin(mix * kPi * 0.5), dgn = std::cos(mix * kPi * 0.5);
            float oL = (float)((in[0] * dgn + wet[0] * wg) * og);
            float oR = (float)((in[1] * dgn + wet[1] * wg) * og);

            // Signature Mode: auto-chop (gates, stutters, half-time, tape-stops, reverse slices) - fully tweakable
            cb_[0][(size_t)(cw_ & cmask_)] = oL;
            cb_[1][(size_t)(cw_ & cmask_)] = oR;
            if (chop) {
                const float origL = oL, origR = oR;
                const int64_t stepIdx = (int64_t)std::floor(beatPos_ * spb);
                if (stepIdx != curStep_) {
                    curStep_ = stepIdx;
                    int pat = 0, dummy = 0;
                    prevAct_ = action_;
                    action_ = pickAction(p, stepIdx, pat);
                    nextAct_ = pickAction(p, stepIdx + 1, dummy);
                    tStep_ = 0; stepStart_ = cw_;
                    stepLen_ = std::max(64, (int)(sr_ / (bps * spb)));
                    slice_ = std::max(32, stepLen_ / stutDiv);
                    uiStep.store((int)(((stepIdx % 16) + 16) % 16)); uiPattern.store(pat); uiAct.store((int)action_);
                }
                const int fadeN = std::max(8, std::min(64, stepLen_ / 4));
                // short declick fades at each step edge - except between two gates in a row, which stay fully silent
                const float eIn = (action_ == 'G' && prevAct_ == 'G') ? 1.f : (float)std::min(tStep_, fadeN) / (float)fadeN;
                const float eOut = (action_ == 'G' && nextAct_ == 'G') ? 1.f : (float)std::max(0, std::min(stepLen_ - tStep_, fadeN)) / (float)fadeN;
                const float e = std::min(eIn, eOut);
                switch (action_) {
                    case 'G': oL *= 1.f - e; oR *= 1.f - e; break;
                    case 'S': { const size_t k = (size_t)((stepStart_ + (tStep_ % slice_)) & cmask_);
                                oL = cb_[0][k] * e; oR = cb_[1][k] * e; break; }
                    case 'H': { const double pos = (double)stepStart_ + 0.5 * tStep_;
                                oL = chopRead(0, pos) * e; oR = chopRead(1, pos) * e; break; }
                    case 'T': { const double pos = (double)stepStart_ + tStep_ - (double)tStep_ * tStep_ / (2.0 * stepLen_);
                                oL = chopRead(0, pos) * e; oR = chopRead(1, pos) * e; break; }
                    case 'R': { const size_t k = (size_t)((stepStart_ - 1 - tStep_) & cmask_);
                                oL = cb_[0][k] * e; oR = cb_[1][k] * e; break; }
                    default: break;
                }
                oL = origL + (oL - origL) * (float)chopMix;
                oR = origR + (oR - origR) * (float)chopMix;
                ++tStep_;
            }
            ++cw_;

            { float pl, pr; ps_.process(oL, oR, gRatio, pl, pr); oL = pl; oR = pr; }   // overall pitch (whole signal)

            if (p.softClip >= 0.5) { oL = softClipSample(oL); oR = softClipSample(oR); }
            outL[i] = oL; outR[i] = oR;
            peak = std::max(peak, std::max(std::fabs(oL), std::fabs(oR)));
        }
        return peak;
    }

private:
    struct Grain { bool active = false; double pos = 0; int t = 0, len = 0, fade = 1, att = 0, cutLen = 0; };

    static float hermite(float xm1, float x0, float x1, float x2, float t) {
        float c1 = 0.5f * (x1 - xm1), c2 = xm1 - 2.5f * x0 + 2.f * x1 - 0.5f * x2, c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
        return ((c3 * t + c2) * t + c1) * t + x0;
    }

    float chopRead(int c, double pos) const {
        const int64_t i0 = (int64_t)std::floor(pos);
        const float fr = (float)(pos - (double)i0);
        const float a = cb_[c][(size_t)(i0 & cmask_)], b = cb_[c][(size_t)((i0 + 1) & cmask_)];
        return a + (b - a) * fr;
    }

    void spawn(int L, double fadeFrac, int attSamples, double cutFrac) {
        Grain* slot = nullptr;
        for (auto& g : grains_) if (!g.active) { slot = &g; break; }
        if (!slot) { slot = &grains_[0]; for (auto& g : grains_) if (g.t > slot->t) slot = &g; }
        int fade = std::min(std::max((int)(L * fadeFrac), 1), L / 2);
        slot->active = true; slot->pos = (double)(wpos_ - 3); slot->t = 0; slot->len = L; slot->fade = fade;
        slot->att = std::min(attSamples, L);
        slot->cutLen = cutFrac >= 0.999 ? L : std::max(32, (int)(L * cutFrac));
        hop_ = L - fade;
    }

    void spawnAligned(int L, double fadeFrac, int attSamples, double cutFrac) {
        Grain* slot = nullptr;
        for (auto& g : grains_) if (!g.active) { slot = &g; break; }
        if (!slot) { slot = &grains_[0]; for (auto& g : grains_) if (g.t > slot->t) slot = &g; }
        const int fade = std::min(std::max((int)(L * fadeFrac), 1), L / 2);
        slot->active = true; slot->pos = (double)(wpos_ - 1); slot->t = 0; slot->len = L + fade; slot->fade = fade;
        slot->att = std::min(attSamples, slot->len);
        slot->cutLen = cutFrac >= 0.999 ? slot->len : std::max(32, (int)(slot->len * cutFrac));
    }

    static float windowAt(const Grain& g) {
        if (g.t < g.fade) return 0.5f * (1.f - (float)std::cos(kPi * g.t / g.fade));
        int rem = g.len - g.t;
        if (rem <= g.fade) return 0.5f * (1.f - (float)std::cos(kPi * rem / g.fade));
        return 1.f;
    }

    double sr_ = 44100.0;
    int maxLen_ = 441000;
    std::vector<float> buf_[2], cb_[2];
    int64_t mask_ = 0, wpos_ = 0, cmask_ = 0, cw_ = 0;
    int hop_ = 0;
    bool first_ = true, hpOn_ = false, lpOn_ = false;
    Grain grains_[4];
    Allpass ap_[2][4];
    CutFilter hpf_[2], lpf_[2];
    ModDelay ch_[2][4], fl_[2];
    Reverb rv_;
    PitchShifter ps_;
    int64_t alignIdx_ = INT64_MIN;
    double lastLow_ = -1, lastHigh_ = -1, lastLoR_ = -1, lastHiR_ = -1;
    int lastLoS_ = -1, lastHiS_ = -1;
    Smoothed mix_, fb_, diff_, width_, out_, duckAmt_, spd_, chorus_, flangus_, rvMix_, gp_;
    double env_ = 0, envAtk_ = 0.99, dg_ = 1.0, duckAtk_ = 0.01, beatPos_ = 0;
    double chPh_[4] = {0, 0, 0, 0}, fgPh_ = 0;
    // chopper state
    int64_t curStep_ = INT64_MIN, stepStart_ = 0;
    char action_ = 'N', prevAct_ = 'N', nextAct_ = 'N';
    int tStep_ = 0, stepLen_ = 1, slice_ = 1;
};

}  // namespace apprehension
