#include "Crossfader/Crossfader.h"

#include <algorithm>

#include "hum/dsp/FadeLaw.h"

namespace hum {

void Crossfader::process(const float* const* in, int numIn,
                         float* const* out, int numOut,
                         int numSamples, const Transport&) {
    const float master = (float) params.get("MasterGain", 1.0);
    const float trimA = (float) params.get("TrimA", 1.0);
    const float trimB = (float) params.get("TrimB", 1.0);
    const float fade = (float) params.get("Fade", 0.5);
    const float curve = (float) params.get("Curve", 0.0);
    const float gA = fadeGain(1.0f - fade, curve) * trimA * master;
    const float gB = fadeGain(fade, curve) * trimB * master;

    for (int c = 0; c < numOut; ++c) {
        const float* a = (c < numIn) ? in[c] : nullptr;
        const float* b = (c + 2 < numIn) ? in[c + 2] : nullptr;
        for (int n = 0; n < numSamples; ++n) {
            float v = 0.0f;
            if (a) v += gA * a[n];
            if (b) v += gB * b[n];
            out[c][n] = v;
        }
    }
}

}
