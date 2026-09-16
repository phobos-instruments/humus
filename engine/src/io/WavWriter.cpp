// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "io/WavWriter.h"

#include <memory>

#include <juce_audio_formats/juce_audio_formats.h>

#include "io/Mp3Writer.h"

namespace hum {

namespace {

std::unique_ptr<juce::AudioFormat> formatFor(const juce::File& f) {
    if (f.hasFileExtension("flac")) return std::make_unique<juce::FlacAudioFormat>();
    if (f.hasFileExtension("ogg")) return std::make_unique<juce::OggVorbisAudioFormat>();
    return std::make_unique<juce::WavAudioFormat>();
}

int qualityFor(const juce::File& f) {
    return f.hasFileExtension("ogg") ? 7 : 0;
}

}

bool writeSound(const std::string& path,
                const std::vector<std::vector<float>>& channels,
                double sampleRate, int bitsPerSample, int mp3Rate) {
    if (channels.empty() || channels[0].empty()) return false;
    const int numCh = (int) channels.size();
    const juce::File f(juce::String(juce::CharPointer_UTF8(path.c_str())));
    if (f.hasFileExtension("mp3")) return writeMp3(path, channels, sampleRate, mp3Rate);
    f.deleteFile();
    std::unique_ptr<juce::FileOutputStream> stream(f.createOutputStream());
    if (!stream) return false;

    auto fmt = formatFor(f);
    int bits = bitsPerSample;
    if (const auto allowed = fmt->getPossibleBitDepths(); !allowed.isEmpty()
        && !allowed.contains(bits))
        bits = allowed.contains(24) ? 24 : allowed[allowed.size() - 1];
    std::unique_ptr<juce::AudioFormatWriter> writer(
        fmt->createWriterFor(stream.get(), sampleRate, (unsigned) numCh, bits, {},
                             qualityFor(f)));
    if (!writer) return false;
    stream.release();

    std::vector<const float*> ptrs((size_t) numCh);
    for (int c = 0; c < numCh; ++c) ptrs[(size_t) c] = channels[(size_t) c].data();
    return writer->writeFromFloatArrays(ptrs.data(), numCh, (int) channels[0].size());
}

bool writeWav(const std::string& path,
              const std::vector<std::vector<float>>& channels,
              double sampleRate, int bitsPerSample) {
    if (channels.empty()) return false;
    const int numCh = (int) channels.size();
    const int64_t numSamples = (int64_t) channels[0].size();

    juce::File f(juce::String(juce::CharPointer_UTF8(path.c_str())));
    f.deleteFile();
    std::unique_ptr<juce::FileOutputStream> stream(f.createOutputStream());
    if (!stream) return false;

    juce::WavAudioFormat fmt;
    std::unique_ptr<juce::AudioFormatWriter> writer(
        fmt.createWriterFor(stream.get(), sampleRate, (unsigned) numCh,
                            bitsPerSample, {}, 0));
    if (!writer) return false;
    stream.release();

    std::vector<const float*> ptrs(numCh);
    for (int c = 0; c < numCh; ++c) ptrs[c] = channels[c].data();
    bool ok = writer->writeFromFloatArrays(ptrs.data(), numCh, (int) numSamples);
    return ok;
}

}
