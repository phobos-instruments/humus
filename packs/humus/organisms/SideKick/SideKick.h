// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <atomic>

#include "hum/caps/Graph.h"
#include "hum/Organism.h"
#include "hum/dsp/GainShape.h"
#include "hum/dsp/Prepared.h"

namespace hum {

class SideKick : public Organism, public ControlSource {
public:
    int numAudioInputs() const override { return 2; }
    int numAudioOutputs() const override { return 2; }
    void prepare(double sampleRate, int) override {
        sampleRate_ = sampleRate;
        syncShape();
        reset();
    }
    void reset() override { g_ = 1.0; }
    void loadFrom(const OrganismState& state) override {
        Organism::loadFrom(state);
        syncShape();
    }
    void onTextChanged(const std::string& param, const std::string& text) override {
        if (param != "Shape") return;
        appliedText_ = text;
        pendingShape_.publish(shapeFor(text));
    }
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport&) override;

    int controlValues(ControlVal* out, int capacity) const override {
        if (capacity < 1) return 0;
        out[0] = {"gain", gainOut_.load(std::memory_order_relaxed)};
        if (capacity < 2) return 1;
        out[1] = {"phase", phaseOut_.load(std::memory_order_relaxed)};
        return 2;
    }

private:
    static GainShape shapeFor(const std::string& text) {
        GainShape shape = gainShapePreset(0);
        if (!text.empty()) decodeGainShape(text.c_str(), shape);
        return shape;
    }
    void syncShape() {
        const std::string text = params.getText("Shape");
        if (text == appliedText_ && shape_.n > 0) return;
        appliedText_ = text;
        shape_ = shapeFor(text);
    }

    GainShape shape_;
    Prepared<GainShape> pendingShape_;
    std::string appliedText_;
    double g_ = 1.0;
    std::atomic<float> gainOut_{1.0f};
    std::atomic<float> phaseOut_{0.0f};
};

}
