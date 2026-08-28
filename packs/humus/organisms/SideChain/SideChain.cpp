#include "SideChain/SideChain.h"

#include <algorithm>
#include <cmath>

namespace hum {

void SideChain::process(const float* const* in, int numIn, float* const* out, int numOut,
                        int numSamples, const Transport&) {
    const bool listen = params.get("Listen", 0.0) >= 0.5;
    const bool rmsMode = params.get("Detector", 1.0) >= 0.5;
    const double mix = std::clamp(params.get("Mix", 1.0), 0.0, 1.0);
    const double makeup = dbToLin(params.get("Makeup", 0.0));
    core_.set(params.get("Threshold", -30.0), params.get("Ratio", 4.0),
              params.get("Knee", 6.0), params.get("Attack", 1.0),
              params.get("Release", 120.0), params.get("Hold", 0.0), sampleRate_);

    auto tap = [&](int c, int n) -> float {
        return (c < numIn && in[c]) ? in[c][n] : 0.0f;
    };
    const int holdN = (int) (0.015 * sampleRate_);
    const double fall = smoothCoeff(2.0, sampleRate_);
    const double msCoeff = smoothCoeff(8.0, sampleRate_);

    for (int n = 0; n < numSamples; ++n) {
        const float key = linkedPeak(in, numIn, ch_, ch_, n);
        double level;
        if (rmsMode) {
            ms_ = msCoeff * ms_ + (1.0 - msCoeff) * (double) key * (double) key;
            level = std::sqrt(ms_);
        } else {
            if (key >= det_) { det_ = key; detHold_ = holdN; }
            else if (detHold_ > 0) --detHold_;
            else det_ *= fall;
            level = det_;
        }
        const double g = core_.process(level) * makeup;
        if (listen) {
            for (int c = 0; c < numOut; ++c) out[c][n] = tap(ch_ + c, n);
            continue;
        }
        for (int c = 0; c < numOut; ++c) {
            const double dry = tap(c, n);
            out[c][n] = (float) (dry * (1.0 - mix) + dry * g * mix);
        }
    }
}

}
