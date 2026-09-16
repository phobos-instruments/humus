// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Console/Console.h"

#include <algorithm>
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
struct Flavor { double xtHz, stress, asym, drive, atkMs, relMs; };
constexpr Flavor kFlavors[4] = {
    {9000.0, 0.6, 0.02, 0.20, 3.0, 180.0},
    {5000.0, 1.3, 0.18, 0.55, 5.0, 260.0},
    {7000.0, 1.8, 0.08, 0.45, 2.0, 90.0},
    {11000.0, 0.35, 0.01, 0.12, 4.0, 150.0},
};

inline int flavorIndex(double v) {
    int i = (int) std::lround(v);
    return i < 0 ? 0 : i > 3 ? 3 : i;
}
}

namespace {
std::string gainSuffix(int k, int width) {
    if (width == 2) {
        const int lo = k * 2 + 1;
        return std::to_string(lo) + "-" + std::to_string(lo + 1);
    }
    return std::to_string(k + 1);
}
}

Console::Strip Console::stripParams(int k, int width) {
    const auto sfx = gainSuffix(k, width);
    return {ParamRef("Mute_" + sfx), ParamRef("Solo_" + sfx), ParamRef("Gain_" + sfx),
            ParamRef("Pan_" + sfx)};
}

void Console::process(const float* const* in, int numIn,
                      float* const* out, int numOut,
                      int numSamples, const Transport&) {
    meter_.measure(in, numIn, numSamples);
    for (int c = 0; c < numOut; ++c) std::fill(out[c], out[c] + numSamples, 0.0f);
    if (numOut < 1) return;

    outputS_.setTarget((float) params.get("Output", 1.0));
    const bool direct = params.get("Direct", 0.0) >= 0.5;

    bool anySolo = false;
    for (int k = 0; k < numInputs_; ++k)
        if (strips_[(size_t) k].solo.on(params)) anySolo = true;

    auto sumChannel = [&](int side, float* dst) {
        for (int k = 0; k < numInputs_; ++k) {
            const auto& strip = strips_[(size_t) k];
            const bool mute = strip.mute.on(params);
            const bool solo = strip.solo.on(params);
            if (mute || (anySolo && !solo)) continue;
            float g = (float) strip.gain.get(params, 1.0);

            const float p = (float) std::clamp(strip.pan.get(params, 0.5), 0.0, 1.0);
            g *= std::min(1.0f, side == 0 ? 2.0f * (1.0f - p) : 2.0f * p);
            const int ch = k * width_ + side;
            const float* src = ch < numIn ? in[ch] : nullptr;
            if (src == nullptr && width_ == 2 && side == 1) {
                const int other = k * 2;
                src = other < numIn ? in[other] : nullptr;
            }
            if (src == nullptr) continue;
            for (int n = 0; n < numSamples; ++n) dst[n] += g * src[n];
        }
    };
    sumChannel(0, out[0]);
    if (numOut > 1) sumChannel(1, out[1]);
    if (direct) {
        for (int n = 0; n < numSamples; ++n) {
            const float output = outputS_.next();
            for (int c = 0; c < numOut; ++c) out[c][n] *= output;
        }
        return;
    }

    const Flavor& f = kFlavors[flavorIndex(params.get("Flavor", 1.0))];
    driveS_.setTarget((float) (params.get("Drive", 0.3) * f.drive));
    xtalkS_.setTarget((float) (params.get("Crosstalk", 0.25) * 0.03));
    stressS_.setTarget((float) (params.get("Sag", 0.1) * f.stress * 6.0));

    const double xtA = 1.0 - std::exp(-2.0 * kPi * f.xtHz / sampleRate_);
    const double atk = smoothCoeff(f.atkMs, sampleRate_);
    const double rel = smoothCoeff(f.relMs, sampleRate_);
    const double dcR = 1.0 - (2.0 * kPi * 10.0 / sampleRate_);

    const bool stereo = numOut > 1;
    for (int n = 0; n < numSamples; ++n) {
        const double xtGain = xtalkS_.next();
        const double dAmt = driveS_.next();
        const double k = 1.0 + dAmt * 4.0;
        const double a = f.asym * dAmt;
        const double ta = std::tanh(a);
        const double makeup = 1.0 + dAmt * 0.4;
        const double stress = stressS_.next();
        const double output = outputS_.next();
        double bl = out[0][n];
        double br = stereo ? out[1][n] : bl;

        xtLpL_ += xtA * (bl - xtLpL_);
        xtLpR_ += xtA * (br - xtLpR_);
        const double hpL = bl - xtLpL_;
        const double hpR = br - xtLpR_;
        bl += xtGain * hpR;
        br += xtGain * hpL;

        const double demand = 0.5 * (std::fabs(bl) + std::fabs(br));
        const double cf = demand > sagEnv_ ? atk : rel;
        sagEnv_ = cf * sagEnv_ + (1.0 - cf) * demand;
        const double rail = 1.0 / (1.0 + sagEnv_ * stress);
        bl *= rail;
        br *= rail;

        double yl = bl + dAmt * ((std::tanh(k * bl + a) - ta) / k * makeup - bl);
        double yr = br + dAmt * ((std::tanh(k * br + a) - ta) / k * makeup - br);
        const double ol = yl - dcXL_ + dcR * dcYL_;
        dcXL_ = yl; dcYL_ = ol;
        const double orr = yr - dcXR_ + dcR * dcYR_;
        dcXR_ = yr; dcYR_ = orr;

        out[0][n] = (float) (ol * output);
        if (stereo) out[1][n] = (float) (orr * output);
    }
}

}
