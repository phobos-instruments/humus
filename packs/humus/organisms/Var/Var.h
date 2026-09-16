// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <atomic>
#include <string>

#include <juce_core/juce_core.h>

#include "hum/caps/Graph.h"
#include "hum/Organism.h"

namespace hum {

class Var : public Organism, public ControlSource, public PinKinds, public Tagged {
public:
    bool controlOutlet(int) const override { return true; }
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    void prepare(double sampleRate, int maxBlock) override;
    void loadFrom(const OrganismState& state) override {
        Organism::loadFrom(state);
        syncName(params.getText("Name"));
    }
    void onTextChanged(const std::string& param, const std::string& text) override {
        if (param == "Name") syncName(text);
    }
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    int controlValues(ControlVal* out, int capacity) const override {
        if (capacity < 1) return 0;
        out[0] = {"value", live_.load(std::memory_order_relaxed)};
        return 1;
    }

    std::string tag() const override {
        const juce::SpinLock::ScopedLockType sl(nameLock_);
        return name_;
    }

private:
    void syncName(const std::string& text) {
        const juce::SpinLock::ScopedLockType sl(nameLock_);
        name_ = text;
    }

    mutable juce::SpinLock nameLock_;
    std::string name_;
    std::atomic<float> live_{0.0f};
    float lastTyped_ = 0.0f;
    bool primed_ = false;
    unsigned seenStamp_ = 0;
};

}
