// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Firefly.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace hum {

double Firefly::divisionBeats(int division) {
    static const double beats[] = {4.0, 2.0, 1.0, 0.5, 0.25, 0.125,
                                   4.0 / 3.0, 2.0 / 3.0, 1.0 / 3.0, 3.0, 1.5, 0.75};
    const int n = (int) (sizeof(beats) / sizeof(beats[0]));
    return beats[(std::size_t) std::clamp(division, 0, n - 1)];
}

void Firefly::hueToRgb(double hueDegrees, double saturation, float& r, float& g, float& b) {
    const double h = std::fmod(std::fmod(hueDegrees, 360.0) + 360.0, 360.0) / 60.0;
    const double s = std::clamp(saturation, 0.0, 1.0);
    const double x = 1.0 - std::abs(std::fmod(h, 2.0) - 1.0);
    double rr = 1.0, gg = 1.0, bb = 1.0;
    if (h < 1.0) { rr = 1.0; gg = x; bb = 0.0; }
    else if (h < 2.0) { rr = x; gg = 1.0; bb = 0.0; }
    else if (h < 3.0) { rr = 0.0; gg = 1.0; bb = x; }
    else if (h < 4.0) { rr = 0.0; gg = x; bb = 1.0; }
    else if (h < 5.0) { rr = x; gg = 0.0; bb = 1.0; }
    else { rr = 1.0; gg = 0.0; bb = x; }
    r = (float) (1.0 - s + s * rr);
    g = (float) (1.0 - s + s * gg);
    b = (float) (1.0 - s + s * bb);
}

void Firefly::fire(double strength, double lengthSeconds) {
    seconds_.store((float) std::max(0.004, lengthSeconds), std::memory_order_relaxed);
    strength_.store((float) std::clamp(strength, 0.0, 1.0), std::memory_order_relaxed);
    count_.fetch_add(1, std::memory_order_release);
}

void Firefly::process(const float* const*, int, float* const*, int, int numSamples,
                      const Transport& transport) {
    const double seconds = (double) numSamples / std::max(1.0, sampleRate_);
    const int mode = std::clamp((int) std::lround(params.get("Mode", (double) kModeBeat)), kModeBeat, kModePlayed);
    const double width = std::clamp(params.get("Width", 0.2), 0.02, 0.9);
    const double level = std::clamp(params.get("Level", 1.0), 0.0, 1.0);

    float r = 1.0f, g = 1.0f, b = 1.0f;
    hueToRgb(params.get("Hue", 0.0), params.get("Saturation", 0.0), r, g, b);
    colourR_.store(r, std::memory_order_relaxed);
    colourG_.store(g, std::memory_order_relaxed);
    colourB_.store(b, std::memory_order_relaxed);

    const double beatSeconds = kSecondsPerMinute / std::max(1.0, transport.tempo());
    double period = beatSeconds;
    if (mode == kModeBeat) {
        const double unitBeats = divisionBeats((int) std::lround(params.get("Division", 3.0)));
        period = unitBeats * beatSeconds;
        const long unit = (long) std::floor(transport.beats() / unitBeats);
        if (!transport.playing()) {
            lastUnit_ = -1;
        } else if (unit != lastUnit_) {
            const bool first = lastUnit_ < 0;
            lastUnit_ = unit;
            if (!first) fire(level, width * period);
        }
    } else if (mode == kModeFree) {
        lastUnit_ = -1;
        period = 1.0 / std::clamp(params.get("Rate", 6.0), 0.05, 30.0);
        held_ += seconds;
        while (held_ >= period) {
            held_ -= period;
            fire(level, width * period);
        }
    } else {
        lastUnit_ = -1;
        held_ = 0.0;
    }

    const bool press = params.get("Flash", 0.0) >= 0.5;
    if (press && !wasPress_) fire(level, width * period);
    wasPress_ = press;

    for (int i = 0; i < stagedCount_; ++i) {
        const auto& e = staged_[(std::size_t) i];
        if (e.size >= 3 && (e.data[0] & 0xF0) == 0x90 && e.data[2] > 0)
            fire(level * (double) e.data[2] / (double) kMidiMax, width * period);
    }
    stagedCount_ = 0;
}

}
