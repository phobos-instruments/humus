// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <memory>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/packs/Roles.h"
#include "gui/assistant/AiClient.h"
#include "gui/assistant/SampleLabCard.h"
#include "gui/common/Localisation.h"
#include "gui/host/AssistantHost.h"

namespace hum {

namespace samplelab {

using Present = std::function<void(std::unique_ptr<juce::Component> card)>;

inline void open(AssistantHost& host, Present present, std::function<void()> onChanged) {
    juce::StringArray samplers;
    for (const auto& cm : host.model().organisms)
        if (classHasRole(cm.classRaw, role::kSampleKit)) samplers.add(juce::String(cm.name));

    auto* w = new juce::AlertWindow(
        "AI Sample Lab",
        "Describe a family of sounds (\"dusty vinyl drum kit\", \"glass bell plucks\"). "
        "The model designs them, Humus renders them, and they land on a Sampler as a kit "
        "(one undo step).",
        juce::MessageBoxIconType::NoIcon);
    w->addTextEditor("prompt", "", tr("sample-lab.describe-the-sounds", "Describe the sounds:"));
    juce::StringArray choices = samplers;
    choices.add(tr("sample-lab.new-sampler", "New Sampler"));
    w->addComboBox("target", choices, "Onto:");
    w->getComboBoxComponent("target")->setSelectedItemIndex(choices.size() - 1);
    if (AiClient::needsApiKey())
        w->addTextEditor("key", "", tr("sample-lab.claude-api-key-stored-for", "Claude API key (stored for next time):"));
    w->addButton(tr("sample-lab.generate", "Generate"), 1, juce::KeyPress(juce::KeyPress::returnKey));
    w->addButton(tr("sample-lab.cancel", "Cancel"), 0, juce::KeyPress(juce::KeyPress::escapeKey));

    auto* hostPtr = &host;
    w->enterModalState(true, juce::ModalCallbackFunction::create(
        [w, hostPtr, samplers, present, onChanged](int result) {
            const juce::String prompt = w->getTextEditorContents("prompt").trim();
            const juce::String key = w->getTextEditorContents("key").trim();
            const int target = w->getComboBoxComponent("target")->getSelectedItemIndex();
            w->exitModalState(result);
            w->setVisible(false);
            delete w;
            if (result != 1 || prompt.isEmpty()) return;
            if (key.isNotEmpty()) AiClient::setApiKey(key);
            const std::string targetName =
                target >= 0 && target < samplers.size()
                    ? samplers[target].toStdString() : std::string();
            auto card = std::make_unique<SampleLabCard>(*hostPtr, targetName, prompt, onChanged);
            card->start();
            if (present) present(std::move(card));
        }), false);
}

}
}
