// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <juce_audio_formats/juce_audio_formats.h>
#include <cmath>

#include <juce_core/juce_core.h>

#include "hum/dsp/SliceDetect.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class WaveformCache {
public:
    static WaveformCache& instance() { static WaveformCache c; return c; }

    enum class Trouble { None, Missing, Unreadable };

    struct Peaks {
        std::vector<float> lo, hi;
        std::vector<float> rms;
        std::vector<SliceOnset> onsets;
        std::int64_t sourceSamples = 0;
        int binSamples = 0;
        double fileSampleRate = 0.0;
        bool ready = false;
        Trouble trouble = Trouble::None;
    };

    const Peaks* get(const std::string& path, std::function<void()> onReady) {
        if (path.empty()) return nullptr;
        const juce::File f(juce::String(juce::CharPointer_UTF8(path.c_str())));
        const std::int64_t mtime = f.getLastModificationTime().toMilliseconds();
        auto& e = cache_[path];
        if (e.peaks && e.peaks->ready && e.mtime == mtime) return e.peaks.get();
        if (!e.loading || e.mtime != mtime) {
            e.mtime = mtime;
            e.loading = true;
            if (onReady) readyCbs_.push_back(std::move(onReady));
            launch(path, mtime);
        } else if (onReady) {
            readyCbs_.push_back(std::move(onReady));
        }
        return nullptr;
    }

    const Peaks* prime(const std::string& path) {
        if (path.empty()) return nullptr;
        const juce::File f(juce::String(juce::CharPointer_UTF8(path.c_str())));
        auto& e = cache_[path];
        e.mtime = f.getLastModificationTime().toMilliseconds();
        auto peaks = std::make_shared<Peaks>();
        compute(path, *peaks);
        e.peaks = peaks;
        e.loading = false;
        return e.peaks.get();
    }

private:
    struct Entry { std::int64_t mtime = 0; std::shared_ptr<Peaks> peaks; bool loading = false; };

    static constexpr int kBinSamples = 512;
    static constexpr int kMaxBins = 300000;

    void launch(const std::string& path, std::int64_t mtime) {
        juce::Thread::launch([path, mtime] {
            auto peaks = std::make_shared<Peaks>();
            compute(path, *peaks);
            juce::MessageManager::callAsync([path, mtime, peaks] {
                auto& self = instance();
                auto& e = self.cache_[path];
                if (e.mtime != mtime) return;
                e.peaks = peaks;
                e.loading = false;
                auto cbs = std::move(self.readyCbs_);
                self.readyCbs_.clear();
                for (auto& cb : cbs) cb();
            });
        });
    }

    static void compute(const std::string& path, Peaks& out) {
        std::string uri = path;
        if (uri.rfind("file://", 0) == 0) uri = uri.substr(7);
        const juce::File f(juce::String(juce::CharPointer_UTF8(uri.c_str())));
        juce::AudioFormatManager fm; fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> rd(fm.createReaderFor(f));
        if (!rd || rd->lengthInSamples <= 0) {
            out.trouble = f.existsAsFile() ? Trouble::Unreadable : Trouble::Missing;
            out.ready = true;
            return;
        }

        const std::int64_t len = rd->lengthInSamples;
        const int nBins = (int) std::min<std::int64_t>(
            kMaxBins, (len + kBinSamples - 1) / kBinSamples);
        out.sourceSamples = len;
        out.binSamples = kBinSamples;
        out.fileSampleRate = rd->sampleRate > 0.0 ? rd->sampleRate : kDefaultSampleRate;
        out.lo.assign((size_t) nBins, 0.0f);
        out.hi.assign((size_t) nBins, 0.0f);
        out.rms.assign((size_t) nBins, 0.0f);
        std::vector<double> sq((size_t) nBins, 0.0);
        std::vector<int> cnt((size_t) nBins, 0);
        const int hops = (int) std::min<std::int64_t>(len / kOnsetHop, (std::int64_t) kMaxBins * 2);
        std::vector<double> hopEnergy((size_t) hops, 0.0);

        juce::AudioBuffer<float> buf((int) rd->numChannels, 1 << 16);
        std::int64_t pos = 0;
        while (pos < len) {
            const int n = (int) std::min<std::int64_t>(buf.getNumSamples(), len - pos);
            rd->read(&buf, 0, n, pos, true, true);
            for (int i = 0; i < n; ++i) {
                float s = 0.0f;
                for (int c = 0; c < buf.getNumChannels(); ++c) s += buf.getSample(c, i);
                s /= (float) juce::jmax(1, buf.getNumChannels());
                const int bin = (int) ((pos + i) / kBinSamples);
                if (const int h = (int) ((pos + i) / kOnsetHop); h < hops)
                    hopEnergy[(size_t) h] += (double) s * s / kOnsetHop;
                if (bin >= nBins) continue;
                out.lo[(size_t) bin] = std::min(out.lo[(size_t) bin], s);
                out.hi[(size_t) bin] = std::max(out.hi[(size_t) bin], s);
                sq[(size_t) bin] += (double) s * s;
                ++cnt[(size_t) bin];
            }
            pos += n;
        }
        for (int b = 0; b < nBins; ++b)
            if (cnt[(size_t) b] > 0)
                out.rms[(size_t) b] = (float) std::sqrt(sq[(size_t) b] / cnt[(size_t) b]);
        out.onsets = pickOnsets(hopEnergy.data(), hops, out.fileSampleRate, 0.05,
                                [](int k) { return std::max(0, (k - 1) * kOnsetHop); });
        out.ready = true;
    }

    std::map<std::string, Entry> cache_;
    std::vector<std::function<void()>> readyCbs_;
};

}
