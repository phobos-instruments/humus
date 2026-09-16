// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <atomic>

#include "hum/caps/Audio.h"
#include "hum/caps/Midi.h"
#include "hum/Organism.h"
#include "hum/dsp/PitchTrack.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class Decomposer : public Organism, public MidiNode, public PitchDetectSource {
public:
    int numAudioInputs() const override { return 1; }
    int numAudioOutputs() const override { return 0; }
    void prepare(double sampleRate, int maxBlock) override;
    void reset() override;
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport&) override;

    int numMidiInputs() const override { return 0; }
    int numMidiOutputs() const override { return 1; }
    void deliverMidi(int, const MidiEvent*, int) override {}
    int collectMidi(int, MidiEvent* out, int capacity) override {
        const int n = std::min(outCount_, capacity);
        for (int i = 0; i < n; ++i) out[i] = out_[(size_t) i];
        outCount_ = 0;
        return n;
    }

    float detectedHz() const override { return rHz_.load(std::memory_order_relaxed); }
    float detectClarity() const override { return rClar_.load(std::memory_order_relaxed); }
    float detectLevel() const override { return rLevel_.load(std::memory_order_relaxed); }
    int detectedNote() const override { return rNote_.load(std::memory_order_relaxed); }

private:
    void emit(int offset, bool on, int note, int vel);
    void segmentMono(int blockEndOffset);
    void allNotesOff(int offset);
    int medianNote(int raw);

    double sampleRate_ = kDefaultSampleRate;
    int channel_ = 1;
    int loNote_ = 12, hiNote_ = 108;
    int confirmFrames_ = 2, releaseFrames_ = 3;

    PitchTracker tracker_;
    int curNote_ = -1;
    int candNote_ = -1, candFrames_ = 0;
    int silentFrames_ = 0;
    std::array<int, 3> noteHist_{-1, -1, -1};
    int histFill_ = 0;
    double slowEnv_ = 0.0;
    int refractory_ = 0;

    std::atomic<float> rHz_{0.0f}, rClar_{0.0f}, rLevel_{0.0f};
    std::atomic<int> rNote_{-1};

    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> out_;
    int outCount_ = 0;
};

}
