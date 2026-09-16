// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "SideKick/SideKick.h"

#include <algorithm>
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

void SideKick::process(const float* const* in, int numIn, float* const* out, int numOut,
                       int numSamples, const Transport& transport) {
    auto tap = [&](int c, int n) -> float {
        return (c < numIn && in[c]) ? in[c][n] : 0.0f;
    };
    if (!transport.playing()) {
        for (int n = 0; n < numSamples; ++n)
            for (int c = 0; c < numOut; ++c) out[c][n] = tap(c, n);
        g_ = 1.0;
        gainOut_.store(1.0f, std::memory_order_relaxed);
        return;
    }

    pendingShape_.adopt(shape_);

    const double mix = std::clamp(params.get("Mix", 1.0), 0.0, 1.0);
    static const double kBars[] = {1.0, 1.0 / 2, 1.0 / 4, 1.0 / 8, 1.0 / 16};
    const int si = std::clamp((int) params.get("Sync", 2.0), 0, 4);
    const double cycleBeats = kBars[si] * transport.beatsPerBar();
    const double beatsPerSample = transport.tempo() / kSecondsPerMinute / sampleRate_;
    const double smoothMs = std::clamp(params.get("Smooth", 3.0), 0.0, 20.0);
    const double sc = smoothMs > 0.01 ? smoothCoeff(smoothMs, sampleRate_) : 0.0;

    double beats = transport.beats();
    double phase = 0.0;
    for (int n = 0; n < numSamples; ++n) {
        phase = std::fmod(beats, cycleBeats) / cycleBeats;
        beats += beatsPerSample;
        const double target = shape_.eval(phase);
        g_ = sc * g_ + (1.0 - sc) * target;
        for (int c = 0; c < numOut; ++c) {
            const double dry = tap(c, n);
            out[c][n] = (float) (dry * (1.0 - mix) + dry * g_ * mix);
        }
    }
    phaseOut_.store((float) phase, std::memory_order_relaxed);
    gainOut_.store((float) g_, std::memory_order_relaxed);
}

}
