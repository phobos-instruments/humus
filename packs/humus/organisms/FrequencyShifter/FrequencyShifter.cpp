#include "FrequencyShifter/FrequencyShifter.h"

#include <algorithm>
#include <cmath>

namespace hum {

void FrequencyShifter::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    for (int j = 0; j < kN; ++j) {
        const int k = j - kM;
        double v = 0.0;
        if (k % 2 != 0) {
            const double w = 0.54 - 0.46 * std::cos(2.0 * M_PI * j / (kN - 1));
            v = 2.0 / (M_PI * k) * w;
        }
        h_[(size_t) j] = v;
    }
    reset();
}

void FrequencyShifter::reset() {
    for (auto& r : ring_) r.fill(0.0f);
    wr_ = 0;
    phase_ = 0.0;
}

void FrequencyShifter::process(const float* const* in, int numIn, float* const* out, int numOut,
                               int numSamples, const Transport&) {
    const double shift = params.get("ShiftFrequency", 0.0);
    const float mix = (float) std::clamp(params.get("WetDryMix", 1.0), 0.0, 1.0);
    const double inc = 2.0 * M_PI * shift / sampleRate_;

    for (int n = 0; n < numSamples; ++n) {
        for (int c = 0; c < numOut && c < 2; ++c)
            ring_[(size_t) c][(size_t) wr_] = (c < numIn && in[c]) ? in[c][n] : 0.0f;

        const double cosv = std::cos(phase_), sinv = std::sin(phase_);
        phase_ += inc;
        if (phase_ >= 2.0 * M_PI) phase_ -= 2.0 * M_PI;
        else if (phase_ < 0.0) phase_ += 2.0 * M_PI;

        for (int c = 0; c < numOut && c < 2; ++c) {
            auto& r = ring_[(size_t) c];
            double q = 0.0;
            for (int j = 0; j < kN; ++j) {
                int idx = wr_ - j; if (idx < 0) idx += kN;
                q += h_[(size_t) j] * r[(size_t) idx];
            }
            int iIdx = wr_ - kM; if (iIdx < 0) iIdx += kN;
            const double iVal = r[(size_t) iIdx];
            const float dry = (c < numIn && in[c]) ? in[c][n] : 0.0f;
            const float shifted = (float) (iVal * cosv - q * sinv);
            out[c][n] = dry * (1.0f - mix) + shifted * mix;
        }
        if (++wr_ >= kN) wr_ = 0;
    }
}

}
