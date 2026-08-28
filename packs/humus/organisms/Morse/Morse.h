#pragma once
#include <array>
#include <string>
#include <vector>

#include "Morse/MorseCode.h"
#include "hum/Capabilities.h"
#include "hum/Organism.h"

namespace hum {

class Morse : public Organism, public MidiNode {
public:
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 1; }
    int numMidiInputs() const override { return 0; }
    int numMidiOutputs() const override { return 1; }

    void deliverMidi(int, const MidiEvent*, int) override {}
    int collectMidi(int, MidiEvent* out, int capacity) override {
        const int n = std::min(outCount_, capacity);
        for (int i = 0; i < n; ++i) out[i] = outEvents_[(size_t) i];
        outCount_ = 0;
        return n;
    }

    void prepare(double sampleRate, int) override {
        sampleRate_ = sampleRate;
        reset();
    }
    void reset() override {
        seg_ = 0;
        segPos_ = 0;
        phase_ = 0.0;
        amp_ = 0.0f;
        done_ = false;
    }

    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

private:
    void emit(int offset, bool on, int note);

    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> outEvents_;
    int outCount_ = 0;
    bool keyWasOn_ = false;
    int midiNote_ = 74;

    std::vector<morse::Seg> pattern_;
    std::string cachedText_;
    size_t seg_ = 0;
    long segPos_ = 0;
    double phase_ = 0.0;
    float amp_ = 0.0f;
    bool done_ = false;
};

}
