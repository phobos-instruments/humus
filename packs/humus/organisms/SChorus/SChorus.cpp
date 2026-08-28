#include "SChorus/SChorus.h"

#include <algorithm>
#include <cmath>

namespace hum {

void SChorus::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    const int maxSamples = (int) (sampleRate * 0.12) + 4;
    lines_[0].prepare(maxSamples);
    lines_[1].prepare(maxSamples);
    reset();
}

void SChorus::process(const float* const* in, int numIn, float* const* out, int numOut,
                      int numSamples, const Transport&) {
    const double minD = std::max(0.0, params.get("MinDelay", 5.0)) * 0.001 * sampleRate_;
    const double depth = std::max(0.0, params.get("DelaySweepDepth", 5.0)) * 0.001 * sampleRate_;
    const double rate = std::clamp(params.get("Rate", 0.5), 0.001, 100.0);
    const double rOff = params.get("RightLFOPhaseOffset", 90.0) / 360.0;
    const float fb = (float) std::clamp(params.get("Feedback", 0.0), -0.95, 0.95);
    const double hf = std::clamp(params.get("HFRolloffFrequency", 12000.0), 200.0, 20000.0);
    const float mix = (float) std::clamp(params.get("WetDryMix", 0.5), 0.0, 1.0);
    lfo_.setRate(rate, sampleRate_);
    const double lpC = 1.0 - std::exp(-2.0 * M_PI * hf / sampleRate_);

    for (int n = 0; n < numSamples; ++n) {
        for (int c = 0; c < numOut && c < 2; ++c) {
            const float dry = (c < numIn && in[c]) ? in[c][n] : 0.0f;
            const double ph = Lfo::wrap(lfo_.phase() + (c == 1 ? rOff : 0.0));
            const double tri = Lfo::triangleUp(ph);
            const float d = (float) (minD + depth * tri);
            float wet = lines_[(size_t) c].read(d);
            double& lp = lp_[(size_t) c];
            lp += lpC * (wet - lp);
            wet = (float) lp;
            lines_[(size_t) c].write(dry + fb * wet);
            out[c][n] = dry * (1.0f - mix) + wet * mix;
        }
        lfo_.tick();
    }
}

}
