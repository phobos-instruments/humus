#include "StereoTool/StereoTool.h"

#include <algorithm>
#include <cmath>

namespace hum {

void StereoTool::process(const float* const* in, int numIn, float* const* out, int numOut,
                         int numSamples, const Transport&) {
    if (numOut < 2) {
        for (int c = 0; c < numOut; ++c) std::fill(out[c], out[c] + numSamples, 0.0f);
        return;
    }
    const float width   = (float) std::clamp(params.get("Width", 1.0), 0.0, 4.0);
    const float balance = (float) std::clamp(params.get("Balance", 0.0), -1.0, 1.0);
    const double rotDeg  = std::clamp(params.get("Rotation", 0.0), -45.0, 45.0);
    const double monoHz  = std::clamp(params.get("MonoBass", 0.0), 0.0, 500.0);
    const bool phaseL = params.get("PhaseL", 0.0) >= 0.5;
    const bool phaseR = params.get("PhaseR", 0.0) >= 0.5;
    const bool swap   = params.get("Swap", 0.0) >= 0.5;
    const bool mono   = params.get("Mono", 0.0) >= 0.5;
    const float outGain = (float) std::clamp(params.get("Output", 1.0), 0.0, 4.0);

    const float gL = balance > 0.0f ? 1.0f - balance : 1.0f;
    const float gR = balance < 0.0f ? 1.0f + balance : 1.0f;
    const double rot = rotDeg * M_PI / 180.0;
    const float cr = (float) std::cos(rot), sr = (float) std::sin(rot);
    const bool monoBass = monoHz > 0.0;
    if (monoBass && monoHz != monoBassHz_) {
        sideHp1_.setHighpass(sampleRate_, monoHz, 0.707);
        sideHp2_.setHighpass(sampleRate_, monoHz, 0.707);
        monoBassHz_ = monoHz;
    }

    const float* inL = (numIn > 0 && in[0]) ? in[0] : nullptr;
    const float* inR = (numIn > 1 && in[1]) ? in[1] : nullptr;
    float* oL = out[0];
    float* oR = out[1];

    for (int i = 0; i < numSamples; ++i) {
        float L = inL ? inL[i] : 0.0f;
        float R = inR ? inR[i] : 0.0f;
        if (phaseL) L = -L;
        if (phaseR) R = -R;
        if (swap) std::swap(L, R);

        float mid  = 0.5f * (L + R);
        float side = 0.5f * (L - R);
        side *= width;
        if (monoBass) side = sideHp2_.process(sideHp1_.process(side));

        L = mid + side;
        R = mid - side;

        const float Lr = L * cr - R * sr;
        const float Rr = L * sr + R * cr;
        L = Lr * gL;
        R = Rr * gR;

        if (mono) { const float m = 0.5f * (L + R); L = R = m; }

        oL[i] = L * outGain;
        oR[i] = R * outGain;
    }
    for (int c = 2; c < numOut; ++c) std::fill(out[c], out[c] + numSamples, 0.0f);

    {
        double sll = 0.0, srr = 0.0, slr = 0.0;
        unsigned w = widx_.load(std::memory_order_relaxed);
        for (int i = 0; i < numSamples; ++i) {
            const float L = oL[i], R = oR[i];
            ring_[2 * w] = L;
            ring_[2 * w + 1] = R;
            w = (w + 1) % (unsigned) kFieldPairs;
            sll += (double) L * L;
            srr += (double) R * R;
            slr += (double) L * R;
        }
        widx_.store(w, std::memory_order_relaxed);
        const double denom = std::sqrt(sll * srr);
        if (denom > 1e-12) {
            corr_.store((float) (slr / denom), std::memory_order_relaxed);
            stamp_.fetch_add(1, std::memory_order_relaxed);
        }
    }
}

}
