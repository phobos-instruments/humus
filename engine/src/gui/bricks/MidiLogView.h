// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "core/midi/MidiFormat.h"
#include "gui/editor/BrickBindings.h"
#include "gui/bricks/EventLogView.h"
#include "hum/caps/Midi.h"

namespace hum {

class MidiLogView : public EventLogView {
public:
    MidiLogView(BrickHost& host, std::string name, const Bindings& bound)
        : EventLogView(host, std::move(name)), sourceParam_(bound(bind::kSource)),
          channelParam_(bound(bind::kChannel)) {}

private:
    juce::String headerText() const override {
        return juce::String((int) lineCount()) + " events";
    }
    juce::String emptyText() const override { return "waiting for MIDI..."; }

    void drain() override {
        auto* n = node();
        auto* src = dynamic_cast<MidiLogSource*>(n);
        if (src == nullptr) return;
        if (src->logGeneration() == lastGen_) return;
        lastGen_ = src->logGeneration();

        MidiLogSource::Logged events[256];
        const int count = src->consumeLog(events, 256);
        if (count == 0 || paused()) return;

        const int source = (int) host_.liveParamValue(name_, sourceParam_);
        const int chan = (int) host_.liveParamValue(name_, channelParam_);
        for (int i = 0; i < count; ++i) {
            const auto& e = events[i];
            const bool live = e.origin == 0;
            if (source == 1 && live) continue;
            if (source == 2 && !live) continue;
            const int evChan = midiEventChannel(e.event.data, e.event.size);
            if (chan > 0 && evChan > 0 && evChan != chan) continue;
            const juce::String tag = !live         ? "cord"
                                   : e.port >= 0   ? "in" + juce::String(e.port + 1)
                                                   : "kbd";
            push((tag + "    ").substring(0, 5)
                     + juce::String(formatMidiEvent(e.event.data, e.event.size)),
                 live);
        }
    }

    std::string sourceParam_, channelParam_;
    unsigned lastGen_ = ~0u;
};

}
