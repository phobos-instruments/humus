#include <algorithm>
#include <cmath>
#include <vector>

#include <juce_dsp/juce_dsp.h>

#include "SoundSpace/SoundSpace.h"
#include "hum/dsp/SoundFileBuffer.h"

namespace hum {

namespace {

constexpr int kFftOrder = 12;
constexpr int kWin = 1 << kFftOrder;
constexpr int kBins = kWin / 2;
constexpr int kFeatures = 6;

using Feature = std::array<float, kFeatures>;

Feature featuresOf(const float* mags, const float* time, int timeLen,
                   const float* prevMags, double sampleRate) {
    double sum = 0.0, wsum = 0.0, logsum = 0.0;
    for (int b = 1; b < kBins; ++b) {
        sum += mags[b];
        wsum += mags[b] * b;
        logsum += std::log(1.0e-9 + mags[b]);
    }
    const double centroid = sum > 0.0 ? (wsum / sum) / kBins : 0.0;
    double roll = 0.0, acc = 0.0;
    for (int b = 1; b < kBins && acc < 0.85 * sum; ++b) { acc += mags[b]; roll = b; }
    const double flatness = sum > 0.0
        ? std::exp(logsum / (kBins - 1)) / (sum / (kBins - 1)) : 0.0;
    double flux = 0.0;
    if (prevMags)
        for (int b = 1; b < kBins; ++b) {
            const double d = mags[b] - prevMags[b];
            if (d > 0.0) flux += d;
        }
    double rms = 0.0;
    int zc = 0;
    for (int i = 0; i < timeLen; ++i) {
        rms += time[i] * time[i];
        if (i > 0 && (time[i] >= 0.0f) != (time[i - 1] >= 0.0f)) ++zc;
    }
    rms = std::sqrt(rms / std::max(1, timeLen));

    juce::ignoreUnused(sampleRate);
    return {(float) std::log10(1.0e-6 + rms),
            (float) centroid,
            (float) (roll / kBins),
            (float) flatness,
            (float) std::log10(1.0e-9 + flux),
            (float) zc / (float) std::max(1, timeLen)};
}

void pca2(const std::vector<Feature>& rows, std::vector<float>& outX, std::vector<float>& outY) {
    const int n = (int) rows.size();
    outX.assign((size_t) n, 0.5f);
    outY.assign((size_t) n, 0.5f);
    if (n < 3) return;

    std::array<double, kFeatures> mean{}, sd{};
    for (const auto& r : rows)
        for (int f = 0; f < kFeatures; ++f) mean[(size_t) f] += r[(size_t) f];
    for (auto& m : mean) m /= n;
    for (const auto& r : rows)
        for (int f = 0; f < kFeatures; ++f) {
            const double d = r[(size_t) f] - mean[(size_t) f];
            sd[(size_t) f] += d * d;
        }
    for (auto& s : sd) s = std::sqrt(s / n);
    std::vector<Feature> z(rows.size());
    for (size_t i = 0; i < rows.size(); ++i)
        for (int f = 0; f < kFeatures; ++f)
            z[i][(size_t) f] = sd[(size_t) f] > 1.0e-9
                ? (float) ((rows[i][(size_t) f] - mean[(size_t) f]) / sd[(size_t) f]) : 0.0f;

    double cov[kFeatures][kFeatures] = {};
    for (const auto& r : z)
        for (int a = 0; a < kFeatures; ++a)
            for (int b = 0; b < kFeatures; ++b) cov[a][b] += r[(size_t) a] * r[(size_t) b];
    for (auto& row : cov)
        for (auto& v : row) v /= n;

    auto powerIterate = [&](std::array<double, kFeatures>& v) {
        for (int f = 0; f < kFeatures; ++f) v[(size_t) f] = 0.3 + 0.1 * f;
        double lambda = 0.0;
        for (int it = 0; it < 60; ++it) {
            std::array<double, kFeatures> w{};
            for (int a = 0; a < kFeatures; ++a)
                for (int b = 0; b < kFeatures; ++b) w[(size_t) a] += cov[a][b] * v[(size_t) b];
            double norm = 0.0;
            for (double x : w) norm += x * x;
            norm = std::sqrt(norm);
            if (norm < 1.0e-12) break;
            for (int f = 0; f < kFeatures; ++f) v[(size_t) f] = w[(size_t) f] / norm;
            lambda = norm;
        }
        return lambda;
    };
    std::array<double, kFeatures> e1{}, e2{};
    const double l1 = powerIterate(e1);
    for (int a = 0; a < kFeatures; ++a)
        for (int b = 0; b < kFeatures; ++b) cov[a][b] -= l1 * e1[(size_t) a] * e1[(size_t) b];
    powerIterate(e2);

    float loX = 1.0e9f, hiX = -1.0e9f, loY = 1.0e9f, hiY = -1.0e9f;
    for (size_t i = 0; i < z.size(); ++i) {
        double px = 0.0, py = 0.0;
        for (int f = 0; f < kFeatures; ++f) {
            px += z[i][(size_t) f] * e1[(size_t) f];
            py += z[i][(size_t) f] * e2[(size_t) f];
        }
        outX[i] = (float) px; outY[i] = (float) py;
        loX = std::min(loX, outX[i]); hiX = std::max(hiX, outX[i]);
        loY = std::min(loY, outY[i]); hiY = std::max(hiY, outY[i]);
    }
    const float sx = hiX - loX > 1.0e-6f ? 0.94f / (hiX - loX) : 0.0f;
    const float sy = hiY - loY > 1.0e-6f ? 0.94f / (hiY - loY) : 0.0f;
    for (int i = 0; i < n; ++i) {
        outX[(size_t) i] = 0.03f + (sx > 0.0f ? (outX[(size_t) i] - loX) * sx : 0.47f);
        outY[(size_t) i] = 0.03f + (sy > 0.0f ? (outY[(size_t) i] - loY) * sy : 0.47f);
    }
}

}

std::shared_ptr<SoundSpace::Corpus> SoundSpace::analyzeCorpus(
        const std::array<std::string, kFiles>& uris, double engineRate) {
    juce::ignoreUnused(engineRate);
    auto corpus = std::make_shared<Corpus>();
    std::vector<Feature> feats;

    juce::dsp::FFT fft(kFftOrder);
    std::vector<float> window((size_t) kWin), fftBuf((size_t) kWin * 2);
    std::vector<float> mags((size_t) kBins), prevMags((size_t) kBins);

    for (int f = 0; f < kFiles; ++f) {
        const std::string& uri = uris[(size_t) f];
        if (uri.empty()) continue;
        juce::AudioBuffer<float>& buf = corpus->files[(size_t) f];
        double sr = 44100.0;
        if (!loadSoundFile(uri, buf, sr) || buf.getNumSamples() < kWin) { buf.setSize(0, 0); continue; }
        corpus->rates[(size_t) f] = sr;

        const int len = buf.getNumSamples();
        const int chans = buf.getNumChannels();
        const int hop = std::max(kWin / 2, (len - kWin) / std::max(1, kGrainsPerFile - 1));
        bool havePrev = false;
        for (int start = 0; start + kWin <= len; start += hop) {
            for (int i = 0; i < kWin; ++i) {
                float s = 0.0f;
                for (int c = 0; c < chans; ++c) s += buf.getSample(c, start + i);
                s /= (float) chans;
                const float w = 0.5f - 0.5f * std::cos(6.2831853f * i / (kWin - 1));
                window[(size_t) i] = s;
                fftBuf[(size_t) i] = s * w;
            }
            std::fill(fftBuf.begin() + kWin, fftBuf.end(), 0.0f);
            fft.performFrequencyOnlyForwardTransform(fftBuf.data());
            std::copy(fftBuf.begin(), fftBuf.begin() + kBins, mags.begin());

            feats.push_back(featuresOf(mags.data(), window.data(), kWin,
                                       havePrev ? prevMags.data() : nullptr, sr));
            corpus->grains.push_back({f, start, kWin, 0.5f, 0.5f});
            prevMags = mags;
            havePrev = true;
        }
    }

    std::vector<float> xs, ys;
    pca2(feats, xs, ys);
    for (size_t i = 0; i < corpus->grains.size(); ++i) {
        corpus->grains[i].x = xs[i];
        corpus->grains[i].y = ys[i];
    }
    return corpus;
}

}
