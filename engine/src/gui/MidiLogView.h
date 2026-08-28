#pragma once
#include "core/MidiFormat.h"
#include "gui/EventLogView.h"
#include "hum/Capabilities.h"

namespace hum {

class MidiLogView : public EventLogView {
public:
    using EventLogView::EventLogView;

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

        const int source = n ? (int) n->params.get("Source", 0.0) : 0;
        const int chan = n ? (int) n->params.get("Channel", 0.0) : 0;
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

    unsigned lastGen_ = ~0u;
};

}
