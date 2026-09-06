#pragma once
#include <algorithm>
#include <array>
#include <string>

#include "hum/Capabilities.h"
#include "hum/Organism.h"

namespace hum {

class Cluster : public Organism, public MidiNode {
public:
    static constexpr int kSlots = 8;
    static constexpr int kChordMax = 16;

    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    int numMidiInputs() const override { return 1; }
    int numMidiOutputs() const override { return 1; }

    void prepare(double sampleRate, int) override { sampleRate_ = sampleRate; reset(); }
    void reset() override;
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    void deliverMidi(int, const MidiEvent* events, int count) override {
        stagedCount_ = std::min(count, (int) staged_.size());
        for (int i = 0; i < stagedCount_; ++i) staged_[(size_t) i] = events[i];
    }
    int collectMidi(int, MidiEvent* out, int capacity) override {
        const int n = std::min(outCount_, capacity);
        for (int i = 0; i < n; ++i) out[i] = outEvents_[(size_t) i];
        outCount_ = 0;
        return n;
    }

    static int parseNotes(const std::string& text, int* out, int capacity);

private:
    struct Emitted {
        std::array<int, kChordMax> notes{};
        int count = 0;
        bool active = false;
    };

    void emit(int offset, bool on, int note, int velocity);
    void soundChord(const int* notes, int count, int velocity, int offset, Emitted& slot);
    void hushChord(Emitted& slot, int offset);
    void refreshSlotTexts();
    void hushAll(int offset);
    void handleFireEdges(int hold);
    void handleEvent(const MidiEvent& e, int mode, int hold, int triggerNote,
                     int followSlot, int velocityParam);

    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> staged_;
    int stagedCount_ = 0;
    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> outEvents_;
    int outCount_ = 0;

    std::array<std::string, kSlots> cachedText_;
    std::array<std::array<int, kChordMax>, kSlots> chord_{};
    std::array<int, kSlots> chordCount_{};

    std::array<Emitted, kSlots> pad_;
    std::array<Emitted, 128> follow_;
    Emitted followLatch_;
    std::array<unsigned char, 128> refs_{};
    std::array<bool, kSlots> lastFire_{};
    bool firePrimed_ = false;
    int lastMode_ = 0;
    int lastHold_ = 0;
};

}
