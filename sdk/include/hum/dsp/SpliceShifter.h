// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

#include "hum/dsp/DspMath.h"

namespace hum {

class SpliceShifter {
public:
    static constexpr int kChannels = 2;
    static constexpr double kCenterSeconds = 0.030;

    void prepare(double sampleRate) {
        sr_ = sampleRate > 0.0 ? sampleRate : kDefaultSampleRate;
        size_ = std::max(64, (int) std::ceil(kBufferSeconds * sr_));
        for (auto& b : buf_) b.assign((size_t) size_, 0.0f);
        mono_.assign((size_t) size_, 0.0f);
        blockLength_ = std::max(1, (int) (kOnsetBlockSeconds * sr_));
        reset();
    }

    void reset() {
        for (auto& b : buf_) std::fill(b.begin(), b.end(), 0.0f);
        std::fill(mono_.begin(), mono_.end(), 0.0f);
        written_ = 0;
        active_ = kCenterSeconds * sr_;
        fading_ = false;
        fadePos_ = 0;
        onsets_.fill(-1);
        onsetHead_ = 0;
        history_.fill(0.0f);
        blockPeak_ = 0.0f;
        blockFill_ = 0;
    }

    void setRatio(double r) { ratio_ = std::clamp(r, 0.25, 4.0); }

    void process(float* frame, int channels) {
        const int n = std::clamp(channels, 1, kChannels);
        write(frame, n);
        if (!fading_) maybeSplice();
        const double step = 1.0 - ratio_;
        if (!fading_) {
            for (int c = 0; c < n; ++c) frame[c] = tap(c, active_);
            active_ = std::max(0.0, active_ + step);
            return;
        }
        const double t = (double) fadePos_ / (double) fadeLength();
        const float in = coherent_ ? (float) (0.5 - 0.5 * std::cos(kPi * t)) : (float) std::sin(0.5 * kPi * t);
        const float out = coherent_ ? 1.0f - in : (float) std::cos(0.5 * kPi * t);
        for (int c = 0; c < n; ++c) frame[c] = out * tap(c, active_) + in * tap(c, next_);
        active_ = std::max(0.0, active_ + step);
        next_ = std::max(0.0, next_ + step);
        if (++fadePos_ >= fadeLength()) {
            active_ = next_;
            fading_ = false;
        }
    }

    double delaySamples() const { return active_; }

private:
    static constexpr double kBufferSeconds = 0.12;
    static constexpr double kHalfSweepSeconds = 0.012;
    static constexpr double kSearchSeconds = 0.008;
    static constexpr double kMatchSeconds = 0.010;
    static constexpr double kFadeSeconds = 0.010;
    static constexpr double kOnsetBlockSeconds = 0.0015;
    static constexpr double kOnsetGuardSeconds = 0.002;
    static constexpr double kHitAheadSeconds = 0.030;
    static constexpr double kQuietFadeShare = 0.75;
    static constexpr double kShortestJumpSeconds = 0.006;
    static constexpr double kHitSearchSeconds = 0.002;
    static constexpr float kOnsetJump = 4.0f;
    static constexpr float kOnsetFloor = 1.0e-3f;
    static constexpr double kCoherent = 0.5;
    static constexpr double kMeaningful = 0.2;
    static constexpr int kHistory = 8;
    static constexpr int kOnsets = 32;
    static constexpr int kCoarse = 4;

    int fadeLength() const { return std::max(2, (int) (kFadeSeconds * sr_)); }
    double seconds(double s) const { return s * sr_; }

    void write(const float* frame, int n) {
        const auto at = (size_t) (written_ % size_);
        float sum = 0.0f;
        for (int c = 0; c < kChannels; ++c) {
            const float v = frame[std::min(c, n - 1)];
            buf_[(size_t) c][at] = v;
            sum += v;
        }
        mono_[at] = 0.5f * sum;
        markOnsets(std::abs(mono_[at]));
        ++written_;
    }

    void markOnsets(float level) {
        blockPeak_ = std::max(blockPeak_, level);
        if (++blockFill_ < blockLength_) return;
        float prior = 0.0f;
        for (float h : history_) prior = std::max(prior, h);
        if (blockPeak_ > kOnsetFloor && blockPeak_ > kOnsetJump * prior)
            onsets_[(size_t) (onsetHead_++ % kOnsets)] = written_ - blockLength_ + 1;
        for (int i = kHistory - 1; i > 0; --i) history_[(size_t) i] = history_[(size_t) (i - 1)];
        history_[0] = blockPeak_;
        blockPeak_ = 0.0f;
        blockFill_ = 0;
    }

    float sampleAt(const std::vector<float>& b, std::int64_t index) const {
        if (index < 0 || index >= written_ || written_ - index > size_) return 0.0f;
        return b[(size_t) (index % size_)];
    }

    float tap(int c, double delay) const {
        const double pos = (double) (written_ - 1) - delay;
        const auto i0 = (std::int64_t) std::floor(pos);
        const auto f = (float) (pos - (double) i0);
        const auto& b = buf_[(size_t) c];
        return sampleAt(b, i0) * (1.0f - f) + sampleAt(b, i0 + 1) * f;
    }

    double fadeReach() const { return ratio_ > 1.0 ? ratio_ * fadeLength() : 0.0; }

    double hitLandingUp() const {
        const double newest = (double) (written_ - 1);
        const double from = newest - active_ + kQuietFadeShare * fadeReach() + seconds(kOnsetGuardSeconds);
        const double to = from + seconds(kHitAheadSeconds) * ratio_;
        const double highest = (double) size_ - seconds(kMatchSeconds) - seconds(kSearchSeconds) - 4.0;
        for (auto o : onsets_)
            if (o >= 0 && (double) o > from && (double) o <= to) {
                const double age = newest - (double) o;
                return std::min(highest, ratio_ * seconds(kCenterSeconds) - (ratio_ - 1.0) * age);
            }
        return 0.0;
    }

    void maybeSplice() {
        if (std::abs(ratio_ - 1.0) < 1.0e-9) return;
        const bool up = ratio_ > 1.0;
        const double shortest = seconds(kShortestJumpSeconds);
        const double hitSearch = seconds(kHitSearchSeconds);
        if (!up)
            if (const double landing = hitLanding(); landing > 0.0 && active_ - landing >= shortest) {
                startSplice(landing, hitSearch);
                return;
            }
        const double trigger = up ? std::max(seconds(kCenterSeconds - kHalfSweepSeconds), (ratio_ - 1.0) * fadeLength() + 2.0)
                                  : seconds(kCenterSeconds + kHalfSweepSeconds);
        const double target = seconds(up ? kCenterSeconds + kHalfSweepSeconds : kCenterSeconds - kHalfSweepSeconds);
        if (up ? active_ > trigger : active_ < trigger) {
            if (const double landing = up ? hitLandingUp() : 0.0; landing > 0.0 && landing >= active_ + shortest)
                startSplice(landing, hitSearch);
            return;
        }
        startSplice(target, seconds(kSearchSeconds));
    }

    double hitLanding() const {
        const double newest = (double) (written_ - 1);
        const double head = newest - active_;
        const double lead = ratio_ * fadeLength() + seconds(kOnsetGuardSeconds);
        const double centre = seconds(kCenterSeconds);
        for (auto o : onsets_) {
            if (o < 0 || (double) o <= head + lead) continue;
            const double age = newest - (double) o;
            const double heard = age + lead / ratio_;
            if (heard >= centre && heard < centre + 1.0) return age + lead;
        }
        return 0.0;
    }

    double match(std::int64_t ref, std::int64_t cand, int length, int stride) const {
        double ab = 0.0, aa = 0.0, bb = 0.0;
        for (int j = 0; j < length; j += stride) {
            const double a = sampleAt(mono_, ref - j), b = sampleAt(mono_, cand - j);
            ab += a * b;
            aa += a * a;
            bb += b * b;
        }
        return aa > 0.0 && bb > 0.0 ? ab / std::sqrt(aa * bb) : 0.0;
    }

    void startSplice(double target, double searchSamples) {
        const auto ref = (std::int64_t) std::floor((double) (written_ - 1) - active_);
        const int jump = (int) std::lround(target - active_);
        const int search = std::max(0, (int) searchSamples);
        const int length = std::max(4, (int) seconds(kMatchSeconds));
        const double lowest = 1.0, highest = (double) size_ - length - 2.0;
        int best = 0;
        double bestScore = -2.0;
        auto consider = [&](int s, int stride) {
            const double cand = active_ + jump + s;
            if (cand < lowest || cand > highest) return;
            const double score = match(ref, ref - jump - s, length, stride);
            if (score > bestScore) { bestScore = score; best = s; }
        };
        for (int s = -search; s <= search; s += kCoarse) consider(s, kCoarse);
        const int around = best;
        bestScore = -2.0;
        const int fine = std::min(kCoarse, search);
        for (int s = around - fine; s <= around + fine; ++s) consider(s, 1);
        if (bestScore < kMeaningful) best = 0;
        next_ = std::clamp(active_ + jump + best, lowest, highest);
        coherent_ = bestScore >= kCoherent;
        fading_ = true;
        fadePos_ = 0;
    }

    double sr_ = kDefaultSampleRate;
    int size_ = 64;
    std::array<std::vector<float>, kChannels> buf_;
    std::vector<float> mono_;
    std::int64_t written_ = 0;
    double ratio_ = 1.0;
    double active_ = 0.0, next_ = 0.0;
    bool fading_ = false, coherent_ = true;
    int fadePos_ = 0;
    std::array<std::int64_t, kOnsets> onsets_{};
    int onsetHead_ = 0;
    std::array<float, kHistory> history_{};
    float blockPeak_ = 0.0f;
    int blockFill_ = 0, blockLength_ = 1;
};

}
