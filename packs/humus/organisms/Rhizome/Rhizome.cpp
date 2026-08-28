#include "Rhizome/Rhizome.h"

#include <algorithm>
#include <cmath>

namespace hum {

namespace {
constexpr double kTwoPi = 6.283185307179586;
}

void Rhizome::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    reset();
    for (auto& v : voices_)
        for (int k = 0; k < kMaxRunners; ++k)
            v.runners[(size_t) k].creep.setRate(0.037 + 0.018 * k, sampleRate);
}

void Rhizome::reset() {
    for (auto& v : voices_) {
        v.note = -1;
        v.gate = false;
        v.env = 0.0;
        for (auto& r : v.runners) r.osc.reset();
    }
    next_ = 0;
    lpL_.reset();
    lpR_.reset();
}

void Rhizome::noteOn(int note, int vel) {
    int slot = -1;
    for (int i = 0; i < kVoices; ++i)
        if (voices_[(size_t) i].note < 0) { slot = i; break; }
    if (slot < 0) { slot = next_; next_ = (next_ + 1) % kVoices; }

    Voice& v = voices_[(size_t) slot];
    v.note = note;
    v.vel = (float) vel / 127.0f;
    v.gate = true;
}

void Rhizome::noteOff(int note) {
    for (auto& v : voices_)
        if (v.note == note && v.gate) v.gate = false;
}

void Rhizome::process(const float* const*, int, float* const* out, int numOut,
                      int numSamples, const Transport& transport) {
    if (numOut == 0) return;
    float* L = out[0];
    float* R = out[numOut > 1 ? 1 : 0];
    std::fill(L, L + numSamples, 0.0f);
    if (R != L) std::fill(R, R + numSamples, 0.0f);

    const double sr = sampleRate_ > 0.0 ? sampleRate_ : 44100.0;

    {
        std::lock_guard<std::mutex> g(liveLock_);
        for (int i = 0; i < liveCount_; ++i)
            if (stagedCount_ < (int) staged_.size()) staged_[(size_t) stagedCount_++] = liveQ_[(size_t) i];
        liveCount_ = 0;
    }
    for (int i = 0; i < stagedCount_; ++i) {
        const auto& e = staged_[(size_t) i];
        const int st = e.data[0] & 0xF0;
        if (st == 0x90 && e.data[2] > 0) noteOn(e.data[1], e.data[2]);
        else if (st == 0x80 || (st == 0x90 && e.data[2] == 0)) noteOff(e.data[1]);
    }
    stagedCount_ = 0;

    const int runners = std::clamp((int) std::lround(params.get("Runners", 4.0)), 1, kMaxRunners);
    const int interval = std::clamp((int) std::lround(params.get("Interval", 7.0)), 1, 24);
    const double creep = params.get("Creep", 8.0);
    const double tilt = std::clamp(params.get("Tilt", 0.6), 0.05, 1.0);
    const int wave = std::clamp((int) std::lround(params.get("Wave", 0.0)), 0, 2);
    const double bloom = std::max(1.0, params.get("Bloom", 300.0));
    const double decay = std::max(20.0, params.get("Decay", 900.0));
    const double cutoff = std::clamp(params.get("Cutoff", 2200.0), 40.0, 0.45 * sr);
    const float level = (float) params.get("Level", 0.8);

    const double atk = 1.0 - std::exp(-1.0 / (bloom * 0.001 * sr));
    const double rel = 1.0 - std::exp(-1.0 / (decay * 0.001 * sr));

    lpL_.setLowpass(sr, cutoff, 0.8);
    lpR_.setLowpass(sr, cutoff, 0.8);

    const auto& tuning = transport.tuning();
    for (auto& v : voices_) {
        if (v.note < 0) continue;
        for (int k = 0; k < runners; ++k)
            v.hz[(size_t) k] = tuning.hz((double) v.note + (double) (k * interval));
    }

    std::array<float, kMaxRunners> gain{};
    double gsum = 0.0;
    for (int k = 0; k < runners; ++k) { gain[(size_t) k] = (float) std::pow(tilt, k); gsum += gain[(size_t) k]; }
    const float gnorm = gsum > 0.0 ? (float) (0.9 / gsum) : 0.0f;

    for (auto& v : voices_) {
        if (v.note < 0) continue;
        for (int i = 0; i < numSamples; ++i) {
            v.env += (v.gate ? atk * (1.0 - v.env) : -rel * v.env);
            const float e = (float) v.env * v.vel;
            float sL = 0.0f, sR = 0.0f;
            for (int k = 0; k < runners; ++k) {
                auto& r = v.runners[(size_t) k];
                const double det = std::pow(2.0, (creep * Lfo::sine(r.creep.tick())) / 1200.0);
                const double dt = std::clamp(v.hz[(size_t) k] * det / sr, 0.0, 0.49);
                float s;
                if (wave == 0) {
                    s = (float) std::sin(kTwoPi * r.osc.phase);
                    r.osc.phase += dt;
                    if (r.osc.phase >= 1.0) r.osc.phase -= 1.0;
                } else {
                    s = wave == 1 ? r.osc.saw(dt) : r.osc.square(dt);
                }
                const float g = gain[(size_t) k] * gnorm * e;
                if (k % 2 == 0) { sL += s * g * 0.9f; sR += s * g * 0.5f; }
                else            { sL += s * g * 0.5f; sR += s * g * 0.9f; }
            }
            if (R != L) { L[i] += sL; R[i] += sR; }
            else        { L[i] += (sL + sR) * 0.5f; }
        }
        if (!v.gate && v.env < 1.0e-4) { v.note = -1; v.env = 0.0; }
    }

    for (int i = 0; i < numSamples; ++i) {
        L[i] = lpL_.process(L[i]) * level;
        if (R != L) R[i] = lpR_.process(R[i]) * level;
    }
}

}
