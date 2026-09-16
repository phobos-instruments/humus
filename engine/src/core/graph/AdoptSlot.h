// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cstring>
#include <string>

#include "hum/caps/Audio.h"
#include "hum/caps/Midi.h"
#include "hum/Organism.h"

namespace hum {

class AdoptSlot : public Organism, public MidiNode, public LatencyReporting {
public:
    explicit AdoptSlot(const Organism& live)
        : ins_(live.numAudioInputs()),
          outs_(live.numAudioOutputs()),
          token_(live.matchToken()) {
        if (auto* m = dynamic_cast<const MidiNode*>(&live)) {
            midiIns_ = m->numMidiInputs();
            midiOuts_ = m->numMidiOutputs();
        }
        if (auto* l = dynamic_cast<const LatencyReporting*>(&live))
            latency_ = std::max(0, l->latencySamples());
    }

    int numAudioInputs() const override { return ins_; }
    int numAudioOutputs() const override { return outs_; }
    void prepare(double sampleRate, int) override { sampleRate_ = sampleRate; }
    void process(const float* const*, int, float* const* out, int numOut,
                 int numSamples, const Transport&) override {
        for (int c = 0; c < numOut; ++c)
            std::memset(out[c], 0, sizeof(float) * (size_t) numSamples);
    }
    std::string matchToken() const override { return token_; }

    int latencySamples() const override { return latency_; }
    int numMidiInputs() const override { return midiIns_; }
    int numMidiOutputs() const override { return midiOuts_; }
    void deliverMidi(int, const MidiEvent*, int) override {}
    int collectMidi(int, MidiEvent*, int) override { return 0; }

private:
    int ins_ = 0, outs_ = 0, midiIns_ = 0, midiOuts_ = 0, latency_ = 0;
    std::string token_;
};

}
