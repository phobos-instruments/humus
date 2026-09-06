#pragma once
#include <atomic>

#include "hum/Capabilities.h"
#include "hum/Organism.h"

namespace hum {

class Button : public Organism, public ControlSource, public MidiNode {
public:
    int numAudioInputs() const override { return 1; }
    int numAudioOutputs() const override { return 1; }
    void prepare(double sampleRate, int) override;
    void reset() override;
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    int controlValues(ControlVal* out, int capacity) const override {
        if (capacity < 1) return 0;
        out[0] = {"value", ctl_.load(std::memory_order_relaxed)};
        return 1;
    }

    int numMidiInputs() const override { return 0; }
    int numMidiOutputs() const override { return 1; }
    void deliverMidi(int, const MidiEvent*, int) override {}
    int collectMidi(int, MidiEvent* out, int capacity) override;

    bool isOn() const { return on_; }

private:
    void emit(bool on);

    bool pressed_ = false;
    bool on_ = false;
    bool primed_ = false;
    int soundingNote_ = -1;
    float smoothed_ = 0.0f;
    std::atomic<float> ctl_{0.0f};
    MidiEvent pending_[2];
    int pendingCount_ = 0;
};

}
