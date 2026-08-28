#pragma once
#include "gui/EventLogView.h"
#include "hum/Capabilities.h"

namespace hum {

class OscLogView : public EventLogView {
public:
    using EventLogView::EventLogView;

private:
    juce::String headerText() const override {
        const auto count = juce::String((int) lineCount());
        return host_.osc().enabled()
            ? count + " messages"
            : count + juce::String::fromUTF8(" messages \xc2\xb7 input OFF (Settings)");
    }
    juce::String emptyText() const override { return "waiting for OSC..."; }

    void drain() override {
        auto* n = node();
        auto* src = dynamic_cast<OscLogSource*>(n);
        if (src == nullptr) return;
        if (src->oscLogGeneration() == lastGen_) return;
        lastGen_ = src->oscLogGeneration();

        OscLogSource::Logged events[256];
        const int count = src->consumeOscLog(events, 256);
        if (count == 0 || paused()) return;

        const int dir = n ? (int) n->params.get("Direction", 0.0) : 0;
        for (int i = 0; i < count; ++i) {
            const auto& e = events[i];
            if (dir == 1 && e.out) continue;
            if (dir == 2 && !e.out) continue;
            push(juce::String(e.out ? "out  " : "in   ")
                     + juce::String::fromUTF8(e.address) + "  "
                     + juce::String::fromUTF8(e.args),
                 e.out);
        }
    }

    unsigned lastGen_ = ~0u;
};

}
