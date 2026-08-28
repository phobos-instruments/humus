#pragma once
#include <atomic>
#include <cstdint>

#include <juce_audio_basics/juce_audio_basics.h>

#include "hum/Capabilities.h"
#include "hum/Organism.h"

namespace hum {

class FilePlayer : public Organism, public FileTransportCap {
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

    juce::AudioBuffer<float> file_;
    std::string loadedUri_;
    int64_t readPos_ = 0;
    int64_t delayRemaining_ = 0;
    bool prevActive_ = false;
    double sampleRate_ = 44100.0;

    juce::CriticalSection loadLock_;
    juce::AudioBuffer<float> pending_;
    std::atomic<bool> hasPending_ {false};

    std::atomic<int64_t> playPos_ {0};
    std::atomic<int64_t> fileLen_ {0};
    std::atomic<int64_t> seekReq_ {-1};
    std::atomic<double>  fileSr_  {44100.0};
};

}
