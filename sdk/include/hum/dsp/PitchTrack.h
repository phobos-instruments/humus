#pragma once
#include <algorithm>
#include <cmath>

namespace hum {

class PitchTracker {
public:
    void prepare(double sampleRate) {
        srDecim_ = sampleRate / 4.0;
        reset();
    }
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
    }

    float ring_[kBuf] = {};
    int fill_ = 0;
    int phase_ = 0;
    float lp1_ = 0.0f, lp2_ = 0.0f;
    double srDecim_ = 11025.0;
    double pitchHz_ = 0.0, clarity_ = 0.0, level_ = 0.0;
    int lagLo_ = kMinLag, lagHi_ = kMaxLag;
};

}
