// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/params/ParamSchema.h"
#include "gui/assistant/AiClient.h"
#include "gui/host/PropertiesHost.h"
#include "gui/app/GenieCard.h"
#include "gui/host/NodeRandomize.h"
#include "gui/common/Localisation.h"

namespace hum {

namespace presets {

inline const std::vector<ParamDesc>& schemaOf(PropertiesHost& host, const std::string& node) {
    static const std::vector<ParamDesc> none;
    const auto* cm = host.model().byName(node);
    return cm ? schemaFor(cm->classRaw) : none;
}

inline void recallBracketed(PropertiesHost& host, const std::string& node,
                            const std::function<void()>& op) {
    auto& hist = host.paramHistory();
    hist.commit(node, host.captureNodeState(node));
    op();
    hist.commit(node, host.captureNodeState(node));
}

inline void evolve(PropertiesHost& host, const std::string& node) {
    const auto& schema = schemaOf(host, node);
    const bool curated = hasRandomParams(schema);
    auto& r = juce::Random::getSystemRandom();
    std::vector<std::pair<std::string, double>> vals;
    for (const auto& d : schema) {
        if (d.isText || d.isRange) continue;
        if (curated && !d.randomize) continue;
        double v = host.liveParamValue(node, d.name)
                   + (r.nextDouble() * 2.0 - 1.0) * 0.08 * (d.max - d.min);
        v = juce::jlimit(d.min, d.max, v);
        if (d.isBool || d.isEnum || d.isInt) v = std::round(v);
        vals.emplace_back(d.name, v);
    }
    if (vals.empty()) return;
    host.pushUndo();
    for (const auto& [param, value] : vals) host.setParam(node, param, value);
}

inline void showGenie(PropertiesHost& host, const std::string& node,
                      std::function<void()> onChanged) {
    const auto* cm = host.model().byName(node);
    if (cm == nullptr) return;
    auto* const hostPtr = &host;
    const juce::String display(cm->displayClass);

    auto* w = new juce::AlertWindow(
        juce::String("Preset Genie - ") + juce::String(node),
        "Describe the sound; the model sets " + display + "'s parameters (one undo step).",
        juce::MessageBoxIconType::NoIcon);
    w->addTextEditor("prompt", "", tr("preset-actions.describe-it", "Describe it:"));
    if (AiClient::needsApiKey())
        w->addTextEditor("key", "", tr("preset-actions.claude-api-key-stored-for", "Claude API key (stored for next time):"));
    w->addButton(tr("preset-actions.generate", "Generate"), 1, juce::KeyPress(juce::KeyPress::returnKey));
    w->addButton(tr("preset-actions.cancel", "Cancel"), 0, juce::KeyPress(juce::KeyPress::escapeKey));
    w->enterModalState(true, juce::ModalCallbackFunction::create(
        [w, hostPtr, node, onChanged](int result) {
            const juce::String prompt = w->getTextEditorContents("prompt").trim();
            const juce::String key = w->getTextEditorContents("key").trim();
            w->exitModalState(result);
            w->setVisible(false);
            delete w;
            if (result != 1 || prompt.isEmpty()) return;
            if (key.isNotEmpty()) AiClient::setApiKey(key);
            auto card = std::make_unique<GenieCard>(*hostPtr, node, prompt, onChanged);
            card->start();
            hostPtr->presentCard(std::move(card));
        }), false);
}

}
}
