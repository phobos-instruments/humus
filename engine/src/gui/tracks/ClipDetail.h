// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "core/app/AppPaths.h"
#include <juce_audio_formats/juce_audio_formats.h>

namespace hum::clipdetail {

class SampleWindow {
public:
    const std::vector<float>& read(const std::string& path, std::int64_t from, std::int64_t to, unsigned stamp = 0) {
        if (path != path_ || stamp != stamp_) open(path, stamp);
        from = std::max<std::int64_t>(0, from);
        to = std::min(to, length_);
        if (length_ <= 0 || to <= from) { mono_.clear(); from_ = 0; return mono_; }
        if (from == from_ && (std::int64_t) mono_.size() == to - from) return mono_;
        const auto reader = readerFor(path_);
        if (!reader) { mono_.clear(); from_ = 0; return mono_; }
        const int n = (int) std::min<std::int64_t>(to - from, kMaxWindow);
        juce::AudioBuffer<float> buf((int) reader->numChannels, n);
        reader->read(&buf, 0, n, from, true, true);
        mono_.assign((size_t) n, 0.0f);
        for (int c = 0; c < buf.getNumChannels(); ++c) {
            const float* s = buf.getReadPointer(c);
            for (int i = 0; i < n; ++i) mono_[(size_t) i] += s[i];
        }
        const float k = 1.0f / (float) std::max(1, buf.getNumChannels());
        for (auto& v : mono_) v *= k;
        from_ = from;
        return mono_;
    }
    std::int64_t from() const { return from_; }
    double sampleRate() const { return rate_; }
    std::int64_t length() const { return length_; }

private:
    static constexpr std::int64_t kMaxWindow = 1 << 20;

    static std::unique_ptr<juce::AudioFormatReader> readerFor(const std::string& path) {
        std::string uri = path;
        if (uri.rfind("file://", 0) == 0) uri = uri.substr(7);
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        return std::unique_ptr<juce::AudioFormatReader>(fm.createReaderFor(fileAt(uri)));
    }

    void open(const std::string& path, unsigned stamp) {
        path_ = path;
        stamp_ = stamp;
        mono_.clear();
        from_ = 0;
        const auto reader = readerFor(path);
        length_ = reader ? reader->lengthInSamples : 0;
        rate_ = reader ? reader->sampleRate : 0.0;
    }

    std::string path_;
    unsigned stamp_ = 0;
    std::vector<float> mono_;
    std::int64_t from_ = 0, length_ = 0;
    double rate_ = 0.0;
};

inline juce::String gainDb(double gain) {
    if (gain <= 0.0) return "-inf dB";
    const double db = 20.0 * std::log10(gain);
    return (db >= 0.0 ? "+" : "") + juce::String(db, 1) + " dB";
}

}
