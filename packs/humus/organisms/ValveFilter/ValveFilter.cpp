#include "ValveFilter/ValveFilter.h"

#include <algorithm>
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

void ValveFilter::process(const float* const* in, int numIn, float* const* out, int numOut,
                          int numSamples, const Transport& transport) {
    auto tap = [&](int c, int n) -> float {
        return (c < numIn && in[c]) ? in[c][n] : 0.0f;
    };
    const double sr = sampleRate_ > 0.0 ? sampleRate_ : 44100.0;

    const bool legacy = params.byName("Mode") != nullptr;
    const int legacyMode = (int) params.get("Mode", 0.0);
    auto band = [&](const char* nm, int m, bool def) {
        if (const auto* pp = params.byName(nm)) return pp->value >= 0.5;
        return legacy ? legacyMode == m : def;
    };
    const bool hpT = band("HiPass", 1, false);
    const bool bpT = band("BandPass", 2, false);
    const bool lpT = band("LoPass", 0, true);
    const bool onT = params.get("FilterOn", 1.0) >= 0.5 && (hpT || bpT || lpT);
    const double freq = std::clamp(params.get("Frequency", 20000.0), 20.0, 20000.0);
    const double res01 = std::clamp(params.get("Resonance", 2.0) / 10.0, 0.0, 1.0);
    const double drive01 = std::clamp(params.get("Drive", 0.0) / 10.0, 0.0, 1.0);
    const double envAmt = std::clamp(params.get("EnvFollow", 0.0) / 10.0, 0.0, 1.0);
    const bool slowDecay = params.get("EnvDecay", 0.0) >= 0.5;
    const bool efToOd = params.get("EnvToDrive", 0.0) >= 0.5;
    const double lfoHz = std::clamp(params.get("LfoSpeed", 2.0), 0.2, 10000.0);
    const bool lfoSync = params.get("LfoSync", 0.0) >= 0.5;
    const double lfoBeats = std::max(0.0625, params.get("LfoBeats", 1.0));
    const bool lfoSquare = params.get("LfoWave", 0.0) >= 0.5;
    const double lfoAmt = std::clamp(params.get("LfoDepth", 0.0) / 10.0, 0.0, 1.0);
    const bool mono = params.get("Mono", 0.0) >= 0.5;
    const bool invMix = params.get("MixInvert", 0.0) >= 0.5;

    const double atk = smoothCoeff(3.0, sr);
    const double rel = smoothCoeff(slowDecay ? 500.0 : 120.0, sr);
    const double sw = smoothCoeff(4.0, sr);
    const double lfoInc = lfoSync ? 1.0 / std::max(1.0, transport.samplesPerBeat() * lfoBeats)
                                  : lfoHz / sr;
    const bool fastLfo = !lfoSync && lfoHz > 100.0;

    auto stage = [&](double x, int c) -> double {
        double u = x * vg_;
        u += asym_ * u * u;
        double y = std::tanh(u);
        if (hard_ > 0.0) y += hard_ * (std::clamp(u, -0.85, 0.85) - y);
        y *= norm_;
        const double dcOut = y - dcx_[c] + 0.9989 * dcy_[c];
        dcx_[c] = y;
        dcy_[c] = dcOut;
        y = dcOut;
        const auto o = svf_[c].process(y, g_, k_);
        return hpG_ * o.hp + bpG_ * o.bp + lpG_ * o.lp;
    };

    for (int n = 0; n < numSamples; ++n) {
        const double L = tap(0, n), R = tap(1, n);

        const double a = std::max(std::abs(L), std::abs(R));
        env_ = a > env_ ? atk * env_ + (1.0 - atk) * a
                        : rel * env_ + (1.0 - rel) * a;
        const double env01 = std::min(1.0, env_ * 2.5);

        lfoPhase_ += lfoInc;
        if (lfoPhase_ >= 1.0) lfoPhase_ -= 1.0;
        const double lfo = lfoSquare ? (lfoPhase_ < 0.5 ? 1.0 : -1.0)
                                     : 4.0 * std::abs(lfoPhase_ - 0.5) - 1.0;

        if (fastLfo || (n & 7) == 0) {
            double oct = lfoAmt * 3.5 * lfo;
            if (!efToOd) oct += envAmt * 7.0 * env01;
            const double fc =
                std::clamp(freq * std::exp2(oct), 20.0, std::min(20000.0, sr * 0.45));
            double q = legacy ? std::clamp(res01 * 10.0, 0.5, 16.0)
                              : 0.707 * std::pow(10.0, res01 * 1.35);
            q /= 1.0 + 1.8 * env01 * (q / 16.0);
            if (fc < 150.0) q = std::max(0.707, q * std::sqrt(fc / 150.0));
            g_ = SvfTpt::gFor(fc, sr);
            k_ = 1.0 / q;
            const double d = std::clamp(drive01 + (efToOd ? envAmt * env01 : 0.0), 0.0, 1.0);
            vg_ = 1.0 + d * 24.0;
            norm_ = 1.0 / (1.0 + d * 2.2);
            hard_ = std::max(0.0, d - 0.6) / 0.4 * 0.6;
            asym_ = 0.10 + 0.25 * d;
        }

        on_ += ((onT ? 1.0 : 0.0) - on_) * (1.0 - sw);
        hpG_ += ((hpT ? 1.0 : 0.0) - hpG_) * (1.0 - sw);
        bpG_ += ((bpT ? 1.0 : 0.0) - bpG_) * (1.0 - sw);
        lpG_ += ((lpT ? 1.0 : 0.0) - lpG_) * (1.0 - sw);

        double wetL, wetR;
        if (mono) {
            const double w = stage(stage(L, 0), 1);
            wetL = wetR = w;
        } else {
            wetL = stage(L, 0);
            wetR = stage(R, 1);
        }
        if (invMix) {
            wetL = L - wetL;
            wetR = R - wetR;
        }
        if (numOut > 0) out[0][n] = (float) (L * (1.0 - on_) + wetL * on_);
        if (numOut > 1) out[1][n] = (float) (R * (1.0 - on_) + wetR * on_);
    }
}

}
