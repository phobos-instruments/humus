#include "Crossover/Crossover.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
constexpr double kQButter = kSqrtHalf;
constexpr double kQ8a = 0.54119610014620261, kQ8b = 1.3065629648763764;
constexpr double kDefaults[4][4] = {
    {1000, 0, 0, 0}, {300, 3000, 0, 0}, {150, 800, 5000, 0}, {100, 500, 2000, 8000}};
}

Crossover::SlopeSpec Crossover::specFor(int slope) {
    if (slope == 1) return {1, {0.5}, 1, {-1.0}, true};
    if (slope == 2) return {4, {kQ8a, kQ8b, kQ8a, kQ8b}, 2, {kQ8a, kQ8b}, false};
    return {2, {kQButter, kQButter}, 1, {kQButter}, false};
}

Crossover::Crossover(int bands) : bands_(std::clamp(bands, 2, kMaxBands)) {
    for (int i = 0; i < bands_ - 1; ++i) def_[(size_t) i] = kDefaults[bands_ - 2][i];
}

void Crossover::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    reset();
}

void Crossover::reset() {
    for (auto& s : splits_)
        for (int st = 0; st < 4; ++st)
            for (int c = 0; c < 2; ++c) { s.lp[st][c].reset(); s.hp[st][c].reset(); }
    for (auto& byBand : ap_)
        for (auto& bySplit : byBand)
            for (auto& byComp : bySplit)
                for (auto& b : byComp) b.reset();
    lastSlope_ = -1;
}

void Crossover::design(const double* f, const SlopeSpec& sp) {
    for (int i = 0; i < bands_ - 1; ++i) {
        auto& s = splits_[(size_t) i];
        for (int c = 0; c < 2; ++c) {
            for (int st = 0; st < sp.stages; ++st) {
                s.lp[st][c].setLowpass(sampleRate_, f[i], sp.q[st]);
                s.hp[st][c].setHighpass(sampleRate_, f[i], sp.q[st]);
            }
            for (int j = i + 1; j < bands_ - 1; ++j)
                for (int k = 0; k < sp.comps; ++k) {
                    if (sp.cq[k] < 0.0) ap_[i][j][k][c].setAllpass1(sampleRate_, f[j]);
                    else ap_[i][j][k][c].setAllpass(sampleRate_, f[j], sp.cq[k]);
                }
        }
    }
}

void Crossover::process(const float* const* in, int numIn, float* const* out, int numOut,
                        int numSamples, const Transport&) {
    for (int c = 0; c < numOut; ++c) std::fill(out[c], out[c] + numSamples, 0.0f);

    const int slope = std::clamp((int) std::lround(params.get("Slope", 0.0)), 0, 2);
    const auto sp = specFor(slope);
    double f[kMaxBands - 1] = {};
    double prev = 20.0;
    bool moved = slope != lastSlope_;
    for (int i = 0; i < bands_ - 1; ++i) {
        const double raw = params.get("Freq" + std::to_string(i + 1), def_[(size_t) i]);
        f[i] = std::clamp(raw, prev * 1.02, sampleRate_ * 0.45);
        prev = f[i];
        moved = moved || std::abs(f[i] - lastF_[(size_t) i]) > 1e-6;
        lastF_[(size_t) i] = f[i];
    }
    if (moved) {
        if (slope != lastSlope_) reset();
        design(f, sp);
        lastSlope_ = slope;
    }

    for (int n = 0; n < numSamples; ++n) {
        const float inL = (numIn > 0 && in && in[0]) ? in[0][n] : 0.0f;
        const float inR = (numIn > 1 && in && in[1]) ? in[1][n] : inL;
        float w[2] = { inL, inR };
        for (int i = 0; i < bands_ - 1; ++i) {
            auto& s = splits_[(size_t) i];
            for (int c = 0; c < 2; ++c) {
                float lo = w[c], hi = w[c];
                for (int st = 0; st < sp.stages; ++st) {
                    lo = s.lp[st][c].process(lo);
                    hi = s.hp[st][c].process(hi);
                }
                if (sp.invertHi) hi = -hi;
                for (int j = i + 1; j < bands_ - 1; ++j)
                    for (int k = 0; k < sp.comps; ++k) lo = ap_[i][j][k][c].process(lo);
                if (numOut > i * 2 + c) out[i * 2 + c][n] = lo;
                w[c] = hi;
            }
        }
        for (int c = 0; c < 2; ++c)
            if (numOut > (bands_ - 1) * 2 + c) out[(bands_ - 1) * 2 + c][n] = w[c];
    }
}

}
