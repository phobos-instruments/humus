// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"

#include <string>

#include "core/graph/AudioGraph.h"
#include "core/graph/InletNames.h"
#include "gui/common/Localisation.h"
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
            line(tr("control-summary.midi-line", "MIDI: {source} - {behaviour}",
                    {{"source", juce::String(midiSourceLabel(e.source()))}, {"behaviour", behaviourText(e.shape)}})
                     .toStdString());
    for (const auto& e : osc().map().entries())
        if (e.organism == organism && e.param == param)
            line(tr("control-summary.osc-line", "OSC: {address} - {behaviour}",
                    {{"address", juce::String(e.address)}, {"behaviour", behaviourText(e.shape)}})
                     .toStdString());
    for (const auto& e : mod().map().entries())
        if (e.organism == organism && e.param == param)
            line(tr("control-summary.follows-source", "Follows: {source} / {value}",
                    {{"source", juce::String(e.source)}, {"value", juce::String(e.value)}})
                     .toStdString());
    if (const auto var = varDrivingKnob(organism, param); !var.empty())
        line(tr("control-summary.driven-by-name", "Driven by: {name}", {{"name", juce::String(var)}}).toStdString());
    if (automation().isAutomated(organism, param))
        line(tr("control-summary.automated", "Automated").toStdString());
    return out;
}

}
