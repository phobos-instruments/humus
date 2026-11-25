// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>

#include "core/graph/GraphMidi.h"
#include "core/graph/MidiTap.h"
#include "gui/host/DawMacroTarget.h"
#include "hum/Parameter.h"
#include "hum/Transport.h"

namespace hum {

class AudioGraph;
struct PatchDocumentModel;

class DawBridge {
public:
    static constexpr int kControls = 8;

    struct Clock {
        bool known = false;
        bool playing = false;
        double bpm = 0.0;
        double ppq = -1.0;
    };

    void setClock(const Clock& clock) { clock_ = clock; }
    void setMidi(juce::MidiBuffer* in, juce::MidiBuffer* out) {
        in_ = in;
        out_ = out;
    }
    void setControlValue(int slot, float normalized) {
        if (slot >= 0 && slot < kControls) values_[(size_t) slot] = normalized;
    }
    void send(const juce::MidiMessage& m);
    std::function<void(int slot, float normalized)> onControlMapped;

    void rebuild(AudioGraph* graph, const PatchDocumentModel& model);
    bool add(int slot, const DawMacroTarget& target, AudioGraph* graph, const PatchDocumentModel& model);
    void setTargets(int slot, const std::vector<DawMacroTarget>& targets, AudioGraph* graph,
                    const PatchDocumentModel& model);
    void remove(const std::string& organism, const std::string& param);
    void setRange(const std::string& organism, const std::string& param, double from, double to);
    void noteMappedAt(int slot, double value);
    std::vector<DawMacroTarget> targets(int slot) const;
    int slotFor(const std::string& organism, const std::string& param) const;
    bool drives(const std::string& organism, const std::string& param) const { return slotFor(organism, param) >= 0; }

    void applyClock(Transport& transport, bool playIntent, int numSamples);
    void applyControls();
    void deliverMidi();
    void collectMidi();

    void drainSeen(const std::function<void(const juce::MidiMessage&)>& noteControl);

private:
    struct Target {
        DawMacroTarget spec;
        Parameter* live = nullptr;
        double lo = 0.0, hi = 1.0;
        bool stepped = false;
    };

    void resolveTargets(AudioGraph* graph, const PatchDocumentModel& model);
    void collectPorts(AudioGraph* graph, const PatchDocumentModel& model);

    Clock clock_;
    bool wasPlaying_ = false;
    double lastPpq_ = -1.0;
    int lastBlock_ = 0;
    graphmidi::Ports ports_;
    juce::MidiBuffer* in_ = nullptr;
    juce::MidiBuffer* out_ = nullptr;
    const std::unique_ptr<MidiTapRing> seen_ = std::make_unique<MidiTapRing>();
    const std::unique_ptr<MidiTapRing> toSend_ = std::make_unique<MidiTapRing>();
    std::array<std::vector<Target>, kControls> targets_;
    std::array<float, kControls> values_{};
    std::array<float, kControls> applied_{};
};

}
