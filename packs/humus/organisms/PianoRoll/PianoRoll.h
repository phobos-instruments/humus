// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <mutex>
#include <string>
#include <vector>

#include "hum/caps/Files.h"
#include "hum/caps/Midi.h"
#include "hum/Organism.h"
#include "hum/NoteSchedule.h"
#include "hum/Pattern.h"
#include "hum/PatternMatrix.h"

namespace hum {

class PianoRoll : public Organism, public MidiNode, public ClipArrangement,
                  public LiveMidiIn {
public:
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    void prepare(double sampleRate, int maxBlock) override;
    void reset() override;
    void loadFrom(const OrganismState& state) override;
    void setPattern(const Pattern& pattern) override {
        pattern_ = pattern;
        clipsDirty_ = true;
        patternEdited_ = true;
    }
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    int numMidiInputs() const override { return 0; }
    int numMidiOutputs() const override { return 1; }
    void deliverMidi(int, const MidiEvent*, int) override {}
    int collectMidi(int, MidiEvent* out, int capacity) override {
        int n = outCount_ < capacity ? outCount_ : capacity;
        for (int i = 0; i < n; ++i) out[i] = outEvents_[(size_t) i];
        std::unique_lock<std::mutex> g(liveMutex_, std::try_to_lock);
        if (g.owns_lock()) {
            for (int i = 0; i < liveCount_ && n < capacity; ++i) out[n++] = liveBuf_[(size_t) i];
            liveCount_ = 0;
        }
        return n;
    }

    int liveMidiPort() const override { return -1; }
    void pushLiveMidi(const MidiEvent& e) override {
        std::lock_guard<std::mutex> g(liveMutex_);
        if (liveCount_ < (int) liveBuf_.size()) liveBuf_[(size_t) liveCount_++] = e;
    }

private:
    void refreshClips();
    int durationTicks() const;
    void emit(int offset, bool on, int pitch, int velocity);
    void emitCC(int offset, int controller, int value);
    void flushHeldNotes(int offset);

    Pattern pattern_;
    std::vector<noteschedule::Voice> clips_;
    bool clipsDirty_ = true;
    bool patternEdited_ = false;
    bool wasPlaying_ = false;
    bool wasMuted_ = false;

    std::array<bool, 128> held_{};
    bool bentOut_ = false;
    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> outEvents_;
    int outCount_ = 0;

    std::mutex liveMutex_;
    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> liveBuf_;
    int liveCount_ = 0;
};

}
