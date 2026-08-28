#include "Dynamics/Dynamics.h"

#include <algorithm>
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
void copyThrough(const float* const* in, int numIn, float* const* out, int numOut, int n) {
    for (int c = 0; c < numOut; ++c)
        out[c][n] = (c < numIn && in[c]) ? in[c][n] : 0.0f;
}
}

void Compressor::process(const float* const* in, int numIn, float* const* out, int numOut,
                         int numSamples, const Transport& transport) {
    const bool bypass = params.get("Bypass", 0.0) >= 0.5;
    const float inGain = (float) params.get("InputGain", 1.0);
    double release = params.get("ReleaseTime", 100.0);
    if (params.get("Sync", 0.0) >= 0.5) {
        const double unit = transport.rhythmicUnitToSamples(params.getText("SyncUnit", "1/8"));
        const double mult = std::max(1.0, params.get("SyncMultiplier", 1.0));
        if (unit > 0.0 && sampleRate_ > 0.0)
            release = std::clamp(mult * unit * 1000.0 / sampleRate_, 1.0, 2000.0);
    }
    core_.set(linToDb(std::max(1e-6, params.get("Threshold", 0.5))),
              params.get("CompressionRatio", 2.0), params.get("KneeWidth", 6.0),
              params.get("AttackTime", 10.0), release,
              params.get("HoldTime", 0.0), sampleRate_);
    const double makeup = params.get("AutoMakeupGain", 0.0) >= 0.5
                              ? core_.autoMakeup()
                              : params.get("MakeupGain", 1.0);

    for (int n = 0; n < numSamples; ++n) {
        if (bypass) { copyThrough(in, numIn, out, numOut, n); continue; }
        const float g =
            (float) (core_.process(inGain * linkedPeak(in, numIn, 0, ch_, n)) * makeup);
        for (int c = 0; c < numOut; ++c)
            out[c][n] = ((c < numIn && in[c]) ? in[c][n] : 0.0f) * inGain * g;
    }
    gr_.store(bypass ? 0.0f : (float) std::min(60.0, -core_.reductionDb()), std::memory_order_relaxed);
}

void Limiter::process(const float* const* in, int numIn, float* const* out, int numOut,
                      int numSamples, const Transport&) {
    const bool bypass = params.get("Bypass", 0.0) >= 0.5;
    const float inGain = (float) params.get("InputGain", 1.0);
    const double thr = std::max(1e-6, params.get("Threshold", 0.5));
    const double ceil = std::max(1e-6, params.get("Ceiling", 0.95));
    core_.set(thr, params.get("ReleaseTime", 150.0), params.get("HoldTime", 0.0), sampleRate_);
    const double makeup = ceil / thr;

    for (int n = 0; n < numSamples; ++n) {
        if (bypass) { copyThrough(in, numIn, out, numOut, n); continue; }
        const double g = core_.process(inGain * linkedPeak(in, numIn, 0, ch_, n));
        for (int c = 0; c < numOut; ++c) {
            const double y = ((c < numIn && in[c]) ? in[c][n] : 0.0f) * inGain * g * makeup;
            out[c][n] = (float) std::clamp(y, -ceil, ceil);
        }
    }
    gr_.store(bypass ? 0.0f : (float) std::min(60.0, -core_.reductionDb()), std::memory_order_relaxed);
}

void NoiseGate::process(const float* const* in, int numIn, float* const* out, int numOut,
                        int numSamples, const Transport&) {
    const bool bypass = params.get("Bypass", 0.0) >= 0.5;
    const bool duck = params.get("Mode", 0.0) >= 0.5;
    const float inGain = (float) params.get("InputGain", 1.0);
    const double closeT = params.get("Threshold", 0.1);
    const double openT = std::max(closeT, params.getMax("Threshold", 0.2));
    const double floorG = std::clamp(params.get("Range", 0.0), 0.0, 1.0);
    core_.set(openT, closeT, floorG, duck, params.get("AttackTime", 1.0),
              params.get("ReleaseTime", 100.0), params.get("HoldTime", 0.0), sampleRate_);

    for (int n = 0; n < numSamples; ++n) {
        if (bypass) { copyThrough(in, numIn, out, numOut, n); continue; }
        const float g = (float) core_.process(inGain * linkedPeak(in, numIn, 0, ch_, n));
        for (int ch = 0; ch < numOut; ++ch)
            out[ch][n] = ((ch < numIn && in[ch]) ? in[ch][n] : 0.0f) * inGain * g;
    }
    gr_.store(bypass ? 0.0f : (float) std::min(60.0, -core_.reductionDb()),
              std::memory_order_relaxed);
}

}
