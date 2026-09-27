// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"

#include <string>

#include "core/graph/AudioGraph.h"
#include "core/graph/InletNames.h"
#include "gui/properties/ControlModeText.h"
#include "hum/caps/Graph.h"

namespace hum {

std::string EngineHost::varDrivingKnob(const std::string& organism, const std::string& param) const {
    if (!graph_) return {};
    const auto* sink = dynamic_cast<const NamedInlet*>(graph_->find(organism));
    if (sink == nullptr) return {};
    for (const auto& e : mod_.map().entries()) {
        if (e.organism != organism || e.param != sink->namedInletParam()) continue;
        const auto* tagged = dynamic_cast<const Tagged*>(graph_->find(e.source));
        if (tagged == nullptr) continue;
        const auto tag = tagged->tag();
        if (tag.empty()) continue;
        const char* knob = sink->knobForName(inletNameFor(tag).c_str());
        if (knob != nullptr && param == knob) return e.source;
    }
    return {};
}

std::string EngineHost::controlSummary(const std::string& organism, const std::string& param) const {
    std::string out;
    auto line = [&out](const std::string& text) { out += (out.empty() ? "" : "\n") + text; };
    for (const auto& e : midiState_.map.entries())
        if (e.organism == organism && e.param == param)
            line("MIDI: " + midiSourceLabel(e.source()) + " - " + behaviourText(e.shape).toStdString());
    for (const auto& e : osc().map().entries())
        if (e.organism == organism && e.param == param)
            line("OSC: " + e.address + " - " + behaviourText(e.shape).toStdString());
    for (const auto& e : mod().map().entries())
        if (e.organism == organism && e.param == param)
            line(translated("control-summary.follows", "Follows") + ": " + e.source + " / " + e.value);
    if (const auto var = varDrivingKnob(organism, param); !var.empty())
        line(translated("control-summary.driven-by", "Driven by") + ": " + var);
    if (automation().isAutomated(organism, param))
        line(translated("control-summary.automated", "Automated"));
    return out;
}

}
