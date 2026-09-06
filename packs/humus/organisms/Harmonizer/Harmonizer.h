#pragma once
#include <algorithm>
#include <array>
#include <atomic>

#include "hum/Capabilities.h"
#include "hum/HeldNotes.h"
#include "hum/Organism.h"
#include "hum/Scale.h"
#include "hum/dsp/PitchShifter.h"
#include "hum/dsp/PitchTrack.h"

namespace hum {

class Harmonizer : public Organism, public PitchDetectSource, public MidiNode {
public:
    static constexpr int kVoices = 4;

    int numAudioInputs() const override { return 1; }
    int numAudioOutputs() const override { return 2; }
    int numMidiInputs() const override { return 1; }
    int numMidiOutputs() const override { return 0; }

    void prepare(double sampleRate, int) override;
    void reset() override;
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    void deliverMidi(int, const MidiEvent* events, int count) override {
        stagedCount_ = std::min(count, (int) staged_.size());
        for (int i = 0; i < stagedCount_; ++i) staged_[(size_t) i] = events[i];
    }
    int collectMidi(int, MidiEvent*, int) override { return 0; }

    float detectedHz() const override { return hz_.load(std::memory_order_relaxed); }
    float detectClarity() const override { return clarity_.load(std::memory_order_relaxed); }
    float detectLevel() const override { return level_.load(std::memory_order_relaxed); }
    int detectedNote() const override { return note_.load(std::memory_order_relaxed); }

    static int scaleStep(int baseNote, int key, int scaleId, int degrees);
    static int stableNote(double exactMidi, int current);

private:
    void chooseTargets(bool voiced, int rounded);

    PitchTracker tracker_;
    std::array<PitchShifter, kVoices> shift_;
    std::array<double, kVoices> ratio_{};
    std::array<double, kVoices> targetRatio_{};
    std::array<float, kVoices> gain_{};
    std::array<float, kVoices> targetGain_{};
    std::array<float, kVoices> voiceLevel_{};
    std::array<double, kVoices> humPhase_{};

    HeldNotes held_;
    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> staged_;
    int stagedCount_ = 0;
    int refNote_ = -1;
    int voicedHold_ = 0;
    int window_ = 0;

    std::atomic<float> hz_{0.0f};
    std::atomic<float> clarity_{0.0f};
    std::atomic<float> level_{0.0f};
    std::atomic<int> note_{-1};
};

}
