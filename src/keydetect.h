// Apprehension - real-time musical key detector (chroma + Krumhansl-Schmuckler). No dependencies.
#pragma once
#include <algorithm>
#include <atomic>
#include <cmath>
#include <vector>

namespace apprehension {

class KeyDetector {
public:
    static constexpr int N = 16384, HOP = 8192;
    std::atomic<int> root{-1};          // 0 = C ... 11 = B, -1 = nothing heard yet
    std::atomic<bool> minor{false};
    std::atomic<float> confidence{0.f}; // best correlation, roughly 0..1
    std::atomic<float> chroma[12];

    KeyDetector() { for (auto& c : chroma) c.store(0.f); }

    void prepare(double sampleRate) {
        const double pi = 3.14159265358979323846;
        sr_ = sampleRate;
        ring_.assign(N, 0.f); re_.assign(N, 0.f); im_.assign(N, 0.f); win_.assign(N, 0.f);
        cosT_.assign(N / 2, 0.f); sinT_.assign(N / 2, 0.f);
        for (int i = 0; i < N; ++i) win_[(size_t)i] = 0.5f * (1.f - (float)std::cos(2.0 * pi * i / (N - 1)));
        for (int i = 0; i < N / 2; ++i) {
            cosT_[(size_t)i] = (float)std::cos(2.0 * pi * i / N);
            sinT_[(size_t)i] = (float)-std::sin(2.0 * pi * i / N);
        }
        binPc_.assign(N / 2, -1); binW_.assign(N / 2, 0.f);
        for (int k = 1; k < N / 2; ++k) {
            double f = k * sr_ / N;
            if (f < 65.0 || f > 1100.0) continue;
            double midi = 69.0 + 12.0 * std::log2(f / 440.0), r = std::round(midi), d = midi - r;
            binPc_[(size_t)k] = (((int)r % 12) + 12) % 12;
            binW_[(size_t)k] = (float)std::max(0.0, 1.0 - 2.0 * std::fabs(d));
        }
        for (auto& a : acc_) a = 0.0;
        w_ = 0; count_ = 0;
        root.store(-1);
    }

    void push(float x) {
        if (ring_.empty()) return;
        ring_[(size_t)w_] = x;
        w_ = (w_ + 1) & (N - 1);
        if (++count_ >= HOP) { count_ = 0; analyse(); }
    }

private:
    void analyse() {
        double e = 0;
        for (int i = 0; i < N; ++i) {
            float v = ring_[(size_t)((w_ + i) & (N - 1))];
            e += (double)v * v;
            re_[(size_t)i] = v * win_[(size_t)i]; im_[(size_t)i] = 0.f;
        }
        if (e / N < 1e-6) return;   // too quiet (below about -60 dBFS): keep the last result
        fft();
        double frame[12] = {}, sum = 0;
        for (int k = 1; k < N / 2; ++k) {
            int pc = binPc_[(size_t)k];
            if (pc < 0) continue;
            double mag = std::sqrt((double)re_[(size_t)k] * re_[(size_t)k] + (double)im_[(size_t)k] * im_[(size_t)k]);
            frame[pc] += binW_[(size_t)k] * mag;
        }
        // harmonic suppression: the 3rd harmonic of a note lands on the pitch class 7 semitones above it,
        // which would otherwise make the fifth look like a chord/key note in its own right
        double cleaned[12];
        for (int p = 0; p < 12; ++p) cleaned[p] = std::max(0.0, frame[p] - 0.3 * frame[(p + 5) % 12]);
        for (int p = 0; p < 12; ++p) frame[p] = cleaned[p];
        for (double v : frame) sum += v;
        if (sum <= 0) return;
        for (int p = 0; p < 12; ++p) acc_[p] = 0.8 * acc_[p] + frame[p] / sum;

        static const double maj[12] = {6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88};
        static const double mnr[12] = {6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17};
        double best = -2, mean = 0, mx = 0;
        int bestRoot = 0; bool bestMinor = false;
        for (double a : acc_) { mean += a; mx = std::max(mx, a); }
        mean /= 12.0;
        for (int mode = 0; mode < 2; ++mode) {
            const double* prof = mode ? mnr : maj;
            double pm = 0; for (int i = 0; i < 12; ++i) pm += prof[i];
            pm /= 12.0;
            for (int t = 0; t < 12; ++t) {
                double num = 0, da = 0, dp = 0;
                for (int i = 0; i < 12; ++i) {
                    double a = acc_[(t + i) % 12] - mean, b = prof[i] - pm;
                    num += a * b; da += a * a; dp += b * b;
                }
                double c = num / std::sqrt(da * dp + 1e-12);
                if (c > best) { best = c; bestRoot = t; bestMinor = (mode == 1); }
            }
        }
        for (int p = 0; p < 12; ++p) chroma[p].store((float)(mx > 0 ? acc_[p] / mx : 0.0));
        confidence.store((float)std::max(0.0, best));
        minor.store(bestMinor);
        root.store(bestRoot);
    }

    void fft() {
        for (int i = 1, j = 0; i < N; ++i) {
            int bit = N >> 1;
            for (; j & bit; bit >>= 1) j ^= bit;
            j ^= bit;
            if (i < j) { std::swap(re_[(size_t)i], re_[(size_t)j]); std::swap(im_[(size_t)i], im_[(size_t)j]); }
        }
        for (int len = 2; len <= N; len <<= 1) {
            int half = len >> 1, step = N / len;
            for (int i = 0; i < N; i += len)
                for (int k = 0; k < half; ++k) {
                    float wr = cosT_[(size_t)(k * step)], wi = sinT_[(size_t)(k * step)];
                    size_t a = (size_t)(i + k), b = a + (size_t)half;
                    float tr = re_[b] * wr - im_[b] * wi, ti = re_[b] * wi + im_[b] * wr;
                    re_[b] = re_[a] - tr; im_[b] = im_[a] - ti;
                    re_[a] += tr; im_[a] += ti;
                }
        }
    }

    double sr_ = 44100.0, acc_[12] = {};
    int w_ = 0, count_ = 0;
    std::vector<float> ring_, re_, im_, win_, cosT_, sinT_, binW_;
    std::vector<int> binPc_;
};

}  // namespace apprehension
