#include "Kick/Kick.h"

#include <algorithm>
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

void Kick::process(const float* const*, int, float* const* out, int numOut,
                   int numSamples, const Transport& transport) {
    if (numOut == 0) return;
    float* o = out[0];
    for (int c = 1; c < numOut; ++c) std::fill(out[c], out[c] + numSamples, 0.0f);

    const double sr = sampleRate_ > 0.0 ? sampleRate_ : kDefaultSampleRate;
    const double tune = params.get("Tune", 48.0);
    const double punch = params.get("Punch", 4.0);
    const double tp = std::max(1.0, params.get("PitchDecay", 40.0)) * 0.001 * sr;
    const double td = std::max(10.0, params.get("Decay", 420.0)) * 0.001 * sr;
    const float click = (float) params.get("Click", 0.35);
    const float wood = (float) params.get("Wood", 0.0);
    const float drive = (float) params.get("Drive", 0.25);
    const float level = (float) params.get("Level", 0.9);

    int trig[16];
    int nT = 0;
    const bool trigNow = params.get("Trigger", 0.0) >= 0.5;
    if (trigNow && !lastTrigParam_ && nT < 16) trig[nT++] = 0;
    lastTrigParam_ = trigNow;
    for (int i = 0; i < stagedCount_ && nT < 16; ++i) {
        const auto& e = staged_[(size_t) i];
        if ((e.data[0] & 0xF0) == 0x90 && e.data[2] > 0)
            trig[nT++] = std::clamp(e.sampleOffset, 0, numSamples - 1);
    }
    stagedCount_ = 0;
    if (params.get("FourFloor", 0.0) >= 0.5 && transport.playing()) {
        const double bps = transport.tempo() / kSecondsPerMinute;
        double next = std::ceil(transport.beats() - 1e-9);
        while (nT < 16) {
            const double off = (next - transport.beats()) / bps * sr;
            if (off >= numSamples) break;
            trig[nT++] = (int) std::max(0.0, off);
            next += 1.0;
        }
    }
    std::sort(trig, trig + nT);

    const long clickLen = (long) (0.003 * sr);
    int ti = 0;
    for (int i = 0; i < numSamples; ++i) {
        while (ti < nT && trig[ti] <= i) {
            if (trig[ti] == i) { t_ = 0; phase_ = 0.0; wphase1_ = wphase2_ = 0.0; }
            ++ti;
        }
        float v = 0.0f;
        if (t_ >= 0) {
            const double pe = std::exp(-(double) t_ / tp);
            const double f = tune * (1.0 + punch * pe);
            phase_ += f / sr;
            if (phase_ >= 1.0) phase_ -= 1.0;
            const double body = std::exp(-(double) t_ / td);
            double amp = body;
            if (t_ < 48) amp *= (double) t_ / 48.0;
            double s = std::sin(2.0 * kPi * phase_) * amp;
            if (t_ < clickLen)
                s += click * frand() * std::exp(-(double) t_ / (0.001 * sr)) * 0.8;
            if (wood > 0.0f) {
                const double kamp = std::exp(-(double) t_ / (0.035 * sr));
                wphase1_ += tune * 6.7 / sr;
                if (wphase1_ >= 1.0) wphase1_ -= 1.0;
                wphase2_ += tune * 12.3 / sr;
                if (wphase2_ >= 1.0) wphase2_ -= 1.0;
                s += wood * 0.9 * kamp
                     * (0.7 * std::sin(2.0 * kPi * wphase1_)
                        + 0.3 * std::sin(2.0 * kPi * wphase2_));
            }
            v = (float) std::tanh(s * (1.0 + 5.0 * drive));
            ++t_;
            if (body < 1e-4) t_ = -1;
        }
        o[i] = v * level;
    }
}

}
