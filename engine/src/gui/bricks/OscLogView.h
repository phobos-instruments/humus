// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "gui/host/OscHost.h"
#include "gui/editor/BrickBindings.h"
#include "gui/bricks/EventLogView.h"

namespace hum {

class OscLogView : public EventLogView {
public:
    OscLogView(BrickHost& host, std::string name, const Bindings& bound)
        : EventLogView(host, std::move(name)), reading_(bound(bind::kDirection)) {}

private:
    juce::String headerText() const override {
        const auto count = (long long) lineCount();
        return host_.osc().enabled()
            ? trn("osc-log.messages", "{n} message", "{n} messages", count)
            : trn("osc-log.messages-input-off", "{n} message {dot} input OFF (Settings)",
                  "{n} messages {dot} input OFF (Settings)", count,
                  {{"dot", juce::String::fromUTF8("\xc2\xb7")}});
    }
    juce::String emptyText() const override { return tr("osc-log.waiting-for-osc", "waiting for OSC..."); }

    bool drain(readout::EventLog& log) override { return reading_.drain(host_, name_, paused(), log); }

    readout::OscLog reading_;
};

}
