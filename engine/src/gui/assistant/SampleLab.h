// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/app/AppPaths.h"
#include "core/assistant/RecipeSynth.h"
#include "core/packs/Roles.h"
#include "gui/assistant/AiClient.h"
#include "gui/host/AssistantHost.h"
#include "io/WavWriter.h"
#include "gui/common/Localisation.h"

#include "hum/dsp/DspMath.h"

namespace hum {

namespace samplelab {

inline juce::File sampleFolder() {
    auto dir = userLibraryRoot().getChildFile("Library").getChildFile("Samples");
    dir.createDirectory();
    return dir;
}

inline juce::String sanitize(const std::string& name) {
    return juce::File::createLegalFileName(juce::String(name)).replaceCharacter(' ', '-');
}

inline juce::String applyRecipes(AssistantHost& host, const std::string& targetSampler,
                                 const std::vector<Recipe>& recipes) {
    if (recipes.empty()) return tr("sample-lab.the-reply-contained-no-usable", "The reply contained no usable sounds.");
    const auto stamp = juce::Time::getCurrentTime().formatted("%H%M%S");
    std::vector<juce::String> paths;
    for (const auto& r : recipes) {
        const auto mono = renderRecipe(r, kDefaultSampleRate);
        const auto f = sampleFolder().getChildFile(sanitize(r.name) + "-" + stamp + ".wav");
        if (writeWav(f.getFullPathName().toStdString(), {mono}, kDefaultSampleRate))
            paths.push_back(f.getFullPathName());
    }
    if (paths.empty()) return tr("sample-lab.could-not-write-the-sample", "Could not write the sample files.");

    host.pushUndo();
    std::string sampler = targetSampler;
    if (sampler.empty() || !host.model().byName(sampler))
        sampler = host.addOrganism(classWithRole(role::kSampleKit), {160, 160});
    host.setParam(sampler, "Mode", 1.0);
    for (size_t i = 0; i < paths.size() && i < 8; ++i) {
        const auto zone = std::to_string(i + 1);
        host.setParamText(sampler, "File" + zone, paths[i].toStdString());
        host.setParam(sampler, "Root" + zone, 60.0 + (double) i);
    }
    return juce::String((int) paths.size()) + " sounds on '" + juce::String(sampler)
           + juce::String("' - keys C4.. play them (files in ")
           + sampleFolder().getFullPathName() + ")";
}

inline void open(AssistantHost& host, std::function<void(juce::String status)> onStatus) {
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
        [w, hostPtr, samplers, onStatus](int result) {
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
            if (onStatus) onStatus(tr("sample-lab.sample-lab-designing-sounds", "Sample Lab: designing sounds..."));

            AiClient::Request req;
            req.system = recipeSystemPrompt();
            req.user = prompt;
            req.maxTokens = 2000;
            AiClient::complete(std::move(req),
                [hostPtr, targetName, onStatus](juce::String text, juce::String error) {
                    if (error.isNotEmpty()) {
                        if (onStatus) onStatus("Sample Lab: " + error);
                        return;
                    }
                    const auto status =
                        applyRecipes(*hostPtr, targetName, parseRecipes(text));
                    if (onStatus) onStatus("Sample Lab: " + status);
                });
        }), false);
}

}
}
