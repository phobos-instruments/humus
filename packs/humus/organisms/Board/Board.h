// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

#include <juce_core/juce_core.h>

#include "hum/caps/Audio.h"
#include "hum/caps/Graph.h"
#include "hum/Firmata.h"
#include "hum/Organism.h"
#include "hum/SerialPort.h"

namespace hum {

class Board : public Organism, public ControlSource, public LevelMeterSource, public PinKinds,
              public TextSource, private juce::Thread {
public:
    static constexpr int kOutlets = 6;
    static constexpr int kInlets = 6;
    static constexpr int kChannels = firmata::kMaxChannels;

    Board() : juce::Thread("hum-board") {}
    ~Board() override;

    bool controlOutlet(int) const override { return true; }
    bool controlInlet(const std::string& param) const override;
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    void prepare(double sampleRate, int maxBlock) override;
    void loadFrom(const OrganismState& state) override {
        Organism::loadFrom(state);
        syncPort(params.getText("Port"));
    }
    void onTextChanged(const std::string& param, const std::string& text) override {
        if (param == "Port") syncPort(text);
    }
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    int controlValues(ControlVal* out, int capacity) const override;
    int textLines(std::string* out, int capacity) const override;

    int meterChannels() const override { return outlets_.load(); }
    float meterLevel(int ch) const override { return outletValue(juce::jlimit(0, kOutlets - 1, ch)); }

    bool connectedForTest() const { return configured_.load(); }

private:
    void run() override;
    bool ensureOpen();
    void onOpened();
    void takeEvent(const firmata::Event& ev);
    void configure();
    void pumpReports();
    void pumpOutputs();
    bool send(const std::uint8_t* bytes, int n);
    void readParams();
    float outletValue(int outlet) const {
        return analog_[(size_t) channel_[(size_t) outlet].load()].load();
    }

    void syncPort(const std::string& text) {
        const juce::SpinLock::ScopedLockType sl(portLock_);
        portPath_ = text;
    }

    std::atomic<float> analog_[kChannels]{};
    std::atomic<int> channel_[kOutlets]{};
    std::atomic<float> inlet_[kInlets]{};
    std::atomic<int> pin_[kInlets]{};
    std::atomic<int> pwm_[kInlets]{};
    std::atomic<int> baud_{57600};
    std::atomic<bool> reset_{true};
    std::atomic<int> deviceIndex_{1};
    std::atomic<int> sampleMs_{19};
    std::atomic<int> outlets_{2};
    std::atomic<int> inlets_{2};
    std::atomic<bool> configured_{false};
    std::atomic<unsigned> reconnects_{0};
    bool reconnectHeld_ = false;
    unsigned seenReconnects_ = 0;

    juce::SpinLock portLock_;
    std::string portPath_;
    std::string status_;
    void note(const std::string& status) {
        const juce::SpinLock::ScopedLockType sl(portLock_);
        status_ = status;
    }

    std::shared_ptr<serial::Link> link_;
    unsigned seenGeneration_ = 0;
    firmata::Parser parser_;
    std::uint32_t openedAt_ = 0;
    bool versionSeen_ = false;
    bool mappingKnown_ = false;
    bool reported_[kChannels]{};
    bool analogModeSent_[kChannels]{};
    int sentPin_[kInlets]{};
    int sentPwm_[kInlets]{};
    int sentValue_[kInlets]{};
    int sentSampleMs_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Board)
};

}
