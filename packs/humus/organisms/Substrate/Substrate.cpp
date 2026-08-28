#include "Substrate/Substrate.h"

#include <algorithm>
#include <cmath>

namespace hum {

void Substrate::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    reset();
    for (int v = 0; v < kVoices; ++v)
        for (int k = 0; k < kMaxLayers; ++k) {
            auto& st = voices_[(size_t) v].strata[(size_t) k];
            st.pitchDrift.setRate(0.05 + std::fmod(0.023 * k, 0.14) + 0.003 * v, sampleRate);
            st.ampDrift.setRate(0.031 + std::fmod(0.017 * k, 0.1) + 0.002 * v, sampleRate);
        }
}

void Substrate::reset() {
    for (auto& v : voices_) {
        for (auto& st : v.strata) st.osc.reset();
        v.note = -1;
        v.gate = false;
        v.env = 0.0;
    }
    lpL_.reset();
    lpR_.reset();
    held_ = 0;
    midiRoot_ = -1;
    heldCount_ = 0;
    lastCount_ = 0;
    next_ = 0;
    intervalSm_ = -1.0;
}

void Substrate::process(const float* const*, int, float* const* out, int numOut,
                        int numSamples, const Transport& transport) {
    if (numOut == 0) return;
    float* L = out[0];
    float* R = out[numOut > 1 ? 1 : 0];

    const double sr = sampleRate_ > 0.0 ? sampleRate_ : 44100.0;

    {
        std::lock_guard<std::mutex> g(liveLock_);
        for (int i = 0; i < liveCount_; ++i)
            if (stagedCount_ < (int) staged_.size()) staged_[(size_t) stagedCount_++] = liveQ_[(size_t) i];
        liveCount_ = 0;
    }

    const bool drone = params.get("Drone", 1.0) >= 0.5;
    const int mode = drone ? 0 : std::clamp((int) params.get("Mode", 0.0), 0, 2);

    if (mode == 0) {
        for (int i = 0; i < stagedCount_; ++i) {
            const auto& e = staged_[(size_t) i];
            const int st = e.data[0] & 0xF0;
            if (st == 0x90 && e.data[2] > 0) { midiRoot_ = e.data[1]; ++held_; }
            else if (st == 0x80 || (st == 0x90 && e.data[2] == 0)) held_ = std::max(0, held_ - 1);
        }
    } else if (mode == 1) {
        for (int i = 0; i < stagedCount_; ++i) {
            const auto& e = staged_[(size_t) i];
            const int st = e.data[0] & 0xF0;
            const int n = e.data[1];
            if (st == 0x90 && e.data[2] > 0) {
                bool present = false;
                for (int j = 0; j < heldCount_; ++j) present |= heldNotes_[(size_t) j] == n;
                if (!present && heldCount_ < kVoices) heldNotes_[(size_t) heldCount_++] = n;
            } else if (st == 0x80 || (st == 0x90 && e.data[2] == 0)) {
                for (int j = 0; j < heldCount_; ++j)
                    if (heldNotes_[(size_t) j] == n) {
                        for (int m = j; m + 1 < heldCount_; ++m)
                            heldNotes_[(size_t) m] = heldNotes_[(size_t) (m + 1)];
                        --heldCount_;
                        break;
                    }
            }
        }
        if (heldCount_ > 0) {
            lastCount_ = heldCount_;
            for (int j = 0; j < heldCount_; ++j) lastNotes_[(size_t) j] = heldNotes_[(size_t) j];
        }
    } else {
        for (int i = 0; i < stagedCount_; ++i) {
            const auto& e = staged_[(size_t) i];
            const int st = e.data[0] & 0xF0;
            const int n = e.data[1];
            if (st == 0x90 && e.data[2] > 0) {
                int slot = -1;
                for (int v = 0; v < kVoices; ++v)
                    if (voices_[(size_t) v].gate && voices_[(size_t) v].note == n) { slot = v; break; }
                if (slot < 0)
                    for (int v = 0; v < kVoices; ++v)
                        if (voices_[(size_t) v].note < 0) { slot = v; break; }
                if (slot < 0) { slot = next_; next_ = (next_ + 1) % kVoices; }
                voices_[(size_t) slot].note = n;
                voices_[(size_t) slot].gate = true;
            } else if (st == 0x80 || (st == 0x90 && e.data[2] == 0)) {
                for (auto& v : voices_)
                    if (v.gate && v.note == n) { v.gate = false; break; }
            }
        }
    }
    stagedCount_ = 0;

    const int layers = std::clamp((int) params.get("Layers", 4.0), 1, kMaxLayers);

    const auto* stackP = params.byName("Stack");
    const int stack = stackP != nullptr ? (int) stackP->value
                    : params.byName("Interval") != nullptr ? 3 : 0;
    const double ratio = stack == 1 ? 1.5 : stack == 2 ? 1.2599 : 2.0;
    const double intervalTarget = std::clamp(params.get("Interval", 12.0), 1.0, 24.0);
    if (intervalSm_ < 0.0) intervalSm_ = intervalTarget;
    const double glide = 1.0 - std::exp(-(double) numSamples / (0.06 * sr));
    intervalSm_ += (intervalTarget - intervalSm_) * glide;
    if (std::abs(intervalTarget - intervalSm_) < 1.0e-3) intervalSm_ = intervalTarget;
    const double interval = intervalSm_;
    const bool custom = stack == 3;
    const double spread = params.get("Spread", 12.0);
    const double motion = params.get("Motion", 0.5);
    const double cutoff = params.get("Cutoff", 1400.0);
    const float level = (float) params.get("Level", 0.7);
    const double atk = 1.0 - std::exp(-1.0 / (std::max(1.0, params.get("Attack", 800.0)) * 0.001 * sr));
    const double rel = 1.0 - std::exp(-1.0 / (std::max(1.0, params.get("Release", 1200.0)) * 0.001 * sr));

    lpL_.setLowpass(sr, cutoff, 0.9);
    lpR_.setLowpass(sr, cutoff, 0.9);
    const float norm = 0.9f / (float) layers;

    if (mode != 2) {

        int M = 1;
        double roots[kVoices];
        double f0s[kVoices];
        double gateTarget;
        if (mode == 1 && lastCount_ > 0) {
            M = lastCount_;
            for (int j = 0; j < M; ++j) {
                roots[j] = (double) lastNotes_[(size_t) j];
                f0s[j] = transport.tuning().hz(roots[j]);
            }
            gateTarget = heldCount_ > 0 ? 1.0 : 0.0;
        } else {
            const double note = mode == 0 && midiRoot_ >= 0 ? midiRoot_ : params.get("Note", 36.0);
            roots[0] = note;
            f0s[0] = transport.tuning().hz(note);
            gateTarget = mode == 1 ? (heldCount_ > 0 ? 1.0 : 0.0)
                                   : (drone || held_ > 0 ? 1.0 : 0.0);
        }
        double hzTab[kVoices][kMaxStack];
        for (int j = 0; j < M; ++j)
            for (int r = 0; r < kMaxStack; ++r)
                hzTab[j][r] = custom
                    ? transport.tuning().hz(roots[j] + (double) (r * interval))
                    : f0s[j] * std::pow(ratio, (double) r);

        Voice& v0 = voices_[0];
        for (int i = 0; i < numSamples; ++i) {
            v0.env += (gateTarget - v0.env) * (gateTarget > v0.env ? atk : rel);
            float l = 0.0f, r = 0.0f;
            for (int k = 0; k < layers; ++k) {
                auto& st2 = v0.strata[(size_t) k];
                const double drift = Lfo::sine(st2.pitchDrift.tick()) * spread * motion;
                const double amp = 0.75 + 0.25 * Lfo::sine(st2.ampDrift.tick()) * motion;
                const double f = hzTab[k % M][(k / M) % kMaxStack]
                                 * std::pow(2.0, drift / 1200.0);
                const float s = st2.osc.saw(std::min(0.45, f / sr)) * (float) amp;
                const float pan = k == 0 ? 0.5f : (k & 1) ? 0.22f : 0.78f;
                l += s * (1.0f - pan);
                r += s * pan;
            }
            l = lpL_.process(l * norm);
            r = lpR_.process(r * norm);
            L[i] = std::tanh(l * 1.4f) * level * (float) v0.env;
            R[i] = std::tanh(r * 1.4f) * level * (float) v0.env;
        }
        return;
    }

    double hzTab[kVoices][kMaxStack];
    for (int v = 0; v < kVoices; ++v) {
        Voice& vv = voices_[(size_t) v];
        if (vv.note < 0) { vv.gate = false; vv.env = 0.0; continue; }
        const double f0 = transport.tuning().hz((double) vv.note);
        for (int r = 0; r < kMaxStack; ++r)
            hzTab[v][r] = custom
                ? transport.tuning().hz((double) vv.note + (double) (r * interval))
                : f0 * std::pow(ratio, (double) r);
    }

    for (int i = 0; i < numSamples; ++i) {
        float l = 0.0f, r = 0.0f;
        for (int v = 0; v < kVoices; ++v) {
            Voice& vv = voices_[(size_t) v];
            if (vv.note < 0) continue;
            const double g = vv.gate ? 1.0 : 0.0;
            vv.env += (g - vv.env) * (g > vv.env ? atk : rel);
            if (vv.env < 1.0e-4 && !vv.gate) continue;
            float vl = 0.0f, vr = 0.0f;
            for (int k = 0; k < layers; ++k) {
                auto& st2 = vv.strata[(size_t) k];
                const double drift = Lfo::sine(st2.pitchDrift.tick()) * spread * motion;
                const double amp = 0.75 + 0.25 * Lfo::sine(st2.ampDrift.tick()) * motion;
                const double f = hzTab[v][k % kMaxStack] * std::pow(2.0, drift / 1200.0);
                const float s = st2.osc.saw(std::min(0.45, f / sr)) * (float) amp;
                const float pan = k == 0 ? 0.5f : (k & 1) ? 0.22f : 0.78f;
                vl += s * (1.0f - pan);
                vr += s * pan;
            }
            l += vl * (float) vv.env;
            r += vr * (float) vv.env;
        }
        l = lpL_.process(l * norm);
        r = lpR_.process(r * norm);
        L[i] = std::tanh(l * 1.4f) * level;
        R[i] = std::tanh(r * 1.4f) * level;
    }
    for (auto& v : voices_)
        if (!v.gate && v.env < 1.0e-4) v.note = -1;
}

}
