// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <memory>
#include <string>

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>

namespace hum {

class LiveWavWriter {
public:
    LiveWavWriter() : thread_("hum-wav-writer") {}
    ~LiveWavWriter() { stop(); }

    bool start(const std::string& path, int numChannels, double sampleRate, bool append = false);

    static constexpr int kFifoSamples = 32768;
    bool write(const float* const* data, int numSamples);

    void stop();

    bool active() const { return writer_ != nullptr; }

private:
    juce::TimeSliceThread thread_;
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> writer_;
    int channels_ = 0;
};

}
