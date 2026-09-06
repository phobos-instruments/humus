#include "Spark.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "hum/dsp/DspMath.h"

namespace hum {
namespace {

constexpr double kMaxTimeMs = 8.0, kMaxRange = 5.0;

}

void Spark::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate > 0.0 ? sampleRate : kDefaultSampleRate;
    const int maxSamples =
        (int) std::ceil(kMaxTimeMs * std::exp2(kMaxRange) * 0.001 * sampleRate_) + 8;
    std::uint32_t rng = 0x5A2C0FFu;
    for (int i = 0; i < kLines; ++i) {
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        offset_[i] = (float) (rng & 0xFFFFFF) / (float) 0xFFFFFF - 0.5f;
        for (auto& ch : lines_) ch[i].delay.prepare(maxSamples);
    }
    reset();
}

void Spark::reset() {
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < kLines; ++i) {
            lines_[c][i].delay.clear();
            lines_[c][i].damp = 0.0f;
            lines_[c][i].phase = (double) i / kLines;
        }
}

void Spark::process(const float* const* in, int numIn, float* const* out, int numOut,
                    int numSamples, const Transport&) {
    const int mode = (int) std::lround(params.get("Mode", 0.0));
    const double rate = std::clamp(params.get("Rate", 0.12), 0.01, 2.0);
    const double timeMs = std::clamp(params.get("Time", 1.5), 0.5, kMaxTimeMs);
    const double range = std::clamp(params.get("Range", 4.0), 1.0, kMaxRange);
    const float fb = (float) std::clamp(params.get("Feedback", 0.7), 0.0, 0.95);
    const float damp = (float) std::clamp(params.get("Damp", 0.35), 0.0, 1.0);
    const float spark = (float) std::clamp(params.get("Spark", 0.35), 0.0, 1.0);
    const double spread = std::clamp(params.get("Spread", 0.5), 0.0, 1.0);
    const float mix = (float) std::clamp(params.get("Mix", 0.5), 0.0, 1.0);
    const float level = (float) std::clamp(params.get("Level", 1.0), 0.0, 2.0);

    const double dir = mode == 1 ? -1.0 : 1.0;
    const double inc = dir * rate / sampleRate_;
    const float a = 1.0f - damp * 0.97f;
    const double longest = timeMs * std::exp2(range) * 0.001 * sampleRate_;
    const float norm = 1.0f / (kLines * 0.5f);

    for (int c = 0; c < numOut; ++c) {
        const float* src = c < numIn && in[c] != nullptr ? in[c] : nullptr;
        float* dst = out[c];
        auto& ch = lines_[std::min(c, 1)];
        const double chOff = c == 1 ? spread * 0.5 / kLines : 0.0;

        for (int n = 0; n < numSamples; ++n) {
            const float x = src != nullptr ? src[n] : 0.0f;
            float wet = 0.0f;
            for (int i = 0; i < kLines; ++i) {
                auto& L = ch[i];
                L.phase += inc;
                L.phase -= std::floor(L.phase);
                double p = L.phase + chOff + (double) (spark * offset_[i]);
                p -= std::floor(p);
                const double sweep = mode == 2 ? 0.5 - 0.5 * std::cos(kTwoPi * p) : p;
                const float d = (float) (longest * std::exp2(-range * sweep));
                const float y = L.delay.read(d);
                L.damp += a * (y - L.damp);
                L.delay.write(x + fb * L.damp);
                wet += (float) (0.5 - 0.5 * std::cos(kTwoPi * p)) * y;
            }
            dst[n] = level * ((1.0f - mix) * x + mix * wet * norm);
        }
    }
}

}
