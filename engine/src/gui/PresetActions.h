#pragma once
#include <cmath>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/ParamSchema.h"
#include "core/PresetGenie.h"
#include "gui/AiClient.h"
#include "gui/EngineHost.h"
#include "gui/NodeRandomize.h"
#include "gui/Localisation.h"

namespace hum {

namespace presets {

inline const std::vector<ParamDesc>& schemaOf(EngineHost& host, const std::string& node) {
    static const std::vector<ParamDesc> none;
    const auto* cm = host.model().byName(node);
    return cm ? schemaFor(cm->classRaw) : none;
}

inline void recallBracketed(EngineHost& host, const std::string& node,
                            const std::function<void()>& op) {
    auto& hist = host.paramHistory();
    hist.commit(node, host.captureNodeState(node));
    op();
    hist.commit(node, host.captureNodeState(node));
}

inline void evolve(EngineHost& host, const std::string& node) {
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

inline void showGenie(EngineHost& host, const std::string& node,
                      std::function<void()> onChanged,
                      std::function<void(bool)> onBusy = {}) {
    const auto* cm = host.model().byName(node);
    if (cm == nullptr) return;
    auto* const hostPtr = &host;
    const auto schemaCopy = schemaOf(host, node);
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
        [w, hostPtr, node, schemaCopy, onChanged, onBusy](int result) {
            const juce::String prompt = w->getTextEditorContents("prompt").trim();
            const juce::String key = w->getTextEditorContents("key").trim();
            w->exitModalState(result);
            w->setVisible(false);
            delete w;
            if (result != 1 || prompt.isEmpty()) return;
            if (key.isNotEmpty()) AiClient::setApiKey(key);
            if (onBusy) onBusy(true);

            AiClient::Request req;
            req.system = genieSystemPrompt(
                hostPtr->model().byName(node)
                    ? hostPtr->model().byName(node)->displayClass : node,
                schemaCopy);
            req.user = prompt;
            AiClient::complete(std::move(req),
                [hostPtr, node, schemaCopy, onChanged, onBusy](juce::String text, juce::String error) {
                    if (onBusy) onBusy(false);
                    if (error.isNotEmpty()) {
                        juce::AlertWindow::showMessageBoxAsync(
                            juce::MessageBoxIconType::WarningIcon, tr("preset-actions.preset-genie", "Preset Genie"), error);
                        return;
                    }
                    const auto vals = parseGenieReply(text, schemaCopy);
                    if (vals.empty()) {
                        juce::AlertWindow::showMessageBoxAsync(
                            juce::MessageBoxIconType::WarningIcon, "Preset Genie",
                            "The reply contained no usable parameters.");
                        return;
                    }
                    hostPtr->pushUndo();
                    for (const auto& [param, value] : vals) hostPtr->setParam(node, param, value);
                    if (onChanged) onChanged();
                });
        }), false);
}

}
}
