// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: MIT
#include "PingPong.h"

#include <algorithm>

namespace hum {

void PingPong::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    const int maxSamples = (int) (2.0 * sampleRate) + 4;
    left_.prepare(maxSamples);
    right_.prepare(maxSamples);
}

void PingPong::process(const float* const* in, int numIn,
                       float* const* out, int numOut,
                       int numSamples, const Transport& transport) {
    const double timeMs = std::clamp(params.get("TimeMs", 350.0), 1.0, 2000.0);
    const float fb = (float) std::clamp(params.get("Feedback", 0.4), 0.0, 0.95);
    const float wet = (float) std::clamp(params.get("WetDry", 0.35), 0.0, 1.0);
    const bool sync = params.get("Sync", 0.0) >= 0.5;

    const double delaySamples = std::clamp(
        sync ? transport.rhythmicUnitToSamples(params.getText("SyncUnit", "1/8"))
             : timeMs * 0.001 * sampleRate_,
        1.0, 2.0 * sampleRate_);

    for (int n = 0; n < numSamples; ++n) {
        const float inL = (numIn > 0 && in && in[0]) ? in[0][n] : 0.0f;
        const float inR = (numIn > 1 && in && in[1]) ? in[1][n] : inL;

        const float dL = left_.read((float) delaySamples);
        const float dR = right_.read((float) delaySamples);

        left_.write(inL + fb * dR);
        right_.write(inR + fb * dL);

        if (numOut > 0) out[0][n] = inL * (1.0f - wet) + dL * wet;
        if (numOut > 1) out[1][n] = inR * (1.0f - wet) + dR * wet;
        for (int c = 2; c < numOut; ++c) out[c][n] = 0.0f;
    }
}

}
