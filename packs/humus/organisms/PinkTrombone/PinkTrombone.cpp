#include "PinkTrombone/PinkTrombone.h"

#include <algorithm>
#include <cmath>

namespace hum {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 6.28318530717958647692;
}

void TromboneGlottis::reset() {
    timeInWaveform = 0.0;
    totalTime = 0.0;
    oldFrequency = newFrequency = targetFrequency = smoothFrequency = 140.0;
    oldTenseness = newTenseness = targetTenseness = 0.6;
    intensity = 0.0;
    loudness = 1.0;
    aspMod = 0.2;
    rng = 0x9e3779b9u;
    setupWaveform(0.0);
}

void TromboneGlottis::setupWaveform(double lambda) {
    frequency = oldFrequency * (1.0 - lambda) + newFrequency * lambda;
    const double tenseness = oldTenseness * (1.0 - lambda) + newTenseness * lambda;
    waveformLength = 1.0 / std::max(20.0, frequency);

    double rd = std::clamp(3.0 * (1.0 - tenseness), 0.5, 2.7);
    const double ra = -0.01 + 0.048 * rd;
    const double rk = 0.224 + 0.118 * rd;
    const double rg = (rk / 4.0) * (0.5 + 1.2 * rk) / (0.11 * rd - ra * (0.5 + 1.2 * rk));

    const double ta = ra;
    const double tp = 1.0 / (2.0 * rg);
    te = tp + tp * rk;

    epsilon = 1.0 / ta;
    shift = std::exp(-epsilon * (1.0 - te));
    delta = 1.0 - shift;

    double rhsIntegral = (1.0 / epsilon) * (shift - 1.0) + (1.0 - te) * shift;
    rhsIntegral = rhsIntegral / delta;
    const double totalLowerIntegral = -(te - tp) / 2.0 + rhsIntegral;
    const double totalUpperIntegral = -totalLowerIntegral;

    omega = kPi / tp;
    const double s = std::sin(omega * te);
    const double y = -kPi * s * totalUpperIntegral / (tp * 2.0);
    const double z = std::log(y);
    alpha = z / (tp / 2.0 - te);
    e0 = -1.0 / (s * std::exp(alpha * te));
}

double TromboneGlottis::noiseModulator() const {
    const double voiced =
        0.1 + 0.2 * std::max(0.0, std::sin(kTwoPi * timeInWaveform / waveformLength));
    return targetTenseness * intensity * voiced
         + (1.0 - targetTenseness * intensity) * 0.3;
}

double TromboneGlottis::runStep(double lambda, double aspirateNoise, double sampleRate) {
    const double timeStep = 1.0 / sampleRate;
    timeInWaveform += timeStep;
    totalTime += timeStep;
    if (timeInWaveform > waveformLength) {
        timeInWaveform -= waveformLength;
        setupWaveform(lambda);
    }
    const double t = timeInWaveform / waveformLength;
    double out;
    if (t > te) out = (-std::exp(-epsilon * (t - te)) + shift) / delta;
    else out = e0 * std::exp(alpha * t) * std::sin(omega * t);
    out *= intensity * loudness;

    double aspiration = intensity * (1.0 - std::sqrt(targetTenseness))
                      * noiseModulator() * aspirateNoise;
    aspiration *= aspMod;
    return out + aspiration;
}

void TromboneGlottis::finishBlock(bool sounding, bool droneBreath, double blockTime,
                                  bool wobble) {
    double vibrato = vibratoAmount * std::sin(kTwoPi * totalTime * 6.0);
    vibrato += 0.01 * vib1.step(blockTime * 4.07, rng) * driftAmount;
    vibrato += 0.02 * vib2.step(blockTime * 2.15, rng) * driftAmount;
    if (wobble) {
        vibrato += 0.2 * wob1.step(blockTime * 0.98, rng);
        vibrato += 0.4 * wob2.step(blockTime * 0.5, rng);
    }
    aspMod = 0.2 + 0.02 * asp1.step(blockTime * 1.99, rng);

    if (targetFrequency > smoothFrequency)
        smoothFrequency = std::min(smoothFrequency * 1.1, targetFrequency);
    if (targetFrequency < smoothFrequency)
        smoothFrequency = std::max(smoothFrequency / 1.1, targetFrequency);
    oldFrequency = newFrequency;
    newFrequency = smoothFrequency * (1.0 + vibrato);
    oldTenseness = newTenseness;
    newTenseness = targetTenseness + 0.1 * ten1.step(blockTime * 0.46, rng)
                 + 0.05 * ten2.step(blockTime * 0.36, rng);
    if (droneBreath) newTenseness += (3.0 - targetTenseness) * (1.0 - intensity);

    if (sounding) intensity += 12.0 * blockTime;
    else intensity -= 4.3 * blockTime;
    intensity = std::clamp(intensity, 0.0, 1.0);
}

void PinkTrombone::Bandpass::set(double hz, double q, double sr) {
    const double w0 = kTwoPi * hz / sr;
    const double alpha = std::sin(w0) / (2.0 * q);
    const double a0 = 1.0 + alpha;
    b0 = alpha / a0;
    b1 = 0.0;
    b2 = -alpha / a0;
    a1 = -2.0 * std::cos(w0) / a0;
    a2 = (1.0 - alpha) / a0;
}

double PinkTrombone::Bandpass::process(double x) {
    const double y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1;
    x1 = x;
    y2 = y1;
    y1 = y;
    return y;
}

void PinkTrombone::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    aspFilter_.set(500.0, 0.5, sampleRate);
    fricFilter_.set(1000.0, 0.5, sampleRate);
    reset();
}

void PinkTrombone::reset() {
    glottis_.reset();
    tract_.init();
    aspFilter_.clear();
    fricFilter_.clear();
    held_.clear();
    stagedCount_ = 0;
    fricIntensity_ = 0.0;
    velocity_ = 0.8;
}

void PinkTrombone::process(const float* const*, int, float* const* out, int numOut,
                           int numSamples, const Transport&) {
    if (numOut < 1 || numSamples < 1) return;

    for (int i = 0; i < stagedCount_; ++i) {
        const auto& e = staged_[(size_t) i];
        held_.apply(e);
        if ((e.data[0] & 0xF0) == 0x90 && e.data[2] > 0)
            velocity_ = e.data[2] / 127.0;
    }
    stagedCount_ = 0;

    const double tongueX = std::clamp(params.get("TongueX", 0.5), 0.0, 1.0);
    const double tongueY = std::clamp(params.get("TongueY", 0.5), 0.0, 1.0);
    const double place = std::clamp(params.get("Place", 0.7), 0.0, 1.0);
    const double squeeze = std::clamp(params.get("Squeeze", 0.0), 0.0, 1.0);
    const double nasal = std::clamp(params.get("Nasal", 0.0), 0.0, 1.0);
    const double tenseness = std::clamp(params.get("Tenseness", 0.6), 0.01, 0.99);
    const double vibrato = std::clamp(params.get("Vibrato", 0.25), 0.0, 1.0);
    const bool wobble = params.get("Wobble", 0.0) >= 0.5;
    const bool drone = params.get("Drone", 0.0) >= 0.5;
    const float level = (float) std::clamp(params.get("Level", 0.8), 0.0, 2.0);

    const bool voiced = held_.any() || drone;
    if (held_.any()) {
        glottis_.targetFrequency =
            440.0 * std::pow(2.0, (held_.top() - 69) / 12.0);
        if (glottis_.intensity == 0.0)
            glottis_.smoothFrequency = glottis_.targetFrequency;
    }
    glottis_.targetTenseness = tenseness;
    glottis_.loudness = std::pow(tenseness, 0.25) * (0.35 + 0.65 * velocity_);
    glottis_.vibratoAmount = 0.002 + 0.02 * vibrato * vibrato;
    glottis_.driftAmount = 0.2 + 0.8 * vibrato;

    const double tongueIndex =
        TromboneTract::kBladeStart + 2
        + tongueX * (TromboneTract::kTipStart - 3 - (TromboneTract::kBladeStart + 2));
    const double tongueDiameter = 3.5 - 1.45 * tongueY;
    tract_.setRestDiameter(tongueIndex, tongueDiameter);
    const double constrIndex = 2.0 + place * (TromboneTract::kN - 3);
    const double constrRaw = 0.3 + 1.6 * (1.0 - squeeze);
    double constrDiameter = -1.0;
    if (squeeze > 0.05) {
        tract_.applyConstriction(constrIndex, constrRaw);
        constrDiameter = std::max(0.0, constrRaw - 0.3);
    }
    tract_.velumTarget = 0.01 + 0.39 * nasal;

    const double blockTime = numSamples / sampleRate_;
    const double fricStep = blockTime / 0.1;
    fricIntensity_ = std::clamp(
        fricIntensity_ + (squeeze > 0.05 ? fricStep : -fricStep), 0.0, 1.0);

    float* dst = out[0];
    for (int j = 0; j < numSamples; ++j) {
        noiseRng_ ^= noiseRng_ << 13;
        noiseRng_ ^= noiseRng_ >> 17;
        noiseRng_ ^= noiseRng_ << 5;
        const double white = (double) noiseRng_ / 4294967296.0;
        const double asp = aspFilter_.process(white);
        const double fric = fricFilter_.process(white);

        const double lambda1 = (double) j / numSamples;
        const double lambda2 = (j + 0.5) / numSamples;
        const double glottal = glottis_.runStep(lambda1, asp, sampleRate_);
        const double noiseMod = glottis_.noiseModulator();
        const double turbulence = 0.66 * fric * fricIntensity_;

        double vocal = 0.0;
        tract_.runStep(glottal, turbulence, lambda1, noiseMod, constrIndex,
                       constrDiameter, sampleRate_);
        vocal += tract_.lipOutput + tract_.noseOutput;
        tract_.runStep(glottal, turbulence, lambda2, noiseMod, constrIndex,
                       constrDiameter, sampleRate_);
        vocal += tract_.lipOutput + tract_.noseOutput;
        dst[j] = (float) (vocal * 0.125) * level;
    }
    glottis_.finishBlock(voiced, !held_.any() && drone, blockTime, wobble);
    tract_.reshape(blockTime);
    tract_.calculateReflections();
}

}
