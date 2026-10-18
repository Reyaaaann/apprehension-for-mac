// Plain-C++ sanity tests for the Apprehension engine and key detector (no framework needed).
#include "../src/dsp.h"
#include "../src/keydetect.h"
#include "../src/presets.h"
#include <cstdio>
#include <cstring>
#include <vector>

using namespace apprehension;

static int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("FAIL: %s\n", msg); ++failures; } else std::printf("ok:   %s\n", msg); } while (0)

static std::vector<float> run(Engine& e, const std::vector<float>& in0, const Params& p, double bpm = 0, double ppq = -1, int block = 256) {
    // the plugin has a fixed, host-compensated latency: feed extra silence and drop the first `lat` samples so the
    // output lines up with the input exactly as a host's delay compensation would make it
    const size_t lat = (size_t)e.latencySamples();
    std::vector<float> in(in0);
    in.resize(in0.size() + lat, 0.f);
    std::vector<float> out(in.size()), outR(in.size());
    for (size_t i = 0; i < in.size(); i += block) {
        int n = (int)std::min<size_t>(block, in.size() - i);
        double q = ppq >= 0 ? ppq + (double)i / 48000.0 * bpm / 60.0 : -1.0;   // advance the beat position like a host would
        e.process(&in[i], &in[i], &out[i], &outR[i], n, p, bpm, q);
    }
    std::vector<float> res(in0.size());
    for (size_t i = 0; i < in0.size(); ++i) res[i] = out[i + lat];
    return res;
}
static std::vector<float> sine(double f, double sr, double sec, float amp = 0.5f) {
    std::vector<float> v((size_t)(sr * sec));
    for (size_t i = 0; i < v.size(); ++i) v[i] = amp * (float)std::sin(2 * kPi * f * i / sr);
    return v;
}
static int zeroCross(const std::vector<float>& v, size_t a, size_t b) {
    int z = 0; for (size_t i = a + 1; i < b; ++i) if ((v[i - 1] < 0) != (v[i] < 0)) ++z; return z;
}
static double rms(const std::vector<float>& v, size_t a, size_t b) {
    double s = 0; for (size_t i = a; i < b; ++i) s += (double)v[i] * v[i]; return std::sqrt(s / (double)(b - a));
}

int main() {
    const double sr = 48000.0;
    Params base; base.sync = 0; base.timeMs = 100; base.mix = 1.0; base.feedback = 0; base.diffuse = 0; base.smooth = 0.5;

    { // unity crossfade
        Engine e; e.prepare(sr);
        std::vector<float> in((size_t)sr, 1.0f);
        auto out = run(e, in, base);
        float lo = 10, hi = -10;
        for (size_t i = (size_t)(sr * 0.5); i < out.size(); ++i) { lo = std::min(lo, out[i]); hi = std::max(hi, out[i]); }
        CHECK(lo > 0.99f && hi < 1.01f, "grain crossfades sum to unity gain");
    }
    { // really reversed
        Engine e; e.prepare(sr); Params p = base; p.smooth = 0;
        std::vector<float> in((size_t)sr);
        for (size_t i = 0; i < in.size(); ++i) in[i] = (float)i / (float)in.size();
        auto out = run(e, in, p);
        int falling = 0; size_t a = 4800 * 4 + 1000;
        for (size_t i = a + 1; i < a + 1000; ++i) if (out[i] < out[i - 1]) ++falling;
        CHECK(falling > 990, "ramp input plays backwards");
    }
    { // pitch: +12 st doubles frequency, -12 halves it
        auto in = sine(440, sr, 2.0);
        Engine e0, e1, e2; e0.prepare(sr); e1.prepare(sr); e2.prepare(sr);
        Params p0 = base; Params p1 = base; p1.pitchSt = 12; Params p2 = base; p2.pitchSt = -12;
        auto o0 = run(e0, in, p0), o1 = run(e1, in, p1), o2 = run(e2, in, p2);
        size_t a = (size_t)(sr * 1.0), b = a + 24000;
        double z0 = zeroCross(o0, a, b), z1 = zeroCross(o1, a, b), z2 = zeroCross(o2, a, b);
        std::printf("      zero crossings: 0st=%.0f +12st=%.0f -12st=%.0f\n", z0, z1, z2);
        CHECK(std::fabs(z1 / z0 - 2.0) < 0.1, "+12 semitones doubles the frequency");
        CHECK(std::fabs(z2 / z0 - 0.5) < 0.1, "-12 semitones halves the frequency");
    }
    { // pitch +7 = ratio 1.4983
        auto in = sine(440, sr, 2.0);
        Engine e0, e1; e0.prepare(sr); e1.prepare(sr);
        Params p1 = base; p1.pitchSt = 7;
        auto o0 = run(e0, in, base), o1 = run(e1, in, p1);
        size_t a = (size_t)(sr * 1.0), b = a + 24000;
        double r = (double)zeroCross(o1, a, b) / zeroCross(o0, a, b);
        CHECK(std::fabs(r - std::pow(2.0, 7.0 / 12.0)) < 0.05, "+7 semitones gives the right ratio (fractional-speed reads)");
    }
    { // attack slows the swell-in of each reversed chunk
        auto in = sine(300, sr, 2.0);
        Engine e0, e1; e0.prepare(sr); e1.prepare(sr);
        Params p0 = base; p0.timeMs = 400; p0.smooth = 0.02; Params p1 = p0; p1.attackMs = 150;
        auto o0 = run(e0, in, p0), o1 = run(e1, in, p1);
        // chunk starts every ~ (L - fade); look at the first 30 ms after a chunk start later in the signal
        size_t L = 19200, fade = (size_t)(L * 0.0296), hop = L - fade, s = hop * 3;
        double r0 = rms(o0, s + 300, s + 1700), r1 = rms(o1, s + 300, s + 1700);
        std::printf("      early rms: no attack %.3f, attack 150ms %.3f\n", r0, r1);
        CHECK(r1 < r0 * 0.3, "attack knob softens the start of each reversed chunk");
    }
    { // input-ducking lowers the wet while playing
        auto in = sine(300, sr, 2.0);
        Engine e0, e1; e0.prepare(sr); e1.prepare(sr);
        Params p0 = base; p0.mix = 0.9; Params p1 = p0; p1.duckAmt = 1.0; p1.duckMode = 0; p1.duckRelMs = 100;
        auto o0 = run(e0, in, p0), o1 = run(e1, in, p1);
        double r0 = rms(o0, 48000, 90000), r1 = rms(o1, 48000, 90000);
        std::printf("      rms: no duck %.3f, full duck %.3f\n", r0, r1);
        CHECK(r1 < r0 * 0.9, "ducking reduces the wet level while input is playing");
    }
    { // pump mode: gain dips right after each beat and recovers
        std::vector<float> in((size_t)(sr * 4), 0.5f);
        Engine e; e.prepare(sr);
        Params p = base; p.sync = 0; p.timeMs = 50; p.smooth = 0.5; p.mix = 1.0; p.duckAmt = 1.0; p.duckMode = 1; p.duckRelMs = 250;
        auto o = run(e, in, p, 120.0, 0.0);
        // 120 BPM -> beat = 24000 samples. Compare just after a beat vs. mid-beat.
        double early = rms(o, 24000 * 2 + 20, 24000 * 2 + 400), mid = rms(o, 24000 * 2 + 12000, 24000 * 2 + 12400);
        std::printf("      pump: just after beat %.3f, mid beat %.3f\n", early, mid);
        CHECK(early < mid * 0.3, "pump ducking dips on the beat and recovers");
    }
    { // freeze keeps looping the last chunk after input stops
        auto in = sine(330, sr, 1.0);
        in.resize((size_t)(sr * 3), 0.f);   // 1 s of tone then silence
        Engine eOff, eOn; eOff.prepare(sr); eOn.prepare(sr);
        Params p = base; p.timeMs = 200; p.mix = 1.0;
        Params pf = p;
        std::vector<float> o1((size_t)in.size()), o2(o1.size()), tmp(o1.size());
        // freeze flips on at t = 0.9 s
        size_t split = (size_t)(sr * 0.9);
        eOn.process(&in[0], &in[0], &o1[0], &tmp[0], (int)split, p, 0, -1);
        pf.freeze = 1.0;
        eOn.process(&in[split], &in[split], &o1[split], &tmp[split], (int)(in.size() - split), pf, 0, -1);
        eOff.process(&in[0], &in[0], &o2[0], &tmp[0], (int)in.size(), p, 0, -1);
        double on = rms(o1, (size_t)(sr * 2.0), (size_t)(sr * 2.5)), off = rms(o2, (size_t)(sr * 2.0), (size_t)(sr * 2.5));
        std::printf("      late rms: freeze %.3f, normal %.3f\n", on, off);
        CHECK(on > 0.1 && off < 0.01, "freeze keeps sound looping after input stops");
    }
    { // stability with max feedback + extreme pitch
        Engine e; e.prepare(sr);
        Params p = base; p.feedback = 0.95; p.mix = 0.5; p.pitchSt = 24; p.timeMs = 3000;
        std::vector<float> in((size_t)(sr * 6));
        unsigned s = 1; for (auto& v : in) { s = s * 1664525u + 1013904223u; v = ((s >> 8) / 8388608.0f - 1.f); }
        auto out = run(e, in, p);
        bool ok = true; float mx = 0; for (float v : out) { if (!std::isfinite(v)) ok = false; mx = std::max(mx, std::fabs(v)); }
        CHECK(ok && mx < 8.0f, "max feedback + max pitch stays finite and bounded");
    }
    { // mix=0 dry
        Engine e; e.prepare(sr); Params p; p.mix = 0; p.sync = 0;
        auto in = sine(440, sr, 1.0);
        auto out = run(e, in, p);
        float err = 0; for (size_t i = 0; i < in.size(); ++i) err = std::max(err, std::fabs(out[i] - in[i]));
        CHECK(err < 1e-5f, "mix=0 passes dry signal unchanged");
    }
    { // block size independence
        Params q = base; q.mix = 0.7; q.feedback = 0.4; q.diffuse = 0.5; q.pitchSt = 5.3; q.attackMs = 20; q.duckAmt = 0.5;
        auto in = sine(220, sr, 1.5);
        Engine a, b; a.prepare(sr); b.prepare(sr);
        auto o1 = run(a, in, q, 0, -1, 64), o2 = run(b, in, q, 0, -1, 1000);
        float d = 0; for (size_t i = 0; i < in.size(); ++i) d = std::max(d, std::fabs(o1[i] - o2[i]));
        CHECK(d < 1e-5f, "output identical for different block sizes");
    }


    { // cutoff ends each reversed chunk early
        auto in = sine(300, sr, 3.0);
        Engine e0, e1; e0.prepare(sr); e1.prepare(sr);
        Params p0 = base; p0.timeMs = 400; p0.smooth = 0.02; Params p1 = p0; p1.cutoff = 0.3; p1.releaseMs = 20;
        auto o0 = run(e0, in, p0), o1 = run(e1, in, p1);
        size_t L = 19200, fade = (size_t)(L * 0.0296), hop = L - fade, s0 = hop * 3;
        double full = rms(o0, s0 + 8000, s0 + 16000), cut = rms(o1, s0 + 8000, s0 + 16000), early = rms(o1, s0 + 500, s0 + 4000);
        std::printf("      rms: no cutoff %.3f | cutoff 30%% after the cut %.4f, before the cut %.3f\n", full, cut, early);
        CHECK(cut < 0.01 && early > 0.2, "cutoff stops the sound partway through each chunk");
    }
    { // pump ducking follows the chosen note division
        std::vector<float> in((size_t)(sr * 4), 0.5f);
        Params q = base; q.sync = 0; q.timeMs = 50; q.mix = 1.0; q.duckAmt = 1.0; q.duckMode = 1; q.duckRelMs = 60;
        double dipEighth, dipQuarter;
        { Engine e; e.prepare(sr); q.duckDiv = 1; auto o = run(e, in, q, 120.0, 0.0);     // 1/8 = every 12000 samples
          dipEighth = rms(o, 12000 * 3 + 20, 12000 * 3 + 300); }
        { Engine e; e.prepare(sr); q.duckDiv = 2; auto o = run(e, in, q, 120.0, 0.0);     // 1/4 = every 24000 samples
          dipQuarter = rms(o, 12000 * 3 + 20, 12000 * 3 + 300); }
        std::printf("      level at an 1/8 boundary: 1/8 setting %.3f, 1/4 setting %.3f\n", dipEighth, dipQuarter);
        CHECK(dipEighth < 0.2 && dipQuarter > 0.4, "ducking division 1/8 vs 1/4 changes where the dips land");
    }
    { // 24 dB/oct filters: high cut down to 70 Hz, low cut up high
        auto hi = sine(1000, sr, 1.5), lo = sine(100, sr, 1.5);
        Engine a, b, c, d; a.prepare(sr); b.prepare(sr); c.prepare(sr); d.prepare(sr);
        Params pf = base; pf.smooth = 1.0; pf.mix = 1.0; Params pLP = pf; pLP.highCut = 70; Params pHP = pf; pHP.lowCut = 2000;
        double ref1 = rms(run(a, hi, pf), 48000, 72000), lp1 = rms(run(b, hi, pLP), 48000, 72000);
        double ref2 = rms(run(c, lo, pf), 48000, 72000), hp2 = rms(run(d, lo, pHP), 48000, 72000);
        std::printf("      1 kHz through high cut 70 Hz: %.1f dB | 100 Hz through low cut 2 kHz: %.1f dB\n", 20 * std::log10(lp1 / ref1 + 1e-9), 20 * std::log10(hp2 / ref2 + 1e-9));
        CHECK(lp1 < ref1 * 0.01 && hp2 < ref2 * 0.01, "filters remove at least 40 dB outside the cut points");
        CHECK(std::fabs(cutResponseDb(1000, 1000, 3, 0, 20000, 3, 0) + 3.01) < 0.1 && std::fabs(cutResponseDb(70, 20, 3, 0, 70, 3, 0) + 3.01) < 0.1, "graph response is -3 dB at the cut frequency");
    }
    { // soft clip
        auto in = sine(200, sr, 1.0, 2.0f);
        Engine e0, e1; e0.prepare(sr); e1.prepare(sr);
        Params p0 = base; p0.mix = 0.0; Params p1 = p0; p1.softClip = 1;
        auto o0 = run(e0, in, p0), o1 = run(e1, in, p1);
        float m0 = 0, m1 = 0; for (size_t i = 0; i < in.size(); ++i) { m0 = std::max(m0, std::fabs(o0[i])); m1 = std::max(m1, std::fabs(o1[i])); }
        std::printf("      peak without soft clip %.2f, with %.3f\n", m0, m1);
        auto quiet = sine(200, sr, 0.5, 0.3f);
        Engine e2; e2.prepare(sr); auto o2 = run(e2, quiet, p1);
        float err = 0; for (size_t i = 0; i < quiet.size(); ++i) err = std::max(err, std::fabs(o2[i] - quiet[i]));
        CHECK(m0 > 1.9f && m1 <= 1.0f && err < 1e-6f, "soft clip caps peaks at 0 dBFS and leaves quiet signals untouched");
    }
    { // chorus + flangus: audible change, stable
        auto in = sine(330, sr, 2.0);
        Engine e0, e1, e2; e0.prepare(sr); e1.prepare(sr); e2.prepare(sr);
        Params p0 = base; p0.smooth = 1.0; p0.mix = 1.0; Params pc = p0; pc.chorus = 1.0; Params pf = p0; pf.flangus = 1.0;
        auto o0 = run(e0, in, p0), oc = run(e1, in, pc), of = run(e2, in, pf);
        double d1 = 0, d2 = 0; bool fin = true;
        for (size_t i = 48000; i < in.size(); ++i) { d1 += std::fabs(oc[i] - o0[i]); d2 += std::fabs(of[i] - o0[i]); if (!std::isfinite(oc[i]) || !std::isfinite(of[i])) fin = false; }
        CHECK(fin && d1 > 1000 && d2 > 1000, "chorus and flangus change the sound and stay stable");
    }
    { // signature mode (auto-chop)
        CHECK(std::strlen(kChop[0]) == 16 && std::strlen(kChop[7]) == 16, "chop patterns are 16 steps long");
        std::vector<float> in = sine(220, sr, 12.0);   // 6 bars at 120 BPM
        Params ps = base; ps.mix = 0.0; Params po = ps; ps.signature = 1;
        Engine e0, e1, e2; e0.prepare(sr); e1.prepare(sr); e2.prepare(sr);
        auto off = run(e0, in, po, 120.0, 0.0), on = run(e1, in, ps, 120.0, 0.0), on2 = run(e2, in, ps, 120.0, 0.0);
        const size_t step = 6000; int quietSteps = 0, quietOff = 0; double diff = 0;
        for (size_t s0 = 0; s0 + step <= in.size(); s0 += step) {
            if (rms(on, s0 + 1000, s0 + 5000) < 0.02) ++quietSteps;
            if (rms(off, s0 + 1000, s0 + 5000) < 0.02) ++quietOff;
        }
        for (size_t i = 0; i < in.size(); ++i) diff += std::fabs(on[i] - off[i]);
        bool same = true; for (size_t i = 0; i < on.size(); ++i) if (on[i] != on2[i]) same = false;
        std::printf("      silent 1/16 steps with signature on: %d, off: %d\n", quietSteps, quietOff);
        CHECK(quietSteps >= 3 && quietOff == 0 && diff > 1000, "signature mode chops the signal (gates, stutters, half-time...) on the grid");
        CHECK(same, "chop pattern is deterministic");
    }


    { // cut shapes: 6 dB/oct all the way to brickwall (measured through the engine)
        auto tone = [&](double f) { return sine(f, sr, 1.2); };
        auto level = [&](double f, double hiCut, int slope, double res, bool low) {
            Engine e; e.prepare(sr); Params q = base; q.smooth = 1.0; q.mix = 1.0;
            if (low) { q.lowCut = hiCut; q.lowSlope = slope; q.lowRes = res; } else { q.highCut = hiCut; q.highSlope = slope; q.highRes = res; }
            return rms(run(e, tone(f), q), 38000, 56000);
        };
        Engine e0; e0.prepare(sr);
        const double ref2k = rms(run(e0, tone(2000), base), 38000, 56000), ref1250 = rms(run(e0, tone(1250), base), 38000, 56000);
        (void)ref1250;
        double s6 = 20 * std::log10(level(2000, 1000, 0, 0, false) / (ref2k + 1e-9) + 1e-9);
        double s24 = 20 * std::log10(level(2000, 1000, 3, 0, false) / (ref2k + 1e-9) + 1e-9);
        double s48 = 20 * std::log10(level(2000, 1000, 5, 0, false) / (ref2k + 1e-9) + 1e-9);
        Engine e1; e1.prepare(sr);
        double refB = rms(run(e1, tone(1250), base), 38000, 56000);
        double bw = 20 * std::log10(level(1250, 1000, 8, 0, false) / (refB + 1e-9) + 1e-9);
        std::printf("      high cut 1 kHz, tone at 2 kHz: 6dB %.1f | 24dB %.1f | 48dB %.1f ; brickwall at 1.25 kHz %.1f dB\n", s6, s24, s48, bw);
        CHECK(s6 > -9.5 && s6 < -5.0 && s24 < -20.0 && s24 > -30.0 && s48 < -40.0 && bw < -55.0, "slopes: 6 dB, 24 dB, 48 dB and brickwall all behave");
        double hp6 = 20 * std::log10(level(500, 1000, 0, 0, true) / (rms(run(e0, tone(500), base), 38000, 56000) + 1e-9) + 1e-9);
        double hpB = 20 * std::log10(level(800, 1000, 8, 0, true) / (rms(run(e0, tone(800), base), 38000, 56000) + 1e-9) + 1e-9);
        std::printf("      low cut 1 kHz: tone at 500 Hz with 6dB %.1f dB ; brickwall tone at 800 Hz %.1f dB\n", hp6, hpB);
        CHECK(hp6 > -9.5 && hp6 < -5.0 && hpB < -55.0, "low cut slopes behave too");
        double flat = level(1000, 1000, 1, 0, false), reso = level(1000, 1000, 1, 12, false);
        std::printf("      resonance at the corner: %.1f dB extra\n", 20 * std::log10(reso / flat));
        CHECK(reso / flat > 3.0, "resonance boosts the corner frequency");
        CHECK(std::fabs(cutResponseDb(2000, 20, 3, 0, 1000, 5, 0) - (-48.0)) < 1.5, "graph shows ~-48 dB at one octave above a 48 dB/oct cut");
        // brickwall stability at extreme settings
        Engine eb; eb.prepare(sr); Params qb = base; qb.mix = 1; qb.lowCut = 30; qb.lowSlope = 8; qb.highCut = 70; qb.highSlope = 8; qb.lowRes = 12; qb.highRes = 12;
        std::vector<float> nz((size_t)(sr * 3)); unsigned sd = 7; for (auto& v : nz) { sd = sd * 1664525u + 1013904223u; v = ((sd >> 8) / 8388608.0f - 1.f); }
        auto ob = run(eb, nz, qb); bool ok = true; float mx = 0; for (float v : ob) { if (!std::isfinite(v)) ok = false; mx = std::max(mx, std::fabs(v)); }
        CHECK(ok && mx < 20.f, "brickwall filters with resonance stay stable");
    }
    { // reverb
        auto burst = sine(440, sr, 0.2); burst.resize((size_t)(sr * 6), 0.f);
        Params q = base; q.mix = 1.0; q.rvMix = 1.0; q.feedback = 0; q.diffuse = 0; q.timeMs = 20; q.smooth = 1;
        auto tail = [&](double decay, double mixv) {
            Engine e; e.prepare(sr); Params z = q; z.rvDecay = decay; z.rvMix = mixv;
            auto o = run(e, burst, z); return o; };
        auto o1 = tail(1.0, 1.0), o2 = tail(6.0, 1.0), o0 = tail(2.0, 0.0);
        double r1 = rms(o1, (size_t)(sr * 3.0), (size_t)(sr * 3.5)), r2 = rms(o2, (size_t)(sr * 3.0), (size_t)(sr * 3.5));
        double early = rms(o2, (size_t)(sr * 0.25), (size_t)(sr * 0.5));
        std::printf("      reverb tail at 3 s: decay 1 s -> %.5f, decay 6 s -> %.5f ; level just after the burst %.3f\n", r1, r2, early);
        CHECK(r2 > r1 * 3.0 && r2 > 0.001, "decay knob lengthens the reverb tail");
        CHECK(early > 0.02 && early < 2.0, "reverb output level is sensible");
        bool fin = true; for (float v : o2) if (!std::isfinite(v)) fin = false;
        CHECK(fin, "reverb stays finite");
        // all five types and extreme settings stay stable
        bool ok = true;
        for (int t = 0; t < kNumReverbTypes; ++t) {
            Engine e; e.prepare(sr); Params z = q; z.rvType = t; z.rvSize = 1.0; z.rvDecay = 15.0; z.rvDamp = 0.0; z.rvMod = 1.0; z.rvPreMs = 250;
            auto o = run(e, burst, z); for (float v : o) if (!std::isfinite(v) || std::fabs(v) > 20.f) ok = false;
        }
        CHECK(ok, "all reverb types are stable at max size, decay and modulation");
        double mid = 0, dry = 0; for (size_t i = (size_t)(sr * 3); i < (size_t)(sr * 4); ++i) { mid += std::fabs(o0[i]); dry += std::fabs(o2[i]); }
        CHECK(mid < dry * 0.2 || mid < 1e-4, "reverb mix 0 leaves no tail");
    }
    { // chorus and flanger modes are all different and stable
        auto in = sine(330, sr, 2.0);
        std::vector<std::vector<float>> oc, of;
        Engine ref; ref.prepare(sr); Params p0 = base; p0.smooth = 1.0; p0.mix = 1.0;
        auto o0 = run(ref, in, p0);
        bool fin = true; double minDiff = 1e9, minDiffF = 1e9, minRef = 1e9;
        for (int m = 0; m < kNumChorusModes; ++m) { Engine e; e.prepare(sr); Params z = p0; z.chorus = 1.0; z.chorusMode = m; oc.push_back(run(e, in, z)); }
        for (int m = 0; m < kNumFlangerModes; ++m) { Engine e; e.prepare(sr); Params z = p0; z.flangus = 1.0; z.flangusMode = m; of.push_back(run(e, in, z)); }
        for (size_t a = 0; a < oc.size(); ++a) {
            double dr = 0; for (size_t i = 48000; i < in.size(); ++i) { dr += std::fabs(oc[a][i] - o0[i]); if (!std::isfinite(oc[a][i])) fin = false; }
            minRef = std::min(minRef, dr);
            for (size_t b = a + 1; b < oc.size(); ++b) { double d = 0; for (size_t i = 48000; i < in.size(); ++i) d += std::fabs(oc[a][i] - oc[b][i]); minDiff = std::min(minDiff, d); }
        }
        for (size_t a = 0; a < of.size(); ++a) {
            double dr = 0; for (size_t i = 48000; i < in.size(); ++i) { dr += std::fabs(of[a][i] - o0[i]); if (!std::isfinite(of[a][i])) fin = false; }
            minRef = std::min(minRef, dr);
            for (size_t b = a + 1; b < of.size(); ++b) { double d = 0; for (size_t i = 48000; i < in.size(); ++i) d += std::fabs(of[a][i] - of[b][i]); minDiffF = std::min(minDiffF, d); }
        }
        std::printf("      smallest difference between two chorus modes %.0f, flanger modes %.0f\n", minDiff, minDiffF);
        CHECK(fin && minRef > 500 && minDiff > 500 && minDiffF > 500, "5 chorus modes and 5 flanger modes each sound different and stay stable");
    }
    { // tweakable chop
        std::vector<float> in = sine(220, sr, 12.0);
        Params ps = base; ps.mix = 0.0; ps.signature = 1;
        auto quiet = [&](const Params& q, double bpmv = 120.0) {
            Engine e; e.prepare(sr); auto o = run(e, in, q, bpmv, 0.0);
            int n = 0; const size_t step = 6000; for (size_t s0 = 0; s0 + step <= in.size(); s0 += step) if (rms(o, s0 + 1000, s0 + 5000) < 0.02) ++n;
            return n; };
        Params off = ps; off.signature = 0;
        Params noInt = ps; noInt.chopIntensity = 0.0;
        Params noGate = ps; noGate.cGate = 0;
        Params half = ps; half.chopIntensity = 0.5;
        int qFull = quiet(ps), qNoGate = quiet(noGate), qNone = quiet(noInt), qHalf = quiet(half);
        std::printf("      silent steps: full %d | gates off %d | intensity 0 %d | intensity 50%% %d\n", qFull, qNoGate, qNone, qHalf);
        CHECK(qFull >= 3 && qNoGate == 0 && qNone == 0 && qHalf < qFull, "intensity and per-action switches change the chop");
        { // custom pattern: gate every step => silence
            Engine e; e.prepare(sr); for (int i = 0; i < 16; ++i) e.setCustom(i, 'G');
            Params c = ps; c.chopPattern = 9; auto o = run(e, in, c, 120.0, 0.0);
            CHECK(rms(o, 48000, 90000) < 0.02, "custom pattern is used when Pattern = Custom");
            Engine e2; e2.prepare(sr); for (int i = 0; i < 16; ++i) e2.setCustom(i, 'N');
            auto o2 = run(e2, in, c, 120.0, 0.0);
            CHECK(rms(o2, 48000, 90000) > 0.3, "custom 'normal' steps pass the sound");
        }
        { // chop mix 0 = untouched
            Engine e; e.prepare(sr); Params c = ps; c.chopMix = 0.0; auto o = run(e, in, c, 120.0, 0.0);
            Engine e2; e2.prepare(sr); auto o2 = run(e2, in, off, 120.0, 0.0);
            float d = 0; for (size_t i = 0; i < o.size(); ++i) d = std::max(d, std::fabs(o[i] - o2[i]));
            CHECK(d < 1e-5f, "chop mix 0 leaves the sound untouched");
        }
        { // seed changes the auto pattern; same seed repeats
            auto cnt = [&](double seed) { Params c = ps; c.chopSeed = seed; return quiet(c); };
            bool differs = false; for (int sd = 1; sd < 12; ++sd) if (cnt(sd) != cnt(0)) differs = true;
            CHECK(differs && cnt(5) == cnt(5), "seed changes the generated patterns deterministically");
        }
        { // rate: 1/32 steps give twice the step changes of 1/16
            Engine e; e.prepare(sr); Params c = ps; c.chopRate = 2; auto o = run(e, in, c, 120.0, 0.0);
            bool fin = true; for (float v : o) if (!std::isfinite(v)) fin = false;
            CHECK(fin, "1/32 chop rate runs cleanly");
        }
    }

    { // overall pitch: shifts the WHOLE signal (even the dry path), with fixed latency
        Engine e0; e0.prepare(sr);
        CHECK(e0.latencySamples() == (int)(0.050 * sr) / 2 * 1 || std::abs(e0.latencySamples() - (int)(0.025 * sr)) <= 1, "reports a fixed latency of about 25 ms");
        auto in = sine(440, sr, 3.0);
        auto zc = [&](double st) {
            Engine e; e.prepare(sr); Params q = base; q.mix = 0.0; q.gPitchSt = st;   // mix 0 = dry only
            auto o = run(e, in, q); return (double)zeroCross(o, 48000, 96000); };
        const double z0 = zc(0), zUp = zc(12), zDn = zc(-12), z7 = zc(7);
        std::printf("      dry-only zero crossings: 0st=%.0f +12st=%.0f -12st=%.0f +7st=%.0f\n", z0, zUp, zDn, z7);
        CHECK(std::fabs(zUp / z0 - 2.0) < 0.12 && std::fabs(zDn / z0 - 0.5) < 0.08 && std::fabs(z7 / z0 - std::pow(2.0, 7.0 / 12.0)) < 0.08,
              "overall pitch changes the pitch of the dry signal by the right amount");
        Engine e1; e1.prepare(sr); Params q0 = base; q0.mix = 0;
        auto o0 = run(e1, in, q0);
        float err = 0; for (size_t i = 0; i < in.size(); ++i) err = std::max(err, std::fabs(o0[i] - in[i]));
        CHECK(err < 1e-5f, "overall pitch at 0 is an exact pass-through (after compensating the latency)");
        Engine e2; e2.prepare(sr); Params qn = base; qn.mix = 0.5; qn.gPitchSt = 12; qn.feedback = 0.5;
        std::vector<float> nz((size_t)(sr * 4)); unsigned sd = 3; for (auto& v : nz) { sd = sd * 1664525u + 1013904223u; v = ((sd >> 8) / 8388608.0f - 1.f); }
        auto on = run(e2, nz, qn); bool ok = true; float mx = 0; for (float v : on) { if (!std::isfinite(v)) ok = false; mx = std::max(mx, std::fabs(v)); }
        CHECK(ok && mx < 4.f, "overall pitch +12 on noise stays finite and bounded");
    }
    { // time align: reversed chunks land on the host beat grid
        const double beat = 24000;   // 120 BPM
        std::vector<float> in((size_t)(sr * 6), 0.f);
        for (int k = 1; k <= 7; ++k) in[(size_t)(k * beat)] = 1.0f;   // hits exactly on the beat
        Params q = base; q.mix = 1.0; q.sync = 1; q.div = 4; q.smooth = 0.02; q.feedback = 0; q.diffuse = 0; q.timeAlign = 1;
        Engine ea; ea.prepare(sr); auto oa = run(ea, in, q, 120.0, 0.0);
        Params qu = q; qu.timeAlign = 0;
        Engine eu; eu.prepare(sr); auto ou = run(eu, in, qu, 120.0, 0.0);
        auto offGrid = [&](const std::vector<float>& o) {   // how far each peak sits from the nearest beat, in samples
            double worst = 0; int peaks = 0;
            for (size_t i = 1; i + 1 < o.size(); ++i)
                if (std::fabs(o[i]) > 0.3f && std::fabs(o[i]) >= std::fabs(o[i - 1]) && std::fabs(o[i]) >= std::fabs(o[i + 1])) {
                    ++peaks; double d = std::fmod((double)i, beat); d = std::min(d, beat - d); worst = std::max(worst, d); }
            return std::make_pair(peaks, worst); };
        auto ra = offGrid(oa), ru = offGrid(ou);
        std::printf("      aligned: %d peaks, worst %.1f samples off the beat | free-running: %d peaks, worst %.0f samples off\n", ra.first, ra.second, ru.first, ru.second);
        CHECK(ra.first >= 4 && ra.second <= 3.0, "with Time Align every reversed hit lands on the beat grid");
        CHECK(ru.second > 100.0, "without Time Align the chunks drift off the grid");
    }
    { // all parameter specs are sane
        bool ok = true;
        for (int i = 0; i < P_COUNT; ++i) {
            const ParamSpec& s = kSpec[i];
            if (!(s.mn <= s.def && s.def <= s.mx) || s.mx < s.mn) ok = false;
            if (s.kind == K_CHOICE && (s.nChoices < 2 || s.choices == nullptr || s.mx != s.nChoices - 1)) ok = false;
            for (int j = i + 1; j < P_COUNT; ++j) if (std::strcmp(kIds[i], kIds[j]) == 0) ok = false;
        }
        CHECK(ok, "parameter table is consistent (ranges, defaults, choices, unique ids)");
    }
    { // presets
        const auto& list = presets();
        CHECK(list.size() == 20, "there are exactly 20 presets");
        bool ok = true, unique = true;
        for (size_t a = 0; a < list.size(); ++a) {
            for (size_t b = a + 1; b < list.size(); ++b) if (std::strcmp(list[a].name, list[b].name) == 0) unique = false;
            std::vector<int> seen;
            for (auto& pv : list[a].values) {
                const ParamSpec& sp = kSpec[pv.id];
                if (pv.v < sp.mn - 1e-9 || pv.v > sp.mx + 1e-9) { ok = false; std::printf("      out of range: %s / %s = %g\n", list[a].name, sp.name, pv.v); }
                if (sp.kind != K_FLOAT && pv.v != std::round(pv.v)) ok = false;
                for (int sid : seen) if (sid == (int)pv.id) { ok = false; std::printf("      duplicate: %s / %s\n", list[a].name, sp.name); }
                seen.push_back((int)pv.id);
            }
        }
        CHECK(ok && unique, "all preset values are inside their parameter ranges, no duplicates, unique names");

        // render every preset on a flip-style loop: finite, not silent, sensible level
        std::vector<float> loop((size_t)(sr * 8), 0.f);
        unsigned sd = 11;
        for (size_t i = 0; i < loop.size(); ++i) {
            const double t = (double)i / sr;
            float v = 0.18f * (float)std::sin(2 * kPi * 220 * t) + 0.12f * (float)std::sin(2 * kPi * 330 * t) + 0.10f * (float)std::sin(2 * kPi * 440 * t);
            const double ph = std::fmod(t, 0.5);
            if (ph < 0.06) { sd = sd * 1664525u + 1013904223u; v += (((sd >> 8) / 8388608.0f) - 1.f) * 0.35f * (float)std::exp(-ph * 50); }
            loop[i] = v;
        }
        bool fine = true;
        for (size_t k = 0; k < list.size(); ++k) {
            double vals[P_COUNT]; for (int i = 0; i < P_COUNT; ++i) vals[i] = presetValue((int)k, i);
            Params q = toEngineParams(vals);
            Engine e; e.prepare(sr);
            auto o = run(e, loop, q, 120.0, 0.0);
            double pk = 0; bool fin = true; for (float x : o) { if (!std::isfinite(x)) fin = false; pk = std::max(pk, (double)std::fabs(x)); }
            const double r = rms(o, (size_t)(sr * 2), (size_t)(sr * 8));
            std::printf("      %2zu %-18s peak %.2f  rms %.3f\n", k + 1, list[k].name, pk, r);
            if (!fin || pk > 2.5 || r < 0.01) { fine = false; std::printf("      ^ problem with this preset\n"); }
        }
        CHECK(fine, "every preset renders finite, audible and at a sensible level");
    }

    // ---- key detector ----
    auto chord = [&](std::initializer_list<double> hz, double sec) {
        std::vector<float> v((size_t)(sr * sec), 0.f);
        for (double f : hz) for (int h = 1; h <= 4; ++h)
            for (size_t i = 0; i < v.size(); ++i) v[i] += (float)(0.15 / h * std::sin(2 * kPi * f * h * i / sr));
        return v;
    };
    auto detect = [&](const std::vector<float>& sig, int& root, bool& minor) {
        KeyDetector k; k.prepare(sr);
        for (float x : sig) k.push(x);
        root = k.root.load(); minor = k.minor.load();
    };
    const double C4 = 261.63, D4 = 293.66, E4 = 329.63, F4 = 349.23, G4 = 392.00, A3 = 220.0, A4 = 440.0, B4 = 493.88, Bb4 = 466.16, Eb4 = 311.13, C5 = 523.25;
    {   // C major: I-IV-V-I
        std::vector<float> s;
        for (auto c : {chord({C4, E4, G4}, 1.0), chord({F4, A4, C5}, 1.0), chord({G4, B4, D4 * 2}, 1.0), chord({C4, E4, G4}, 1.0)}) s.insert(s.end(), c.begin(), c.end());
        int r; bool m; detect(s, r, m);
        std::printf("      detected root=%d minor=%d\n", r, (int)m);
        CHECK(r == 0 && !m, "detects C major");
    }
    {   // A minor: i-iv-v-i (Am, Dm, Em, Am)
        std::vector<float> s;
        for (auto c : {chord({A3, C4, E4}, 1.0), chord({D4, F4, A4}, 1.0), chord({E4, G4 * 1.0, B4}, 1.0), chord({A3, C4, E4}, 1.0)}) s.insert(s.end(), c.begin(), c.end());
        int r; bool m; detect(s, r, m);
        std::printf("      detected root=%d minor=%d\n", r, (int)m);
        CHECK(r == 9 && m, "detects A minor");
    }
    {   // C minor: Cm, Fm(F Ab C), Gm-ish(G Bb D), Cm
        std::vector<float> s;
        const double Ab4 = 415.30;
        for (auto c : {chord({C4, Eb4, G4}, 1.0), chord({F4, Ab4, C5}, 1.0), chord({G4, Bb4, D4 * 2}, 1.0), chord({C4, Eb4, G4}, 1.0)}) s.insert(s.end(), c.begin(), c.end());
        int r; bool m; detect(s, r, m);
        std::printf("      detected root=%d minor=%d\n", r, (int)m);
        CHECK(r == 0 && m, "detects C minor");
    }

    std::printf(failures ? "\n%d test(s) FAILED\n" : "\nall tests passed\n", failures);
    return failures ? 1 : 0;
}
