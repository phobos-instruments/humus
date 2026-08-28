#pragma once
#include <atomic>
#include <string>

#include <juce_core/juce_core.h>

#include "hum/Organism.h"

namespace hum {

class SerialOut : public Organism, private juce::Thread {
public:
    SerialOut() : juce::Thread("hum-serial-out") {}
    ~SerialOut() override;

    int numAudioInputs() const override { return 2; }
    int numAudioOutputs() const override { return 0; }
    void prepare(double sampleRate, int maxBlock) override;
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

private:
    void run() override;
    void closePort();

    std::atomic<float> latest_[2]{0.0f, 0.0f};
    std::atomic<float> rateHz_{30.0f};
    std::atomic<int> baud_{115200};
    std::atomic<int> deviceIndex_{1};
    std::atomic<bool> ascii_{true};

    juce::SpinLock portLock_;
    std::string portPath_;
    std::string cachedText_;

    int fd_ = -1;
    std::string openedPath_;
    int openedBaud_ = 0;
};

}
