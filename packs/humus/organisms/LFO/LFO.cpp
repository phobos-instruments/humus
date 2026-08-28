#include "LFO/LFO.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace hum {

void LfoGen::process(const float* const* in, int numIn, float* const* out, int numOut,
                     int numSamples, const Transport& transport) {
    if (numOut < 1) return;
    const double rate = std::clamp(params.get("Rate", 1.0), 0.01, 100.0);
    const int wave = std::clamp((int) params.get("Waveform", 0.0), 0, 5);
    const double amp = std::clamp(params.get("Amplitude", 1.0), 0.0, 1.0);
    const double off = std::clamp(params.get("Offset", 0.0), -1.0, 1.0);
    const bool sync = params.get("Sync", 0.0) >= 0.5;
    const double beats = std::max(0.0625, params.get("SyncBeats", 1.0));

    if (sync) lfo_.setPeriodSamples(std::max(1.0, transport.samplesPerBeat() * beats));
    else      lfo_.setRate(rate, sampleRate_);

    const float* src = (numIn > 0 && in && in[0]) ? in[0] : nullptr;
    float* dst = out[0];
    double lastV = 0.0;
    for (int i = 0; i < numSamples; ++i) {
        const double p = lfo_.tick();
        if (wave == 5 && p < prevPhase_) {
            rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5;
            held_ = (double) (rng_ & 0xFFFFFF) / (double) 0x7FFFFF - 1.0;
        }
        prevPhase_ = p;
        const double v = wave == 1 ? Lfo::triangle(p)
                       : wave == 2 ? Lfo::square(p)
                       : wave == 3 ? Lfo::sawUp(p)
                       : wave == 4 ? Lfo::sawDown(p)
                       : wave == 5 ? held_
                                   : Lfo::sine(p);
        dst[i] = (src ? src[i] : 0.0f) + (float) (off + amp * v);
        lastV = v;
    }
    for (int c = 1; c < numOut; ++c) std::memset(out[c], 0, sizeof(float) * (size_t) numSamples);
    if (numSamples > 0) {
        ctl_.store((float) std::clamp((lastV + 1.0) * 0.5, 0.0, 1.0), std::memory_order_relaxed);
        phase_.store((float) lfo_.phase(), std::memory_order_relaxed);
    }
}

}
