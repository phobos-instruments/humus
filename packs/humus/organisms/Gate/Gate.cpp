#include "Gate/Gate.h"

#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
enum Source { kSourceLevel = 0, kSourceMidi = 1, kSourceSwitch = 2 };
}

void Gate::process(const float* const* in, int numIn, float* const* out, int numOut,
                   int numSamples, const Transport&) {
    if (numOut < 1) return;
    const int source = (int) std::clamp(params.get("Source", 0.0), 0.0, 2.0);
    const bool rms = params.get("Detector", 0.0) >= 0.5;
    const double threshold = std::clamp(params.get("Threshold", 0.15), 0.0, 1.0);
    const double floorGain = std::clamp(params.get("Floor", 0.0), 0.0, 1.0);
    const bool duck = params.get("Duck", 0.0) >= 0.5;
    const bool manualOpen = params.get("Open", 0.0) >= 0.5;
    const double atkMs = std::max(0.01, params.get("AttackTime", 1.0));
    const double relMs = std::max(1.0, params.get("ReleaseTime", 100.0));
    const double holdMs = std::max(0.0, params.get("HoldTime", 10.0));

    const double openT = source == kSourceLevel ? threshold : 0.5;
    core_.set(openT, openT * 0.7, floorGain, duck, atkMs, relMs, holdMs, sampleRate_);
    const float atkCoef = 1.0f - (float) std::exp(-1.0 / (atkMs * 0.001 * sampleRate_));
    const float relCoef = 1.0f - (float) std::exp(-1.0 / (relMs * 0.001 * sampleRate_));
    const double rmsCoef = 1.0 - std::exp(-1.0 / (0.01 * sampleRate_));

    bool keyPatched = false;
    for (int c = ch_; c < 2 * ch_ && c < numIn; ++c)
        if (in && in[c]) keyPatched = true;
    const int detFirst = keyPatched ? ch_ : 0;

    int ev = 0;
    for (int i = 0; i < numSamples; ++i) {
        while (ev < stagedCount_ && staged_[(size_t) ev].sampleOffset <= i)
            held_.apply(staged_[(size_t) ev++]);
        double level = 0.0;
        if (source == kSourceMidi) {
            level = held_.any() ? 1.0 : 0.0;
        } else if (source == kSourceSwitch) {
            level = manualOpen ? 1.0 : 0.0;
        } else {
            const double x = linkedPeak(in, numIn, detFirst, ch_, i);
            rms2_ += (x * x - rms2_) * rmsCoef;
            level = rms ? std::sqrt(std::max(0.0, rms2_)) : x;
        }
        core_.process(level);
        const float target = core_.isOpen() ? 1.0f : 0.0f;
        openEnv_ += (target - openEnv_) * (target > openEnv_ ? atkCoef : relCoef);
        const float span = 1.0f - (float) floorGain;
        const float gain = duck ? 1.0f - openEnv_ * span
                                : (float) floorGain + openEnv_ * span;
        for (int c = 0; c < numOut; ++c) {
            const float x = (c < numIn && in && in[c]) ? in[c][i] : 0.0f;
            out[c][i] = x * gain;
        }
    }
    stagedCount_ = 0;
    ctl_.store(std::clamp(openEnv_, 0.0f, 1.0f), std::memory_order_relaxed);
}

}
