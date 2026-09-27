// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "gui/editor/BrickBindings.h"
#include "gui/bricks/EventLogView.h"

namespace hum {

class MidiLogView : public EventLogView {
public:
    MidiLogView(BrickHost& host, std::string name, const Bindings& bound)
        : EventLogView(host, std::move(name)), reading_(bound(bind::kSource), bound(bind::kChannel)) {}

private:
    juce::String headerText() const override {
        return juce::String((int) lineCount()) + " events";
    }
    juce::String emptyText() const override { return "waiting for MIDI..."; }

    bool drain(readout::EventLog& log) override { return reading_.drain(host_, name_, paused(), log); }

    readout::MidiLog reading_;
};

}
