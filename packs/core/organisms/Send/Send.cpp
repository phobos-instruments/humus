#include "Send/Send.h"

namespace hum {

void Send::process(const float* const* in, int numIn,
                   float* const* out, int numOut,
                   int numSamples, const Transport&) {
    const float level = (float) params.get("Level", 1.0);
    const float* l = numIn > 0 ? in[0] : nullptr;
    const float* r = numIn > 1 && in[1] != nullptr ? in[1] : l;
    for (int n = 0; n < numSamples; ++n) {
        const float sl = l ? l[n] : 0.0f;
        const float sr = r ? r[n] : 0.0f;
        if (numOut > 0) out[0][n] = sl;
        if (numOut > 1) out[1][n] = sr;
        if (numOut > 2) out[2][n] = sl * level;
        if (numOut > 3) out[3][n] = sr * level;
    }
}

}
