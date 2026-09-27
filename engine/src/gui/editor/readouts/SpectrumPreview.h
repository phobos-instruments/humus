// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <cstdint>

#include "hum/dsp/DspMath.h"

namespace hum::spectrum {

class PreviewMix {
public:
    static constexpr double kBpm = 120.0;
    static constexpr std::uint32_t kSeed = 0xC0FFEEu;

    explicit PreviewMix(double sampleRate) : sr_(sampleRate) {}

    void fill(float* out, int n) {
        for (int i = 0; i < n; ++i, ++clock_) out[i] = (float) (0.5 * sample((double) clock_ / sr_));
    }

private:
    static constexpr double kBass[] = {55.0, 55.0, 65.41, 49.0};
    static constexpr double kChord[] = {220.0, 261.63, 329.63, 392.0};

    double noise() {
        rng_ ^= rng_ << 13;
        rng_ ^= rng_ >> 17;
        rng_ ^= rng_ << 5;
        return (double) (rng_ & 0xFFFFFF) / (double) 0x7FFFFF - 1.0;
    }

    double pink(double white) {
        b0_ = 0.99765 * b0_ + white * 0.0990460;
        b1_ = 0.96300 * b1_ + white * 0.2965164;
        b2_ = 0.57000 * b2_ + white * 1.0526913;
        return (b0_ + b1_ + b2_ + white * 0.1848) * 0.25;
    }

    static double saw(double hz, double t, int partials, double tilt) {
        double v = 0.0;
        for (int k = 1; k <= partials; ++k) v += std::sin(kTwoPi * hz * k * t) / std::pow((double) k, tilt);
        return v;
    }

    double sample(double t) {
        const double beats = t * kBpm / kSecondsPerMinute;
        const double beatLen = kSecondsPerMinute / kBpm;
        const double sinceBeat = (beats - std::floor(beats)) * beatLen;
        const double eighths = beats * 2.0;
        const double sinceEighth = (eighths - std::floor(eighths)) * beatLen * 0.5;
        const int beat = (int) std::floor(beats);
        const bool offbeat = ((int) std::floor(eighths) % 2) == 1;

        const double kickHz = 48.0 + 110.0 * std::exp(-sinceBeat * 30.0);
        kickPhase_ += kickHz / sr_;
        if (sinceBeat * sr_ < 1.0) kickPhase_ = 0.0;
        const double kick = 0.9 * std::exp(-sinceBeat * 9.0) * std::sin(kTwoPi * kickPhase_);

        const double bassHz = kBass[(beat / 4) % 4];
        const double bass = 0.16 * std::exp(-sinceEighth * 5.0) * saw(bassHz, t, 14, 1.0);
        double chord = 0.0;
        for (double hz : kChord) chord += 0.035 * saw(hz, t, 8, 1.4);

        const double white = noise();
        const double airy = white - prevWhite_;
        prevWhite_ = white;
        const double hat = 0.5 * std::exp(-sinceEighth * (offbeat ? 30.0 : 70.0)) * airy;
        const bool backbeat = beat % 2 == 1;
        const double snare = backbeat ? std::exp(-sinceBeat * 16.0)
                                            * (0.25 * white + 0.2 * std::sin(kTwoPi * 185.0 * t))
                                      : 0.0;
        return kick + bass + chord + hat + snare + 0.12 * pink(white);
    }

    double sr_;
    std::int64_t clock_ = 0;
    std::uint32_t rng_ = kSeed;
    double b0_ = 0.0, b1_ = 0.0, b2_ = 0.0, prevWhite_ = 0.0, kickPhase_ = 0.0;
};

}
