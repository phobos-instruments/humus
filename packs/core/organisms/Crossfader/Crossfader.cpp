#include "Crossfader/Crossfader.h"

#include <algorithm>

#include "hum/dsp/FadeLaw.h"

namespace hum {

namespace {
float cutTarget(float fade, bool cutA, bool cutB) {
    if (cutA && cutB) return 0.5f;
    if (cutA) return 0.0f;
    if (cutB) return 1.0f;
    return fade;
}
}

void Crossfader::process(const float* const* in, int numIn,
                         float* const* out, int numOut,
                         int numSamples, const Transport&) {
    const float master = (float) params.get("MasterGain", 1.0);
    const float trimA = (float) params.get("TrimA", 1.0);
    const float trimB = (float) params.get("TrimB", 1.0);
    const float fade = (float) params.get("Fade", 0.5);
    const float curve = (float) params.get("Curve", 0.0);
    const float target = cutTarget(fade,
                                   params.get("CutA", 0.0) >= 0.5,
                                   params.get("CutB", 0.0) >= 0.5);
    const double cutMs = std::max(0.0, params.get("CutTime", 0.0));
    const float step = cutMs <= 0.0 ? 1.0f : (float) (1000.0 / (cutMs * sampleRate_));
    if (!seeded_) {
        seeded_ = true;
        pos_ = target;
    }

    const float* a[2] = {numIn > 0 ? in[0] : nullptr, numIn > 1 ? in[1] : nullptr};
    const float* b[2] = {numIn > 2 ? in[2] : nullptr, numIn > 3 ? in[3] : nullptr};
    float gA = fadeGain(1.0f - pos_, curve) * trimA * master;
    float gB = fadeGain(pos_, curve) * trimB * master;
    for (int n = 0; n < numSamples; ++n) {
        if (pos_ != target) {
            pos_ = pos_ < target ? std::min(target, pos_ + step)
                                 : std::max(target, pos_ - step);
            gA = fadeGain(1.0f - pos_, curve) * trimA * master;
            gB = fadeGain(pos_, curve) * trimB * master;
        }
        for (int c = 0; c < std::min(numOut, 2); ++c) {
            float v = 0.0f;
            if (a[c]) v += gA * a[c][n];
            if (b[c]) v += gB * b[c][n];
            out[c][n] = v;
        }
    }
}

}
