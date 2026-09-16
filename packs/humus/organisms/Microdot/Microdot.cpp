// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Microdot/Microdot.h"

#include <algorithm>
#include <cmath>

#include "hum/Swing.h"

#include "hum/dsp/DspMath.h"

namespace hum {

void Microdot::process(const float* const*, int, float* const* out, int numOut,
                       int numSamples, const Transport& transport) {
    if (numOut == 0) return;
    float* o = out[0];
    for (int c = 1; c < numOut; ++c) std::fill(out[c], out[c] + numSamples, 0.0f);

    if (stepsDirty_) {
        steps_.clear();
        ups_.clear();
        for (const auto& ch : pattern_.channels) {
            if (ch.type == "trigger-tie-matrix" && steps_.empty()) steps_ = decodeArp(ch.matrix);
            if (ch.type == "octave-row" && ups_.empty()) ups_ = decodeOctaveRow(ch.matrix);
        }
        stepsPerBeat_ = steps_.empty() ? 4.0
                                       : (double) Pattern::kTicksPerBeat
                                             / (double) stepTicksFor(pattern_.matrixResolution);
        if (steps_.empty()) {
            steps_.resize(16);
            for (size_t i = 0; i < steps_.size(); ++i) steps_[i].trigger = (i % 4) != 0;
        }
        stepsDirty_ = false;
    }

    const double sr = sampleRate_ > 0.0 ? sampleRate_ : kDefaultSampleRate;
    const int wave = std::clamp((int) std::lround(params.get("Wave", 0.0)), 0, 5);
    const int note = (int) params.get("Note", 33.0);
    const double cutoff = params.get("Cutoff", 700.0);
    const double envMod = params.get("EnvMod", 0.5);
    const double decay = std::max(10.0, params.get("Decay", 120.0)) * 0.001 * sr;
    const double resoQ = 0.707 * std::pow(10.0, std::clamp(params.get("Reso", 0.3), 0.0, 1.0) * 1.15);
    const double gate = params.get("Gate", 0.8);
    const float subMix = (float) params.get("Sub", 0.4);
    const float drive = (float) params.get("Drive", 0.35);
    const float level = (float) params.get("Level", 0.9);

    const double stepsPerSec = transport.tempo() / kSecondsPerMinute * stepsPerBeat_;
    const long stepLen = (long) (sr / stepsPerSec);

    const bool seqOn = params.get("Seq", 1.0) >= 0.5;
    struct Trig { int at; int note; int lift; long glen; };
    Trig trig[32];
    int nT = 0;
    std::sort(staged_.begin(), staged_.begin() + stagedCount_,
              [](const MidiEvent& a, const MidiEvent& b) { return a.sampleOffset < b.sampleOffset; });
    if (!transport.playing()) {
        for (int i = 0; i < stagedCount_ && nT < 32; ++i) {
            const auto& e = staged_[(size_t) i];
            if ((e.data[0] & 0xF0) == 0x90 && e.data[2] > 0)
                trig[nT++] = {std::clamp(e.sampleOffset, 0, numSamples - 1),
                              seqOn ? note : (int) e.data[1], 0, (long) (gate * (double) stepLen)};
        }
    }
    if (transport.playing() && !steps_.empty()) {
        const double step0 = transport.beats() * stepsPerBeat_;
        const double ticksPerStep = (double) Pattern::kTicksPerBeat / stepsPerBeat_;
        const auto groove = swing::resolve(params, transport, (int) std::lround(ticksPerStep));
        double next = std::ceil(step0 - 1e-9) - 1.0;
        while (nT < 32) {
            const double pos = next + swing::delaySteps(next, ticksPerStep, groove);
            const double off = (pos - step0) / stepsPerSec * sr;
            if (off >= numSamples) break;
            if (off < 0.0 || next < 0.0) { next += 1.0; continue; }
            const size_t idx = (size_t) (((long) std::llround(next)) % (long) steps_.size());
            if (steps_[idx].trigger) {
                long glen = (long) (gate * (double) stepLen);
                for (size_t t = 1; t < steps_.size(); ++t) {
                    if (!steps_[(idx + t) % steps_.size()].tie) break;
                    glen = (long) t * stepLen + (long) (gate * (double) stepLen);
                }
                const int lift = idx < ups_.size() && ups_[idx] ? 12 : 0;
                trig[nT++] = {(int) std::max(0.0, off), seqOn ? note : -1, lift, glen};
            }
            next += 1.0;
        }
    }
    std::sort(trig, trig + nT, [](const Trig& a, const Trig& b) { return a.at < b.at; });

    if (t_ < 0 && nT == 0) {
        for (int i = 0; i < stagedCount_; ++i) held_.apply(staged_[(size_t) i]);
        stagedCount_ = 0;
        std::fill(o, o + numSamples, 0.0f);
        return;
    }

    lp_.setLowpass(sr, cutoff, resoQ);
    double dt = transport.tuning().hz(curNote_) / sr;
    int ti = 0;
    int ctrl = 0;
    const double ampDecay = decay * 5.0;
    int mi = 0;
    for (int i = 0; i < numSamples; ++i) {
        while (mi < stagedCount_ && staged_[(size_t) mi].sampleOffset <= i)
            held_.apply(staged_[(size_t) mi++]);
        while (ti < nT && trig[ti].at <= i) {
            const int n = trig[ti].note >= 0 ? trig[ti].note : held_.top();
            if (trig[ti].at == i && n >= 0) {
                t_ = 0;
                gateLen_ = trig[ti].glen;
                curNote_ = n + trig[ti].lift;
                dt = transport.tuning().hz(curNote_) / sr;
                osc_.reset();
                osc2_.reset(0.31);
                sub_.reset();
            }
            ++ti;
        }
        float v = 0.0f;
        if (t_ >= 0) {
            const double fenv = std::exp(-(double) t_ / decay);
            if ((ctrl++ & 15) == 0) {
                const double hz = std::min(sr * 0.45, cutoff * std::pow(2.0, envMod * 4.0 * fenv));
                lp_.setLowpass(sr, hz, resoQ);
            }
            double amp = std::exp(-(double) t_ / ampDecay);
            if (t_ < 32) amp *= (double) t_ / 32.0;
            if (t_ > gateLen_) amp *= std::exp(-(double) (t_ - gateLen_) / (0.008 * sr));
            float m;
            switch (wave) {
                case 1:  m = 0.60f * osc_.square(dt); break;
                case 2:  m = 0.70f * osc_.pulse(dt, 0.25); break;
                case 3:  m = 0.78f * osc_.tri(dt); break;
                case 4:  m = 0.95f * osc_.sine(dt); break;
                case 5:  m = 0.55f * (osc_.saw(dt) + osc2_.saw(dt * 1.011)); break;
                default: m = osc_.saw(dt); break;
            }
            float s = m + subMix * sub_.square(dt * 0.5);
            s = lp_.process(s);
            v = std::tanh(s * (1.0f + 4.0f * drive)) * (float) amp;
            ++t_;
            if (t_ > gateLen_ && amp < 1e-4) t_ = -1;
        }
        o[i] = v * level;
    }
    stagedCount_ = 0;
}

}
