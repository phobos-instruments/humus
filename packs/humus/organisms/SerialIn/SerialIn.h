// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <atomic>
#include <cmath>
#include <memory>
#include <string>

#include <juce_core/juce_core.h>

#include "hum/caps/Audio.h"
#include "hum/caps/Graph.h"
#include "hum/Organism.h"
#include "hum/SerialPort.h"

namespace hum {

class SerialIn : public Organism, public ControlSource, public LevelMeterSource, public PinKinds,
                 public TextSource, private juce::Thread {
public:
    static constexpr int kValues = serial::kMaxValues;

    SerialIn() : juce::Thread("hum-serial-in") {}
    ~SerialIn() override;

    bool controlOutlet(int) const override { return true; }
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    void prepare(double sampleRate, int maxBlock) override;
    void loadFrom(const OrganismState& state) override {
        Organism::loadFrom(state);
        syncText("Port", params.getText("Port"));
        syncText("Parse", params.getText("Parse"));
    }
    void onTextChanged(const std::string& param, const std::string& text) override {
        syncText(param, text);
    }
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    int controlValues(ControlVal* out, int capacity) const override;
    int textLines(std::string* out, int capacity) const override;

    int meterChannels() const override { return values_.load(); }
    float meterLevel(int ch) const override {
        return juce::jlimit(0.0f, 1.0f, std::abs(latest_[(size_t) juce::jlimit(0, kValues - 1, ch)].load()));
    }

private:
    void run() override;
    bool ensureOpen();
    void takeBytes(const char* buf, int n);
    void takeLine();
    void takePackets();
    void publish(const serial::Reading* readings, const bool* captured);

    std::atomic<float> latest_[kValues]{};
    std::atomic<int> values_{2};
    std::atomic<int> baud_{115200};
    std::atomic<int> frame_{serial::kFrame8N1};
    std::atomic<bool> reset_{true};
    std::atomic<int> deviceIndex_{1};
    std::atomic<unsigned> reconnects_{0};
    bool reconnectHeld_ = false;
    unsigned seenReconnects_ = 0;
    std::atomic<bool> hasParse_{false};
    std::atomic<bool> binary_{false};
    std::atomic<unsigned> matches_{0};
    std::atomic<float> matchPulse_{0.0f};
    unsigned seenMatches_ = 0;

    void syncText(const std::string& param, const std::string& text) {
        const juce::SpinLock::ScopedLockType sl(textLock_);
        if (param == "Port") portPath_ = text;
        else if (param == "Parse") {
            parse_ = text;
            hasParse_.store(!text.empty());
            binary_.store(serial::isBinaryPattern(text));
        }
    }
    std::string currentParse();

    juce::SpinLock textLock_;
    std::string portPath_;
    std::string parse_;
    std::string status_;
    std::string lastGot_;
    bool lastFit_ = true;

    std::shared_ptr<serial::Link> link_;
    char line_[256];
    size_t lineLen_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SerialIn)
};

}
