// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <algorithm>
#include <array>

#include "hum/dsp/SmoothedGain.h"

namespace hum {

class VoiceSpread {
public:
    static constexpr std::array<double, 4> kAllpass{0.62, -0.47, 0.35, -0.71};
    static constexpr double kDepth = 0.7;

    void prepare(double sampleRate) {
        width_.prepare(sampleRate);
        left_.prepare(sampleRate);
        right_.prepare(sampleRate);
        clear();
    }
    void clear() {
        x1_.fill(0.0);
        y1_.fill(0.0);
    }

    void setTargets(double width, double pan) {
        width_.setTarget((float) std::clamp(width, 0.0, 1.0));
        const double p = std::clamp(pan, 0.0, 1.0);
        left_.setTarget((float) (p <= 0.5 ? 1.0 : (1.0 - p) * 2.0));
        right_.setTarget((float) (p >= 0.5 ? 1.0 : p * 2.0));
    }

    void spread(float mono, float& left, float& right) {
        const double side = decorrelated(mono) * kDepth * width_.next();
        left = (float) ((mono + side) * left_.next());
        right = (float) ((mono - side) * right_.next());
    }

private:
    double decorrelated(double x) {
        for (size_t k = 0; k < kAllpass.size(); ++k) {
            const double g = kAllpass[k];
            const double y = -g * x + x1_[k] + g * y1_[k];
            x1_[k] = x;
            y1_[k] = y;
            x = y;
        }
        return x;
    }

    SmoothedGain width_, left_, right_;
    std::array<double, 4> x1_{}, y1_{};
};

}
