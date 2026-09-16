// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Siren/Siren.h"

#include <algorithm>
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {

constexpr double kAttackSeconds = 0.003;
constexpr double kSweepSlewSeconds = 0.002;
constexpr double kPulseDuty = 0.25;
constexpr double kTrim = 0.28;

double blep(double t, double dt) {
    if (t < dt) {
        t /= dt;
        return t + t - t * t - 1.0;
    }
    if (t > 1.0 - dt) {
        t = (t - 1.0) / dt;
        return t * t + t + t + 1.0;
    }
    return 0.0;
}

double blepSaw(double phase, double inc) {
    return 2.0 * phase - 1.0 - blep(phase, inc);
}

double wrapped(double phase) {
    return phase >= 1.0 ? phase - 1.0 : phase;
}

}

double Siren::advanceSweep(int mode, double up, double down, double dt) {
    const double upStep = dt / std::max(0.001, up);
    const double downStep = dt / std::max(0.001, down);
    seg_ += rising_ ? upStep : downStep;
    if (seg_ >= 1.0) {
        seg_ -= 1.0;
        rising_ = !rising_;
    }
    switch (mode) {
        case Rise: return rising_ ? seg_ : 0.0;
        case Fall: return rising_ ? 1.0 : 1.0 - seg_;
        case Step: return rising_ ? 1.0 : 0.0;
        default:   return rising_ ? seg_ : 1.0 - seg_;
    }
}

double Siren::oscillate(int wave, double inc) {
    phase_ = wrapped(phase_ + inc);
    switch (wave) {
        case Saw:   return blepSaw(phase_, inc);
        case Pulse: return blepSaw(phase_, inc) - blepSaw(wrapped(phase_ + kPulseDuty), inc)
                         + (2.0 * kPulseDuty - 1.0);
        default:    return blepSaw(phase_, inc) - blepSaw(wrapped(phase_ + 0.5), inc);
    }
}

void Siren::process(const float* const*, int, float* const* out, int numOut,
                    int numSamples, const Transport&) {
    if (numOut == 0) return;
    float* o = out[0];
    for (int c = 1; c < numOut; ++c) std::fill(out[c], out[c] + numSamples, 0.0f);

    const double sr = sampleRate_ > 0.0 ? sampleRate_ : kDefaultSampleRate;
    const double dt = 1.0 / sr;
    const double low = std::clamp(params.get("PitchLow", 180.0), 20.0, sr * 0.4);
    const double high = std::clamp(params.get("PitchHigh", 1400.0), 20.0, sr * 0.4);
    const double up = params.get("LfoUp", 0.6);
    const double down = params.get("LfoDown", 0.6);
    const int mode = std::clamp((int) params.get("Mode", 0.0), 0, 3);
    const int wave = std::clamp((int) params.get("Wave", 0.0), 0, 2);
    const double tone = std::clamp(params.get("Tone", 4000.0), 100.0, sr * 0.45);
    const bool autoSweep = params.get("Auto", 1.0) >= 0.5;
    const double manual = std::clamp(params.get("Manual", 0.5), 0.0, 1.0);
    const double release = std::max(1.0, params.get("Release", 40.0)) * 0.001;
    const float level = (float) params.get("Level", 0.8);

    int noteEdge[64];
    int noteDelta[64];
    int nEdges = 0;
    for (int i = 0; i < stagedCount_ && nEdges < 64; ++i) {
        const auto& e = staged_[(size_t) i];
        const int status = e.data[0] & 0xF0;
        if (status == 0x90 && e.data[2] > 0) { noteEdge[nEdges] = e.sampleOffset; noteDelta[nEdges++] = 1; }
        else if (status == 0x80 || (status == 0x90 && e.data[2] == 0)) { noteEdge[nEdges] = e.sampleOffset; noteDelta[nEdges++] = -1; }
        else if (status == 0xB0 && (e.data[1] == 123 || e.data[1] == 120)) { noteEdge[nEdges] = e.sampleOffset; noteDelta[nEdges++] = -1000; }
    }
    stagedCount_ = 0;

    const bool holdParam = params.get("Hold", 0.0) >= 0.5;
    const double attackCoef = 1.0 - std::exp(-dt / kAttackSeconds);
    const double releaseCoef = 1.0 - std::exp(-dt / release);
    const double slewCoef = 1.0 - std::exp(-dt / kSweepSlewSeconds);
    const double toneCoef = 1.0 - std::exp(-kTwoPi * tone * dt);
    const double ratio = high / low;

    int ei = 0;
    for (int i = 0; i < numSamples; ++i) {
        while (ei < nEdges && noteEdge[ei] <= i) {
            heldNotes_ = noteDelta[ei] <= -1000 ? 0 : std::max(0, heldNotes_ + noteDelta[ei]);
            ++ei;
        }
        const bool gated = holdParam || heldNotes_ > 0;
        if (gated && !wasGated_) {
            rising_ = mode != Fall;
            seg_ = 0.0;
            sweep_ = mode == Fall || mode == Step ? 1.0 : 0.0;
            sweepSm_ = sweep_;
        }
        wasGated_ = gated;

        if (gated) sweep_ = autoSweep ? advanceSweep(mode, up, down, dt) : manual;
        sweepSm_ += (sweep_ - sweepSm_) * slewCoef;
        const double freq = low * std::pow(ratio, sweepSm_);
        const double raw = oscillate(wave, freq * dt);
        lp_ += (raw - lp_) * toneCoef;
        env_ += gated ? (1.0 - env_) * attackCoef : -env_ * releaseCoef;
        o[i] = (float) (lp_ * env_ * kTrim) * level;
    }
    lfo_.store((float) sweepSm_, std::memory_order_relaxed);
    gate_.store(wasGated_ ? 1.0f : 0.0f, std::memory_order_relaxed);
}

}
