// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>

#include "hum/caps/Files.h"
#include "hum/Organism.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class FilePlayer : public Organism, public FileTransportCap {
public:
    static constexpr int kMaxOutputs = 32;
    static constexpr int kStereo = 2;

    explicit FilePlayer(int outputs = 0)
        : multi_(outputs > 0), rows_(multi_ ? std::clamp(outputs, 1, kMaxOutputs) : 1),
          outlets_(multi_ ? rows_ : kStereo),
          files_((size_t) rows_), pending_((size_t) rows_),
          pendingSet_((size_t) rows_, 0), loadedUri_((size_t) rows_),
          loadedLen_((size_t) rows_, 0) {}

    std::string fileParam(int row) const {
        return multi_ ? "File_" + std::to_string(row + 1) : std::string("File");
    }

    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return outlets_; }

    void prepare(double sampleRate, int maxBlock) override;
    void reset() override;
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    int64_t playbackPositionSamples() const override { return playPos_.load(); }
    int64_t fileLengthSamples() const override { return fileLen_.load(); }
    double  playbackSampleRate() const override { return fileSr_.load(); }
    std::string playSwitch() const override { return "Active"; }
    void    requestSeekSamples(int64_t s) override { seekReq_.store(s < 0 ? 0 : s); }

    void    loadFromFile(const std::string& uri) override;

private:
    void applyPending();
    void renderFrame(int n, float* const* out, int numOut);

    bool multi_;
    int rows_;
    int outlets_;
    std::vector<juce::AudioBuffer<float>> files_;
    int64_t length_ = 0;
    int64_t readPos_ = 0;
    int64_t delayRemaining_ = 0;
    bool prevActive_ = false;
    double sampleRate_ = kDefaultSampleRate;

    juce::CriticalSection loadLock_;
    std::vector<juce::AudioBuffer<float>> pending_;
    std::vector<char> pendingSet_;
    std::atomic<bool> hasPending_ {false};
    std::vector<std::string> loadedUri_;
    std::vector<int64_t> loadedLen_;

    std::atomic<int64_t> playPos_ {0};
    std::atomic<int64_t> fileLen_ {0};
    std::atomic<int64_t> seekReq_ {-1};
    std::atomic<double>  fileSr_  {kDefaultSampleRate};
};

}
