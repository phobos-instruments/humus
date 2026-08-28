#pragma once
#include <atomic>
#include <string>

#include <juce_audio_basics/juce_audio_basics.h>

#include "hum/Capabilities.h"
#include "hum/Organism.h"
#include "hum/dsp/LevelMeter.h"

namespace hum {

class SoundIn : public Organism, public HardwareIn, public LevelMeterSource,
               public FileLoader {
public:
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 2; }
    int channel() const override { return (int) params.get("Channel", 1.0) - 1; }
    int channelCount() const override { return 2; }

    void prepare(double sampleRate, int maxBlock) override;
    void reset() override {
        readPos_ = 0;
        meter_.reset();
        gainSm_ = (float) params.get("Gain", 1.0);
    }
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    void loadFromFile(const std::string& uri) override;

    int meterChannels() const override { return 2; }
    float meterLevel(int ch) const override { return meter_.level(ch); }

private:
    void applyPending();
    void applyGain(float* const* out, int numOut, int numSamples);

    juce::AudioBuffer<float> file_;
    int64_t readPos_ = 0;
    bool loop_ = true;
    float gainSm_ = 1.0f;
    LevelMeter meter_;

    juce::CriticalSection loadLock_;
    juce::AudioBuffer<float> pending_;
    std::atomic<bool> hasPending_{false};
    std::string loadedUri_;
};

}
