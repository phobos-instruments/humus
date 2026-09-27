// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <juce_audio_formats/juce_audio_formats.h>

namespace hum {

constexpr int kVideoChannels = 2;

inline int videoChannels(int sourceChannels) {
    return std::clamp(sourceChannels, 1, kVideoChannels);
}

class FfmpegAudioFormat : public juce::AudioFormat {
public:
    explicit FfmpegAudioFormat(int track = 0);

    static bool available();
    static juce::StringArray containers();
    static std::string extraContainers();
    static std::vector<std::string> audioTrackNames(const juce::File& file);

    juce::Array<int> getPossibleSampleRates() override { return {}; }
    juce::Array<int> getPossibleBitDepths() override { return {}; }
    bool canDoStereo() override { return true; }
    bool canDoMono() override { return true; }
    bool isCompressed() override { return true; }

    juce::AudioFormatReader* createReaderFor(juce::InputStream* stream,
                                             bool deleteStreamIfOpeningFails) override;
    juce::AudioFormatWriter* createWriterFor(juce::OutputStream*, double, unsigned int,
                                             int, const juce::StringPairArray&, int) override {
        return nullptr;
    }

private:
    const int track_;
};

void installExtraSoundFormats();

}
