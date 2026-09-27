// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <mutex>
#include <string>
#include <vector>

#include "Morse/MorseCode.h"
#include "hum/caps/Midi.h"
#include "hum/Organism.h"
#include "hum/dsp/Prepared.h"

namespace hum {

class Morse : public Organism, public MidiNode, public KeyedTape {
public:
    int tapeRuns(int* units, bool* on, int capacity) const override {
        const std::lock_guard<std::mutex> hold(tapeLock_);
        const int n = std::min(capacity, (int) tape_.size());
        for (int i = 0; i < n; ++i) {
            units[i] = tape_[(size_t) i].units;
            on[i] = tape_[(size_t) i].on;
        }
        return n;
    }
    int tapeRunNow() const override { return runNow_.load(std::memory_order_relaxed); }

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
        syncText();
        reset();
    }
    void loadFrom(const OrganismState& state) override {
        Organism::loadFrom(state);
        syncText();
    }
    void onTextChanged(const std::string& param, const std::string& text) override {
        if (param != "Text") return;
        appliedText_ = text;
        auto runs = morse::encode(text);
        setTape(runs);
        pendingPattern_.publish(std::move(runs));
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
    static constexpr double kUnitsPerBeat = 4.0;
    static constexpr long kLoopGapUnits = 7;

    void emit(int offset, bool on, int note);
    void placeAt(double units, double unitSamples, bool loop);

    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> outEvents_;
    int outCount_ = 0;
    bool keyWasOn_ = false;
    int midiNote_ = 74;

    void syncText() {
        const std::string text = params.getText("Text");
        if (text == appliedText_) return;
        appliedText_ = text;
        pattern_ = morse::encode(text);
        setTape(pattern_);
    }

    void setTape(const std::vector<morse::Seg>& runs) {
        const std::lock_guard<std::mutex> hold(tapeLock_);
        tape_ = runs;
    }

    mutable std::mutex tapeLock_;
    std::vector<morse::Seg> tape_;
    std::atomic<int> runNow_{-1};
    std::vector<morse::Seg> pattern_;
    Prepared<std::vector<morse::Seg>> pendingPattern_;
    std::string appliedText_;
    size_t seg_ = 0;
    long segPos_ = 0;
    double phase_ = 0.0;
    float amp_ = 0.0f;
    bool done_ = false;
};

}
