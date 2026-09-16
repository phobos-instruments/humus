// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <algorithm>
#include <atomic>
#include <memory>
#include <string>

#include <juce_core/juce_core.h>

#include "hum/caps/Graph.h"
#include "hum/caps/Params.h"
#include "hum/Organism.h"
#include "hum/SerialPort.h"

namespace hum {

class SerialOut : public Organism, public PinKinds, public SocketSources, public TextSource,
                  private juce::Thread {
public:
    static constexpr int kValues = serial::kMaxValues;
    enum SendMode { kContinuous = 0, kOnChange = 1, kOnTrigger = 2 };

    SerialOut() : juce::Thread("hum-serial-out") {}
    ~SerialOut() override;

    bool controlOutlet(int) const override { return false; }
    bool controlInlet(const std::string& param) const override;
    void setSocketSource(const std::string& param, const std::string& sourceName) override;
    std::string socketSourceForTest(int slot) const;
    int textLines(std::string* out, int capacity) const override;

    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    void prepare(double sampleRate, int maxBlock) override;
    void loadFrom(const OrganismState& state) override {
        Organism::loadFrom(state);
        syncText("Port", params.getText("Port"));
        syncText("Format", params.getText("Format"));
    }
    void onTextChanged(const std::string& param, const std::string& text) override {
        syncText(param, text);
    }
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

private:
    void run() override;
    bool ensureOpen();
    bool shouldSend(const serial::Slot* slots, int count);
    std::string currentFormat();
    void readSlots();
    void readTrigger();

    std::atomic<float> latest_[kValues]{};
    std::atomic<float> scale_[kValues]{};
    std::atomic<bool> integer_[kValues]{};
    std::atomic<float> rateHz_{30.0f};
    std::atomic<int> baud_{115200};
    std::atomic<int> frame_{serial::kFrame8N1};
    std::atomic<bool> reset_{true};
    std::atomic<int> deviceIndex_{1};
    std::atomic<unsigned> reconnects_{0};
    bool reconnectHeld_ = false;
    unsigned seenReconnects_ = 0;
    std::atomic<int> sendMode_{kContinuous};
    std::atomic<int> values_{2};
    std::atomic<unsigned> triggers_{0};
    std::atomic<int> lines_{1};
    std::atomic<int> message_{0};
    bool lastTrigger_ = false;
    bool primed_ = false;

    void syncText(const std::string& param, const std::string& text) {
        const juce::SpinLock::ScopedLockType sl(textLock_);
        if (param == "Port") portPath_ = text;
        else if (param == "Format") {
            format_ = text;
            lines_.store(1 + (int) std::count(text.begin(), text.end(), '\n'));
        }
    }

    juce::SpinLock textLock_;
    std::string portPath_;
    std::string format_;
    std::string sourceNames_[kValues];
    std::string status_;
    std::string lastSent_;
    void note(const std::string& status, const std::string* sent) {
        const juce::SpinLock::ScopedLockType sl(textLock_);
        status_ = status;
        if (sent != nullptr) lastSent_ = *sent;
    }

    std::shared_ptr<serial::Link> link_;
    float sent_[kValues]{};
    bool sentAny_ = false;
    unsigned sentTriggers_ = 0;
    int sentMessage_ = -1;
};

}
