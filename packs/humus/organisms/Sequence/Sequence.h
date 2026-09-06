#pragma once
#include <array>
#include <vector>

#include "hum/Capabilities.h"
#include "hum/Organism.h"
#include "hum/Pattern.h"

namespace hum {

class Sequence : public Organism, public MidiNode {
public:
    static constexpr int kRows = 8;
    static constexpr int kMaster = kRows;
    static constexpr int kPorts = kRows + 1;

    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    int numMidiInputs() const override { return 0; }
    int numMidiOutputs() const override { return kPorts; }

    void prepare(double sampleRate, int) override {
        sampleRate_ = sampleRate;
        reset();
    }
    void reset() override {
        for (auto& c : outCount_) c = 0;
        offCount_ = 0;
    }

    void setPattern(const Pattern& p) override { pattern_ = p; }
    void loadFrom(const OrganismState& state) override {
        Organism::loadFrom(state);
        setPattern(state.pattern);
    }

    void deliverMidi(int, const MidiEvent*, int) override {}
    int collectMidi(int port, MidiEvent* out, int capacity) override {
        if (port < 0 || port >= kPorts) return 0;
        const int n = std::min(outCount_[(size_t) port], capacity);
        for (int i = 0; i < n; ++i) out[i] = outEvents_[(size_t) port][(size_t) i];
        outCount_[(size_t) port] = 0;
        return n;
    }

    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

private:
    void emit(int port, int offset, bool on, int note, int vel);

    Pattern pattern_;
    std::array<std::array<MidiEvent, 64>, kPorts> outEvents_;
    std::array<int, kPorts> outCount_ = {};
    struct PendingOff { int port; int note; long samplesLeft; };
    std::array<PendingOff, 64> offs_;
    int offCount_ = 0;
};

}
