// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Cicada/Cicada.h"

#include <algorithm>
#include <cmath>

#include "hum/Swing.h"
#include "hum/dsp/DspMath.h"

namespace hum {

void Cicada::process(const float* const*, int, float* const* out, int numOut,
                     int numSamples, const Transport& transport) {
    if (numOut == 0) return;
    float* o = out[0];
    for (int c = 1; c < numOut; ++c) std::fill(out[c], out[c] + numSamples, 0.0f);

    const double sr = sampleRate_ > 0.0 ? sampleRate_ : kDefaultSampleRate;
    const double ratio = std::exp2(params.get("Tune", 0.0) / 12.0);
    const double metal = params.get("Metal", 0.7);
    const double tone = params.get("Tone", 0.5);
    const double cd = std::max(5.0, params.get("ClosedDecay", 45.0)) * 0.001;
    const double od = std::max(5.0, params.get("OpenDecay", 450.0)) * 0.001;
    const double drive = params.get("Drive", 0.2);
    const float level = (float) params.get("Level", 0.9);

    struct T { int at; int v; };
    T trig[24];
    int nT = 0;
    auto edge = [&](const char* name, bool& last, int v) {
        const bool now = params.get(name, 0.0) >= 0.5;
        if (now && !last && nT < 24) trig[nT++] = {0, v};
        last = now;
    };
    edge("Closed", lastC_, 0);
    edge("Open", lastO_, 1);
    edge("Crash", lastCr_, 2);
    for (int i = 0; i < stagedCount_ && nT < 24; ++i) {
        const auto& e = staged_[(size_t) i];
        if ((e.data[0] & 0xF0) == 0x90 && e.data[2] > 0) {
            const int note = e.data[1];
            const int v = note == 46 ? 1 : note == 49 ? 2 : 0;
            trig[nT++] = {std::clamp(e.sampleOffset, 0, numSamples - 1), v};
        }
    }
    stagedCount_ = 0;
    if (params.get("OffBeat", 0.0) >= 0.5 && transport.playing()) {
        const double bps = transport.tempo() / kSecondsPerMinute;
        const auto groove = swing::resolve(params, transport, Pattern::kTicksPerBeat / 2);
        double next = std::ceil(transport.beats() - 0.5 - 1e-9) - 0.5;
        while (nT < 24) {
            const double tick = next * Pattern::kTicksPerBeat;
            const double pos = next + swing::delayTicks(tick, groove) / Pattern::kTicksPerBeat;
            const double off = (pos - transport.beats()) / bps * sr;
            if (off >= numSamples) break;
            if (off >= 0.0) trig[nT++] = {(int) off, 1};
            next += 1.0;
        }
    }
    std::sort(trig, trig + nT, [](const T& a, const T& b) { return a.at < b.at; });

    if (nT == 0 && cEnv_ <= 0.0 && oEnv_ <= 0.0 && crEnv_ <= 0.0 && splash_ <= 0.0) {
        std::fill(o, o + numSamples, 0.0f);
        return;
    }

    const double cMul = std::exp(-1.0 / (cd * sr));
    const double oMul = std::exp(-1.0 / (od * sr));
    const double crMul = std::exp(-1.0 / (4.0 * od * sr));
    const double chokeMul = std::exp(-1.0 / (0.005 * sr));
    const double splashMul = std::exp(-1.0 / (0.08 * sr));

    const double nyq = 0.45 * sr;
    const double fbp = std::min(nyq, 8000.0 + 3000.0 * tone);
    const double fhp = std::min(nyq, 3500.0 + 5500.0 * tone);
    double b0b, b1b, b2b, a1b, a2b, b0h, b1h, b2h, a1h, a2h;
    {
        const double w = 2.0 * kPi * fbp / sr;
        const double al = std::sin(w) / (2.0 * 1.0);
        const double a0 = 1.0 + al;
        b0b = al / a0; b1b = 0.0; b2b = -al / a0;
        a1b = -2.0 * std::cos(w) / a0; a2b = (1.0 - al) / a0;
    }
    {
        const double w = 2.0 * kPi * fhp / sr;
        const double al = std::sin(w) / (2.0 * 0.707);
        const double a0 = 1.0 + al;
        const double cw = std::cos(w);
        b0h = ((1.0 + cw) / 2.0) / a0; b1h = -(1.0 + cw) / a0; b2h = b0h;
        a1h = -2.0 * cw / a0; a2h = (1.0 - al) / a0;
    }

    static const double kHz[6] = {205.3, 304.4, 369.6, 522.7, 540.0, 800.0};
    const long atk = (long) (0.0008 * sr) + 1;
    auto ramp = [&](long age) { return age < atk ? (double) age / (double) atk : 1.0; };
    int ti = 0;
    for (int i = 0; i < numSamples; ++i) {
        while (ti < nT && trig[ti].at <= i) {
            if (trig[ti].at == i) {
                switch (trig[ti].v) {
                case 0: cEnv_ = 1.0; cAge_ = 0; choked_ = true; break;
                case 1: oEnv_ = 1.0; oAge_ = 0; choked_ = false; break;
                case 2: crEnv_ = 1.0; splash_ = 1.0; crAge_ = 0; break;
                }
            }
            ++ti;
        }
        double sq = 0.0;
        for (int k = 0; k < 6; ++k) {
            phase_[(size_t) k] += kHz[k] * ratio / sr;
            if (phase_[(size_t) k] >= 1.0) phase_[(size_t) k] -= 1.0;
            sq += phase_[(size_t) k] < 0.5 ? 1.0 : -1.0;
        }
        sq *= 1.0 / 6.0;
        const double n = (double) frand();
        const double pre = metal * sq + (1.0 - metal) * n * 0.8;
        double y = b0b * pre + bz1_;
        bz1_ = b1b * pre - a1b * y + bz2_;
        bz2_ = b2b * pre - a2b * y;
        double y2 = b0h * y + hz1_;
        hz1_ = b1h * y - a1h * y2 + hz2_;
        hz2_ = b2h * y - a2h * y2;
        const double body = y2 * 3.0;

        const double amp = cEnv_ * ramp(cAge_) + oEnv_ * ramp(oAge_) * 0.95
                         + crEnv_ * ramp(crAge_) * 1.15;
        double v = body * amp;
        if (splash_ > 1e-4) v += n * splash_ * ramp(crAge_) * 0.5;

        cEnv_ *= cMul;
        oEnv_ *= choked_ ? chokeMul : oMul;
        crEnv_ *= crMul;
        splash_ *= splashMul;
        ++cAge_; ++oAge_; ++crAge_;
        o[i] = (float) std::tanh(v * (1.0 + 4.0 * drive)) * level;
    }
    if (cEnv_ < 1e-4) cEnv_ = 0.0;
    if (oEnv_ < 1e-4) oEnv_ = 0.0;
    if (crEnv_ < 1e-4) crEnv_ = 0.0;
    if (splash_ < 1e-4) splash_ = 0.0;
}

}
