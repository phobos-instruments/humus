#include "Phono/Phono.h"

#include <algorithm>
#include <cmath>
#include <complex>

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
constexpr double kT1 = 3180e-6, kT2 = 318e-6, kT3 = 75e-6, kIecT = 7950e-6;

constexpr double kRumbleHz = 18.0;
constexpr double kGainSmMs = 10.0;

constexpr int kNumB = Phono::kNumB;

std::complex<double> analogRiaa(double f) {
    const std::complex<double> jw(0.0, 2.0 * M_PI * f);
    return (1.0 + jw * kT2) / ((1.0 + jw * kT1) * (1.0 + jw * kT3));
}

void solveB(double a[kNumB][kNumB], double y[kNumB], double* x) {
    for (int col = 0; col < kNumB; ++col) {
        int piv = col;
        for (int r = col + 1; r < kNumB; ++r)
            if (std::abs(a[r][col]) > std::abs(a[piv][col])) piv = r;
        if (piv != col) {
            for (int j = 0; j < kNumB; ++j) std::swap(a[col][j], a[piv][j]);
            std::swap(y[col], y[piv]);
        }
        const double d = a[col][col] == 0.0 ? 1e-30 : a[col][col];
        for (int r = col + 1; r < kNumB; ++r) {
            const double m = a[r][col] / d;
            for (int j = col; j < kNumB; ++j) a[r][j] -= m * a[col][j];
            y[r] -= m * y[col];
        }
    }
    for (int r = kNumB - 1; r >= 0; --r) {
        double s = y[r];
        for (int j = r + 1; j < kNumB; ++j) s -= a[r][j] * x[j];
        x[r] = s / (a[r][r] == 0.0 ? 1e-30 : a[r][r]);
    }
}

void fitNumerator(double sr, const double* a, double tau, double* b) {
    constexpr int kPts = 280;
    const double f0 = 10.0, f1 = 0.47 * sr;
    const double m1k = std::abs(analogRiaa(1000.0));
    double ata[kNumB][kNumB] = {}, aty[kNumB] = {};
    for (int k = 0; k < kPts; ++k) {
        const double f = f0 * std::pow(f1 / f0, (double) k / (double) (kPts - 1));
        const double w = 2.0 * M_PI * f / sr;
        const std::complex<double> d =
            analogRiaa(f) / m1k * std::polar(1.0, -w * tau);
        const std::complex<double> den = 1.0 + a[0] * std::polar(1.0, -w)
                                         + a[1] * std::polar(1.0, -2.0 * w);
        const std::complex<double> target = d * den;
        double wt = 1.0 / std::max(1e-9, std::abs(target));
        if (f > 21000.0) wt *= 0.05;
        std::complex<double> e[kNumB];
        for (int i = 0; i < kNumB; ++i) e[i] = std::polar(1.0, -w * (double) i);
        for (int i = 0; i < kNumB; ++i) {
            for (int j = 0; j < kNumB; ++j)
                ata[i][j] += wt * wt * (e[i].real() * e[j].real()
                                        + e[i].imag() * e[j].imag());
            aty[i] += wt * wt * (e[i].real() * target.real()
                                 + e[i].imag() * target.imag());
        }
    }
    solveB(ata, aty, b);
}

double responseDb(double sr, const double* b, const double* a, double f) {
    const double w = 2.0 * M_PI * f / sr;
    std::complex<double> num = 0.0;
    for (int i = 0; i < kNumB; ++i) num += b[i] * std::polar(1.0, -w * (double) i);
    const std::complex<double> den = 1.0 + a[0] * std::polar(1.0, -w)
                                     + a[1] * std::polar(1.0, -2.0 * w);
    return 20.0 * std::log10(std::max(1e-12, std::abs(num / den)));
}

double maxBandErrorDb(double sr, const double* b, const double* a) {
    const double ref = responseDb(sr, b, a, 1000.0);
    const double aRef = 20.0 * std::log10(std::abs(analogRiaa(1000.0)));
    double worst = 0.0;
    for (double f = 20.0; f <= 20000.0; f *= 1.02) {
        const double meas = responseDb(sr, b, a, f) - ref;
        const double want = 20.0 * std::log10(std::abs(analogRiaa(f))) - aRef;
        worst = std::max(worst, std::abs(meas - want));
    }
    return worst;
}

void designRiaa(double sr, double* b, double* a) {
    const double p1 = std::exp(-1.0 / (kT1 * sr));
    const double p2 = std::exp(-1.0 / (kT3 * sr));
    a[0] = -(p1 + p2);
    a[1] = p1 * p2;

    double best[kNumB] = {};
    double bestErr = 1e9;
    for (double tau = 0.0; tau <= 3.001; tau += 0.05) {
        double cand[kNumB] = {};
        fitNumerator(sr, a, tau, cand);
        const double err = maxBandErrorDb(sr, cand, a);
        if (err < bestErr) {
            bestErr = err;
            std::copy(cand, cand + kNumB, best);
        }
    }
    const double g = std::pow(10.0, responseDb(sr, best, a, 1000.0) / 20.0);
    for (int i = 0; i < kNumB; ++i) b[i] = best[i] / (g <= 0.0 ? 1.0 : g);
}
}

void Phono::OnePoleHp::set(double sr, double f) {
    const double k = std::tan(M_PI * f / sr);
    b0 = 1.0 / (1.0 + k);
    b1 = -b0;
    a1 = (k - 1.0) / (k + 1.0);
}

void Phono::prepare(double sampleRate, int) {
    sr_ = sampleRate;
    designRiaa(sr_, riaaB_, riaaA_);
    const double iecHz = 1.0 / (2.0 * M_PI * kIecT);
    for (int c = 0; c < 2; ++c) {
        iecHp_[c].set(sr_, iecHz);
        rumble1_[c].set(sr_, kRumbleHz);
        rumble2_[c].setHighpass(sr_, kRumbleHz, 1.0);
    }
    reset();
}

void Phono::resetFilters() {
    for (int c = 0; c < 2; ++c) {
        eq_[c].reset();
        iecHp_[c].reset();
        rumble1_[c].reset();
        rumble2_[c].reset();
    }
}

void Phono::reset() {
    resetFilters();
    gainSm_ = -1.0;
}

void Phono::process(const float* const* in, int numIn, float* const* out, int numOut,
                    int numSamples, const Transport&) {
    const int curve = (int) std::clamp(params.get("Curve", 0.0), 0.0, 2.0);
    const double gainDb = std::clamp(params.get("Gain", 40.0), 20.0, 70.0);
    const bool rumbleOn = params.get("Rumble", 1.0) >= 0.5;

    const double gTarget = dbToLin(gainDb);
    if (gainSm_ < 0.0) gainSm_ = gTarget;
    const double gsm = smoothCoeff(kGainSmMs, sr_);

    for (int n = 0; n < numSamples; ++n) {
        gainSm_ = gsm * gainSm_ + (1.0 - gsm) * gTarget;
        for (int c = 0; c < numOut && c < 2; ++c) {
            double x = (c < numIn && in[c]) ? in[c][n] : 0.0;
            if (curve != 2) {
                x = eq_[c].process(riaaB_, riaaA_, x);
                if (curve == 1) x = iecHp_[c].process(x);
            }
            if (rumbleOn) x = rumble2_[c].process((float) rumble1_[c].process(x));
            out[c][n] = (float) (x * gainSm_);
        }
    }
}

}
