#include "core/BeatDetector.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include <juce_dsp/juce_dsp.h>

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
std::vector<float> onsetEnvelope(const juce::AudioBuffer<float>& buf, double sr,
                                 int hop, double& framesPerSec) {
    const int order = 10, N = 1 << order;
    framesPerSec = sr / hop;
    const int ch = buf.getNumChannels();
    const int64_t len = buf.getNumSamples();
    const int nFrames = (int) ((len - N) / hop);
    std::vector<float> env;
    if (nFrames < 8) return env;
    env.assign((size_t) nFrames, 0.0f);

    juce::dsp::FFT fft(order);
    std::vector<float> win((size_t) N), fd((size_t) N * 2), prev((size_t) (N / 2), 0.0f);
    for (int i = 0; i < N; ++i)
        win[(size_t) i] = 0.5f * (1.0f - std::cos(2.0f * kPiF * i / (N - 1)));

    for (int f = 0; f < nFrames; ++f) {
        const int64_t s0 = (int64_t) f * hop;
        std::fill(fd.begin(), fd.end(), 0.0f);
        for (int i = 0; i < N; ++i) {
            float m = 0.0f;
            for (int c = 0; c < ch; ++c) m += buf.getSample(c, (int) (s0 + i));
            fd[(size_t) i] = (m / (float) ch) * win[(size_t) i];
        }
        fft.performFrequencyOnlyForwardTransform(fd.data());
        float flux = 0.0f;
        for (int b = 1; b < N / 2; ++b) {
            const float d = fd[(size_t) b] - prev[(size_t) b];
            if (d > 0.0f) flux += d;
            prev[(size_t) b] = fd[(size_t) b];
        }
        env[(size_t) f] = flux;
    }
    double mean = 0.0; for (float v : env) mean += v; mean /= (double) nFrames;
    for (auto& v : env) v = (float) std::max(0.0, (double) v - mean);
    return env;
}

double autocorrAt(const std::vector<float>& env, int lag) {
    double ac = 0.0;
    for (int f = lag; f < (int) env.size(); ++f) ac += (double) env[(size_t) f] * env[(size_t) (f - lag)];
    return ac;
}
}

BeatEstimate detectBeat(const juce::AudioBuffer<float>& buf, double sampleRate,
                        double minBpm, double maxBpm) {
    BeatEstimate est;
    if (buf.getNumChannels() == 0 || sampleRate <= 0.0 ||
        buf.getNumSamples() < (int64_t) sampleRate) return est;

    const int hop = 512;
    double framesPerSec = 0.0;
    auto env = onsetEnvelope(buf, sampleRate, hop, framesPerSec);
    if (env.empty()) return est;

    const int nF = (int) env.size();
    const int lagMin = std::max(1, (int) std::floor(60.0 / maxBpm * framesPerSec));
    const int lagMax = std::min(nF - 1, (int) std::ceil(60.0 / minBpm * framesPerSec));
    double best = -1.0; int bestLag = lagMin;
    for (int lag = lagMin; lag <= lagMax; ++lag) {
        const double ac = autocorrAt(env, lag);
        if (ac > best) { best = ac; bestLag = lag; }
    }
    if (best <= 0.0) return est;

    auto bpmOf = [&](int lag) { return 60.0 * framesPerSec / lag; };
    auto score = [&](int lag) {
        if (lag < lagMin || lag > lagMax) return -1.0;
        const double b = bpmOf(lag);
        double w = 1.0;
        if (b < 90.0 || b > 150.0) w = 0.85;
        return autocorrAt(env, lag) * w;
    };
    int chosen = bestLag;
    for (int alt : { bestLag * 2, (bestLag + 1) / 2 })
        if (score(alt) > score(chosen)) chosen = alt;

    est.bpm = bpmOf(chosen);
    est.confidence = best;

    const int P = chosen;
    double bestPhaseScore = -1.0; int bestPhase = 0;
    for (int p = 0; p < P; ++p) {
        double sc = 0.0;
        for (int f = p; f < nF; f += P) sc += env[(size_t) f];
        if (sc > bestPhaseScore) { bestPhaseScore = sc; bestPhase = p; }
    }
    est.offsetSamples = (int64_t) bestPhase * hop;
    return est;
}

}
