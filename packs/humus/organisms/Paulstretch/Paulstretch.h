// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>

#include "hum/caps/Files.h"
#include "hum/Organism.h"
#include "hum/dsp/PaulstretchCore.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class Paulstretch : public Organism, public FileTransportCap {
public:
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 2; }

    void prepare(double sampleRate, int maxBlock) override;
    void reset() override;
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    int64_t playbackPositionSamples() const override { return playPos_.load(); }
    int64_t fileLengthSamples() const override { return fileLen_.load(); }
    double  playbackSampleRate() const override { return fileSr_.load(); }
    void    requestSeekSamples(int64_t s) override { seekReq_.store(s < 0 ? 0 : s); }
    void    loadFromFile(const std::string& uri) override;

private:
    void applyPending();
    void synthFrame(double stretch, bool loop);

    juce::AudioBuffer<float> file_;
    std::string loadedUri_;
    juce::CriticalSection loadLock_;
    juce::AudioBuffer<float> pending_;
    std::atomic<bool> hasPending_{false};
    std::atomic<int64_t> playPos_{0};
    std::atomic<int64_t> fileLen_{0};
    std::atomic<int64_t> seekReq_{-1};
    std::atomic<double> fileSr_{kDefaultSampleRate};

    std::array<PaulstretchFrame, 2> frame_;
    std::array<std::vector<float>, 2> outQ_;
    std::array<std::vector<float>, 2> halfBuf_;
    int qHead_ = 0, qTail_ = 0, qCount_ = 0;

    double srcPos_ = 0.0;
    bool done_ = false;
    std::uint32_t rng_ = 0x2545f491u;
};

}
