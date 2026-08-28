#include "Trellis/Trellis.h"

#include <algorithm>
#include <cmath>

namespace hum {

void Trellis::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    tracker_.prepare(sampleRate);
    shifter_.prepare(sampleRate);
    reset();
}

void Trellis::reset() {
    tracker_.reset();
    shifter_.reset();
    targetCents_ = 0.0;
    smoothCents_ = 0.0;
}

double Trellis::snapMidi(double midi, const Tuning& tuning) const {
    const Scale sc = Scale::byId(scale_);
    const int reach = std::max(3, tuning.notesPerPeriod() / 2 + 1);
    double best = midi;
    double bestDist = 1.0e9;
    const int lo = (int) std::floor(midi) - reach;
    const int hi = (int) std::ceil(midi) + reach;
    for (int n = lo; n <= hi; ++n) {
        if (!sc.allows(tuning, n, key_)) continue;
        const double dist = std::abs(midi - n);
        if (dist < bestDist) { bestDist = dist; best = n; }
    }
    return best;
}

void Trellis::process(const float* const* in, int numIn, float* const* out, int numOut,
                      int numSamples, const Transport& transport) {
    float* o = (numOut > 0 && out && out[0]) ? out[0] : nullptr;
    const float* x = (numIn > 0 && in && in[0]) ? in[0] : nullptr;
    if (!o) return;
    if (!x) { std::fill(o, o + numSamples, 0.0f); return; }

    const auto& tuning = transport.tuning();
    key_ = ((int) params.get("Key", 0.0)) % std::max(1, tuning.notesPerPeriod());
    scale_ = (int) params.get("Scale", 0.0);
    const double strength = std::clamp(params.get("Strength", 1.0), 0.0, 1.0);
    const double speed = std::clamp(params.get("Speed", 0.6), 0.0, 1.0);
    const double mix = std::clamp(params.get("Mix", 1.0), 0.0, 1.0);

    const double tau = 0.003 + (1.0 - speed) * 0.117;
    glideCoef_ = 1.0 - std::exp(-1.0 / (tau * sampleRate_));

    const int fresh = tracker_.push(x, numSamples);
    if (fresh > 0) {
        const double hz = tracker_.pitchHz();
        const bool voiced = hz >= 40.0 && hz <= 2000.0 && tracker_.clarity() > 0.6;
        if (voiced) {
            const double midi = tuning.midiNote(hz);
            const double target = snapMidi(midi, tuning);
            targetCents_ = 1200.0 * std::log2(tuning.hz(target) / hz) * strength;
        } else {
            targetCents_ = 0.0;
        }
    }

    for (int i = 0; i < numSamples; ++i) {
        smoothCents_ += glideCoef_ * (targetCents_ - smoothCents_);
        shifter_.setRatio(std::pow(2.0, smoothCents_ / 1200.0));
        const float wet = shifter_.process(x[i]);
        o[i] = (float) ((1.0 - mix) * x[i] + mix * wet);
    }
}

}
