#pragma once
#include <atomic>
#include <string>

#include <juce_core/juce_core.h>

#include "hum/Capabilities.h"
#include "hum/Organism.h"

namespace hum {

class SerialIn : public Organism, public ControlSource, public LevelMeterSource,
                 private juce::Thread {
public:
    SerialIn() : juce::Thread("hum-serial-in") {}
    ~SerialIn() override;

    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 2; }
    void prepare(double sampleRate, int maxBlock) override;
    void reset() override;
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    int controlValues(ControlVal* out, int capacity) const override {
        const ControlVal vals[2] = {{"a", juce::jlimit(0.0f, 1.0f, latest_[0].load())},
                                    {"b", juce::jlimit(0.0f, 1.0f, latest_[1].load())}};
        const int n = capacity < 2 ? capacity : 2;
        for (int i = 0; i < n; ++i) out[i] = vals[i];
        return n;
    }

    int meterChannels() const override { return 2; }
    float meterLevel(int ch) const override {
        return juce::jlimit(0.0f, 1.0f, std::abs(latest_[ch & 1].load()));
    }

private:
    void run() override;
    void closePort();

    std::atomic<float> latest_[2]{0.0f, 0.0f};
    std::atomic<int> baud_{115200};
    std::atomic<int> deviceIndex_{1};
    std::atomic<bool> ascii_{true};

    juce::SpinLock portLock_;
    std::string portPath_;
    std::string cachedText_;

    float smoothed_[2]{0.0f, 0.0f};
    float smoothK_ = 1.0f;

    int fd_ = -1;
    std::string openedPath_;
    int openedBaud_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SerialIn)
};

}
