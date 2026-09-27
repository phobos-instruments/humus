// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/browser/SoundProbe.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>

#include <juce_audio_formats/juce_audio_formats.h>

#include "core/analysis/BeatDetector.h"
#include "core/analysis/KeyDetector.h"
#include "core/app/AppPaths.h"
#include "core/browser/FileKind.h"
#include "core/browser/NameFacts.h"

namespace hum::browser {

namespace {

constexpr double kAnalysedSeconds = 30.0;
constexpr double kSureKey = 0.5;

std::vector<std::uint8_t> peaksOf(juce::AudioFormatReader& reader) {
    std::vector<std::uint8_t> out((size_t) kPeakCount, 0);
    const auto total = reader.lengthInSamples;
    const int channels = (int) std::min<unsigned>(reader.numChannels, 2u);
    float loudest = 0.0f;
    std::vector<float> raw((size_t) kPeakCount, 0.0f);
    for (int b = 0; b < kPeakCount; ++b) {
        const auto from = total * b / kPeakCount;
        const auto to = total * (b + 1) / kPeakCount;
        if (to <= from) continue;
        juce::Range<float> levels[2];
        reader.readMaxLevels(from, to - from, levels, channels);
        float peak = 0.0f;
        for (int c = 0; c < channels; ++c)
            peak = std::max({peak, std::abs(levels[c].getStart()), std::abs(levels[c].getEnd())});
        raw[(size_t) b] = peak;
        loudest = std::max(loudest, peak);
    }
    if (loudest <= 0.0f) return out;
    for (int b = 0; b < kPeakCount; ++b)
        out[(size_t) b] = (std::uint8_t) std::lround(255.0f * std::sqrt(raw[(size_t) b] / loudest));
    return out;
}

void detectLoopFacts(juce::AudioFormatReader& reader, Facts& f) {
    const int n = (int) std::min<std::int64_t>(reader.lengthInSamples, (std::int64_t) (reader.sampleRate * kAnalysedSeconds));
    if (n <= 0) return;
    juce::AudioBuffer<float> buf((int) std::max(1u, reader.numChannels), n);
    reader.read(&buf, 0, n, 0, true, true);
    if (f.bpm <= 0.0)
        if (const auto beat = detectBeat(buf, reader.sampleRate); beat.confidence > 0.0) f.bpm = std::round(beat.bpm * 10.0) / 10.0;
    if (f.key.empty())
        if (const auto key = detectKey(buf, reader.sampleRate); key.confidence >= kSureKey) f.key = key.name;
}

}

Facts probeSound(const std::string& path) {
    Facts f;
    f.probed = true;
    const auto named = factsFromName(fileName(path));
    f.bpm = named.bpm;
    f.key = named.key;
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(fileAt(path)));
    if (!reader || reader->sampleRate <= 0.0 || reader->lengthInSamples <= 0) return f;
    f.sampleRate = reader->sampleRate;
    f.channels = (int) reader->numChannels;
    f.seconds = (double) reader->lengthInSamples / reader->sampleRate;
    if (f.seconds <= kPeaksLongest) f.peaks = peaksOf(*reader);
    if (f.seconds >= kLoopShortest && f.seconds <= kLoopLongest && kindOfFile(path) == Kind::Sound)
        detectLoopFacts(*reader, f);
    return f;
}

}
