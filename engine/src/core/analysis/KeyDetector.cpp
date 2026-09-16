// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/analysis/KeyDetector.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

#include <juce_dsp/juce_dsp.h>

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
const std::array<double, 12> kMajor = {6.35,2.23,3.48,2.33,4.38,4.09,2.52,5.19,2.39,3.66,2.29,2.88};
const std::array<double, 12> kMinor = {6.33,2.68,3.52,5.38,2.60,3.53,2.54,4.75,3.98,2.69,3.34,3.17};
const char* kNames[12] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
const char* kCamMaj[12] = {"8B","3B","10B","5B","12B","7B","2B","9B","4B","11B","6B","1B"};
const char* kCamMin[12] = {"5A","12A","7A","2A","9A","4A","11A","6A","1A","8A","3A","10A"};

double correlate(const std::array<double, 12>& prof, const std::vector<double>& chroma, int rot) {
    double mp = 0, mc = 0;
    for (int i = 0; i < 12; ++i) { mp += prof[(size_t) i]; mc += chroma[(size_t) i]; }
    mp /= 12; mc /= 12;
    double num = 0, dp = 0, dc = 0;
    for (int i = 0; i < 12; ++i) {
        const double p = prof[(size_t) ((i + rot) % 12)] - mp;
        const double c = chroma[(size_t) i] - mc;
        num += p * c; dp += p * p; dc += c * c;
    }
    return (dp > 0 && dc > 0) ? num / std::sqrt(dp * dc) : 0.0;
}
}

KeyEstimate detectKey(const juce::AudioBuffer<float>& buf, double sampleRate) {
    KeyEstimate est;
    const int ch = buf.getNumChannels();
    const int64_t len = buf.getNumSamples();
    if (ch == 0 || sampleRate <= 0.0 || len < (int64_t) sampleRate) return est;

    const int order = 13, N = 1 << order;
    const int hop = N / 2;
    juce::dsp::FFT fft(order);
    std::vector<float> win((size_t) N), fd((size_t) N * 2);
    for (int i = 0; i < N; ++i) win[(size_t) i] = 0.5f * (1.0f - std::cos(2.0f * kPiF * i / (N - 1)));

    std::vector<double> chroma(12, 0.0);
    const double refA = kA4Hz;
    for (int64_t s0 = 0; s0 + N <= len; s0 += hop) {
        std::fill(fd.begin(), fd.end(), 0.0f);
        for (int i = 0; i < N; ++i) {
            float m = 0.0f;
            for (int c = 0; c < ch; ++c) m += buf.getSample(c, (int) (s0 + i));
            fd[(size_t) i] = (m / (float) ch) * win[(size_t) i];
        }
        fft.performFrequencyOnlyForwardTransform(fd.data());
        for (int b = 1; b < N / 2; ++b) {
            const double freq = (double) b * sampleRate / N;
            if (freq < 50.0 || freq > 5000.0) continue;
            const double midi = hzToMidi(freq, refA);
            const int pc = ((int) std::lround(midi) % 12 + 12) % 12;
            chroma[(size_t) pc] += fd[(size_t) b];
        }
    }
    double tot = 0; for (double v : chroma) tot += v;
    if (tot <= 0.0) return est;

    double best = -2.0;
    for (int root = 0; root < 12; ++root) {
        const double cMaj = correlate(kMajor, chroma, root);
        if (cMaj > best) { best = cMaj; est.pitchClass = root; est.major = true; }
        const double cMin = correlate(kMinor, chroma, root);
        if (cMin > best) { best = cMin; est.pitchClass = root; est.major = false; }
    }
    est.confidence = best;
    const int pc = est.pitchClass;
    est.name = std::string(kNames[pc]) + (est.major ? " major" : " minor");
    est.camelot = est.major ? kCamMaj[pc] : kCamMin[pc];
    return est;
}

}
