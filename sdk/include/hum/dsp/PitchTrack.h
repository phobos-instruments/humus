// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

class PitchTracker {
public:
    void prepare(double sampleRate) {
        srDecim_ = sampleRate / 4.0;
        for (int i = 0; i < kBuf; ++i)
            hann_[i] = (float) (0.5 - 0.5 * std::cos(kTwoPi * (double) i / (double) (kBuf - 1)));
        reset();
    }

    void setBellCheck(bool on) { bellCheck_ = on; }
    void reset() {
        fill_ = 0;
        phase_ = 0;
        lp1_ = lp2_ = 0.0f;
        pitchHz_ = 0.0;
        clarity_ = 0.0;
        level_ = 0.0;
        for (auto& s : ring_) s = 0.0f;
    }

    int push(const float* x, int n) {
        int fresh = 0;
        for (int i = 0; i < n; ++i) {
            const float a = 0.35f;
            lp1_ += a * (x[i] - lp1_);
            lp2_ += a * (lp1_ - lp2_);
            if (++phase_ < 4) continue;
            phase_ = 0;
            if (fill_ < kBuf) ring_[(size_t) fill_++] = lp2_;
            if (fill_ >= kBuf) {
                analyze();
                std::copy(ring_ + kHopD, ring_ + kBuf, ring_);
                fill_ = kBuf - kHopD;
                ++fresh;
            }
        }
        return fresh;
    }

    double pitchHz() const { return pitchHz_; }
    double clarity() const { return clarity_; }
    double level() const { return level_; }

    void setHzRange(double minHz, double maxHz) {
        if (minHz <= 0.0 || maxHz <= minHz) { lagLo_ = kMinLag; lagHi_ = kMaxLag; return; }
        lagLo_ = std::clamp((int) std::floor(srDecim_ / maxHz), kMinLag, kMaxLag);
        lagHi_ = std::clamp((int) std::ceil(srDecim_ / minHz), kMinLag, kMaxLag);
        if (lagHi_ <= lagLo_) lagHi_ = std::min(kMaxLag, lagLo_ + 1);
    }

    static constexpr int hopSamples() { return kHopD * 4; }

private:
    static constexpr int kWin = 512;
    static constexpr int kMaxLag = 400;
    static constexpr int kMinLag = 8;
    static constexpr int kBuf = kWin + kMaxLag;
    static constexpr int kHopD = 128;

    void analyze() {
        double rms = 0.0;
        for (int i = 0; i < kWin; ++i) rms += (double) ring_[i] * ring_[i];
        level_ = std::sqrt(rms / kWin);

        double nd[kMaxLag + 1];
        nd[0] = 1.0;
        double cum = 0.0;
        for (int lag = 1; lag <= kMaxLag; ++lag) {
            double d = 0.0;
            for (int i = 0; i < kWin; ++i) {
                const double diff = (double) ring_[i] - ring_[i + lag];
                d += diff * diff;
            }
            cum += d;
            nd[lag] = cum > 0.0 ? d * lag / cum : 1.0;
        }

        const int lo = lagLo_, hi = std::min(lagHi_, kMaxLag);
        int pick = 0;
        for (int lag = lo; lag < hi; ++lag)
            if (nd[lag] < 0.15 && nd[lag] <= nd[lag + 1]) {
                while (lag + 1 <= hi && nd[lag + 1] < nd[lag]) ++lag;
                pick = lag;
                break;
            }
        if (pick == 0) {
            double best = 1.0;
            for (int lag = lo; lag <= hi; ++lag)
                if (nd[lag] < best) { best = nd[lag]; pick = lag; }
        }
        if (pick < kMinLag || nd[pick] > 0.5 || level_ < 1.0e-4) {
            pitchHz_ = 0.0;
            clarity_ = 0.0;
            return;
        }
        double lag = pick;
        if (pick > kMinLag && pick < kMaxLag) {
            const double denom = nd[pick - 1] - 2.0 * nd[pick] + nd[pick + 1];
            if (std::abs(denom) > 1.0e-12) {
                const double adj = 0.5 * (nd[pick - 1] - nd[pick + 1]) / denom;
                if (std::abs(adj) < 1.0) lag += adj;
            }
        }
        pitchHz_ = srDecim_ / lag;
        clarity_ = 1.0 - nd[pick];
        if (bellCheck_)
            if (const double low = lowestPartialHz(pitchHz_); low > 0.0) pitchHz_ = low;
    }

    static constexpr double kSilentShare = 0.02;
    static constexpr double kPartialShare = 0.06;
    static constexpr double kHarmonicTolerance = 0.005;

    double sineEnergy() const {
        double sum = 0.0, wsum = 0.0;
        for (int i = 0; i < kBuf; ++i) {
            sum += (double) ring_[i] * ring_[i];
            wsum += hann_[i];
        }
        const double amp = std::sqrt(2.0 * sum / kBuf);
        return 0.25 * amp * amp * wsum * wsum;
    }

    double energyAt(double hz) const {
        const double c = 2.0 * std::cos(kTwoPi * hz / srDecim_);
        double s1 = 0.0, s2 = 0.0;
        for (int i = 0; i < kBuf; ++i) {
            const double s0 = (double) ring_[i] * hann_[i] + c * s1 - s2;
            s2 = s1;
            s1 = s0;
        }
        return s1 * s1 + s2 * s2 - c * s1 * s2;
    }

    double lowestPartialHz(double f) const {
        const double whole = sineEnergy();
        if (whole <= 0.0 || energyAt(f) >= kSilentShare * whole) return 0.0;
        const double step = 0.5 * srDecim_ / kBuf;
        const double top = std::min(8.5 * f, 0.45 * srDecim_);
        double e2 = energyAt(1.05 * f - 2.0 * step), e1 = energyAt(1.05 * f - step);
        for (double h = 1.05 * f; h < top; h += step) {
            const double e0 = energyAt(h);
            if (e1 >= e2 && e1 > e0 && e1 >= kPartialShare * whole) {
                const double a = std::log(std::max(e2, 1e-30)), b = std::log(e1), c = std::log(std::max(e0, 1e-30));
                const double denom = a - 2.0 * b + c;
                const double low = h - step + (std::abs(denom) > 1e-12 ? 0.5 * step * (a - c) / denom : 0.0);
                const double k = std::round(low / f);
                return k >= 1.0 && std::abs(low - k * f) <= kHarmonicTolerance * low ? 0.0 : low;
            }
            e2 = e1;
            e1 = e0;
        }
        return 0.0;
    }

    float ring_[kBuf] = {};
    float hann_[kBuf] = {};
    bool bellCheck_ = false;
    int fill_ = 0;
    int phase_ = 0;
    float lp1_ = 0.0f, lp2_ = 0.0f;
    double srDecim_ = 11025.0;
    double pitchHz_ = 0.0, clarity_ = 0.0, level_ = 0.0;
    int lagLo_ = kMinLag, lagHi_ = kMaxLag;
};

}
