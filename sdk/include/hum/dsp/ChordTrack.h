// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

#include "hum/dsp/DspMath.h"
#include "hum/dsp/FftFrame.h"

namespace hum {

struct ChordVoice {
    int note = -1;
    float salience = 0.0f;
};

class ChordTracker {
public:
    static constexpr int kMaxVoices = 6;
    static constexpr int kHarmonics = 8;
    static constexpr int kDecimate = 4;

    void prepare(double sampleRate) {
        rate_ = sampleRate / kDecimate;
        spectrum_.prepare(kWindow);
        frame_.assign((size_t) kWindow, 0.0f);
        held_.assign((size_t) spectrum_.bins(), 0.0f);
        reset();
    }

    void reset() {
        fill_ = 0;
        phase_ = 0;
        lp1_ = lp2_ = 0.0f;
        level_ = 0.0;
        voices_.clear();
        std::fill(frame_.begin(), frame_.end(), 0.0f);
    }

    void setRange(int lowNote, int highNote) {
        lo_ = std::max(0, lowNote);
        hi_ = std::min(highestNote(), highNote);
        if (hi_ < lo_) hi_ = lo_;
    }

    void setVoices(int wanted) { wanted_ = std::clamp(wanted, 1, kMaxVoices); }
    void setVoiceFloor(float floor) { voiceFloor_ = std::clamp(floor, 0.02f, 0.9f); }

    int highestNote() const {
        const double top = rate_ * 0.5 / (double) kHarmonicsForTop;
        return top > 0.0 ? (int) std::floor(hzToMidi(top)) : 96;
    }

    int push(const float* x, int n) {
        int fresh = 0;
        for (int i = 0; i < n; ++i) {
            const float a = 0.35f;
            lp1_ += a * (x[i] - lp1_);
            lp2_ += a * (lp1_ - lp2_);
            if (++phase_ < kDecimate) continue;
            phase_ = 0;
            frame_[(size_t) fill_++] = lp2_;
            if (fill_ < kWindow) continue;
            analyse();
            std::copy(frame_.begin() + kHop, frame_.end(), frame_.begin());
            fill_ = kWindow - kHop;
            ++fresh;
        }
        return fresh;
    }

    const std::vector<ChordVoice>& voices() const { return voices_; }
    double level() const { return level_; }
    float tonality() const { return 1.0f - tone_; }
    static constexpr int hopSamples() { return kHop * kDecimate; }

private:
    static constexpr int kWindow = 2048;
    static constexpr int kHop = 512;
    static constexpr int kHarmonicsForTop = 3;

    double noteBin(int note) const {
        const double hz = midiToHz(note);
        return rate_ > 0.0 ? hz * spectrum_.size() / rate_ : 0.0;
    }

    static double tolerance(double bin) { return std::max(0.6, bin * kSemitonePart); }

    float salienceOf(int note) const {
        const double first = noteBin(note);
        if (first < 1.0 || !soundsLikeANote(first)) return 0.0f;
        float sum = 0.0f;
        for (int h = 1; h <= kHarmonics; ++h) {
            const double bin = first * h;
            if (bin >= (double) held_.size() - 1) break;
            sum += peakIn(held_, bin) / std::sqrt((float) h);
        }
        return sum;
    }

    bool soundsLikeANote(double first) const {
        float odd = 0.0f, even = 0.0f, loudest = 0.0f;
        for (int h = 1; h <= 5; ++h) {
            const double bin = first * h;
            if (bin >= (double) raw_.size() - 1) break;
            const float at = peakIn(raw_, bin);
            loudest = std::max(loudest, at);
            if (h > 1) (h % 2 == 0 ? even : odd) += at;
        }
        if (peakIn(raw_, first) < loudest * kRootFloor) return false;
        return odd >= even * kOddFloor;
    }

    float flatness() const {
        double logSum = 0.0, sum = 0.0;
        int counted = 0;
        for (size_t k = 1; k < raw_.size(); ++k) {
            const double mag = (double) raw_[k] + 1.0e-9;
            logSum += std::log(mag);
            sum += mag;
            ++counted;
        }
        if (counted == 0 || sum <= 0.0) return 1.0f;
        return (float) (std::exp(logSum / counted) / (sum / counted));
    }

    static int peakBinIn(const std::vector<float>& bins, double bin) {
        const double tol = tolerance(bin);
        const int lo = (int) std::floor(bin - tol), hi = (int) std::ceil(bin + tol);
        int at = -1;
        float best = 0.0f;
        for (int k = lo; k <= hi; ++k)
            if (k >= 0 && k < (int) bins.size() && bins[(size_t) k] >= best) {
                best = bins[(size_t) k];
                at = k;
            }
        return at;
    }

    static float peakIn(const std::vector<float>& bins, double bin) {
        const int at = peakBinIn(bins, bin);
        return at < 0 ? 0.0f : bins[(size_t) at];
    }

    void subtractHarmonics(int note) {
        const double first = noteBin(note);
        if (first < 1.0) return;
        const float root = peakIn(held_, first);
        for (int h = 1; h <= kHarmonics; ++h) {
            const double bin = first * h;
            if (bin >= (double) held_.size() - 1) break;
            const int at = peakBinIn(held_, bin);
            if (at < 0) continue;
            const float take = root / (float) h;
            for (int k = at - kLobe; k <= at + kLobe; ++k)
                if (k >= 0 && k < (int) held_.size())
                    held_[(size_t) k] = std::max(0.0f, held_[(size_t) k] - take);
        }
    }

    bool alreadyHeard(int note) const {
        for (const auto& v : voices_)
            if (v.note == note) return true;
        return false;
    }

    void analyse() {
        double power = 0.0;
        for (int i = 0; i < kWindow; ++i) power += (double) frame_[(size_t) i] * frame_[(size_t) i];
        level_ = std::sqrt(power / kWindow);

        spectrum_.measure(frame_.data());
        raw_ = spectrum_.magnitudes();
        held_ = raw_;

        voices_.clear();
        tone_ = flatness();
        if (tone_ > kNoiseFloor) return;
        const int top = std::min(hi_, highestNote());
        for (int v = 0; v < wanted_; ++v) {
            int best = -1;
            float bestSalience = 0.0f;
            for (int note = lo_; note <= top; ++note) {
                if (alreadyHeard(note)) continue;
                const float s = salienceOf(note);
                if (s > bestSalience) { bestSalience = s; best = note; }
            }
            if (best < 0 || bestSalience <= 0.0f) break;
            if (!voices_.empty() && bestSalience < voices_.front().salience * voiceFloor_) break;
            voices_.push_back({best, bestSalience});
            subtractHarmonics(best);
        }
        std::sort(voices_.begin(), voices_.end(),
                  [](const ChordVoice& a, const ChordVoice& b) { return a.note < b.note; });
    }


    static constexpr float kRootFloor = 0.12f;
    static constexpr double kSemitonePart = 0.025;
    static constexpr float kOddFloor = 0.15f;
    static constexpr int kLobe = 2;
    static constexpr float kNoiseFloor = 0.22f;

    FftFrame spectrum_;
    std::vector<float> frame_, held_, raw_;
    std::vector<ChordVoice> voices_;
    double rate_ = kDefaultSampleRate / kDecimate;
    double level_ = 0.0;
    float tone_ = 1.0f;
    int fill_ = 0, phase_ = 0;
    int lo_ = 24, hi_ = 96, wanted_ = 3;
    float lp1_ = 0.0f, lp2_ = 0.0f;
    float voiceFloor_ = 0.16f;
};

}
