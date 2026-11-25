// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <atomic>
#include <string>

#include "LFO/LfoFormula.h"
#include "LFO/LfoTable.h"
#include "LFO/LfoTrace.h"
#include "hum/caps/Graph.h"
#include "hum/Organism.h"
#include "hum/dsp/Lfo.h"

namespace hum {

inline constexpr int kLfoFunction = 6;

class LfoGen : public Organism, public ControlSource, public PinKinds, public MotionTrace {
public:
    bool controlOutlet(int) const override { return true; }
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    void prepare(double sampleRate, int) override {
        sampleRate_ = sampleRate;
        formula_.sync(params.getText("Formula"));
        table_.sync(params.getText("Table"));
        table_.glideTo(params.get("Position", 0.0));
        table_.prepare(sampleRate);
        reset();
    }
    void loadFrom(const OrganismState& state) override {
        Organism::loadFrom(state);
        formula_.sync(params.getText("Formula"));
        table_.sync(params.getText("Table"));
    }
    void onTextChanged(const std::string& param, const std::string& text) override {
        if (param == "Formula") formula_.publish(text);
        if (param == "Table") table_.publish(text);
    }
    void reset() override;
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    Kind motionKind() const override { return kind_.load(std::memory_order_relaxed); }
    int motionRead(Point* out, int capacity) const override {
        return motionKind() == Kind::Cycle ? table_.readCycle(out, capacity) : trace_.read(out, capacity);
    }

    int controlValues(ControlVal* out, int capacity) const override {
        if (capacity < 1) return 0;
        out[0] = {"wave", ctl_.load(std::memory_order_relaxed)};
        if (capacity < 2) return 1;
        out[1] = {"phase", phase_.load(std::memory_order_relaxed)};
        return 2;
    }

private:
    LfoFormula::Moment momentFor(double rate, bool sync, double syncBeats, const Transport& transport);

    std::atomic<float> ctl_{0.5f};
    std::atomic<float> phase_{0.0f};
    std::atomic<Kind> kind_{Kind::Params};
    Lfo lfo_;
    unsigned rng_ = 0x1234abcdu;
    double held_ = 0.0;
    double prevPhase_ = 1.0;
    double seconds_ = 0.0, beats_ = 0.0;
    bool traced_ = false;
    LfoFormula formula_;
    LfoTable table_;
    LfoTrace trace_;
};

}
