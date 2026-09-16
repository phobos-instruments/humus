// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/settings/AiSettingsView.h"

#include "gui/app/AppSettings.h"
#include "gui/assistant/AiClient.h"
#include "gui/common/Localisation.h"

namespace hum {

AiSettingsView::AiSettingsView() {
    auto& s = AppSettings::instance();
    title_.setText(tr("settings-ai.ai", "AI"), juce::dontSendNotification);
    title_.setFont(juce::FontOptions(16.0f).withStyle("Bold"));
    addAndMakeVisible(title_);

    providerLabel_.setText(tr("settings-ai.provider", "Provider"), juce::dontSendNotification);
    addAndMakeVisible(providerLabel_);
    providerCombo_.addItem(tr("settings-ai.ollama-local-server", "Ollama (local server)"), 1);
    providerCombo_.addItem(tr("settings-ai.claude-api", "Claude API"), 2);
    providerCombo_.setSelectedId(s.getString("ai.provider", "ollama") == "claude" ? 2 : 1,
                                 juce::dontSendNotification);
    providerCombo_.onChange = [this] {
        AppSettings::instance().set(
            "ai.provider",
            juce::String(providerCombo_.getSelectedId() == 2 ? "claude" : "ollama"));
        resized();
    };
    addAndMakeVisible(providerCombo_);

    auto setupText = [this](juce::TextEditor& ed, juce::Label& lab, const juce::String& name,
                            const juce::String& value, const juce::String& placeholder,
                            const char* settingsKey) {
        lab.setText(name, juce::dontSendNotification);
        addAndMakeVisible(lab);
        ed.setText(value, juce::dontSendNotification);
        ed.setTextToShowWhenEmpty(placeholder, Palette::textDim);
        auto commit = [&ed, settingsKey] {
            AppSettings::instance().set(settingsKey, ed.getText().trim());
        };
        ed.onFocusLost = commit;
        ed.onReturnKey = commit;
        addAndMakeVisible(ed);
    };
    setupText(endpointEdit_, endpointLabel_, tr("settings-ai.ollama-endpoint", "Ollama endpoint"),
              AppSettings::instance().getString("ai.endpoint", kDefaultAiEndpoint),
              "http://host:11434", "ai.endpoint");
    setupText(modelEdit_, modelLabel_, "Model",
              AppSettings::instance().getString("ai.model"),
              "empty = llama3.1 (Ollama) / claude-sonnet-5 (Claude)", "ai.model");
    setupText(keyEdit_, keyLabel_, tr("settings-ai.claude-api-key", "Claude API key"),
              AppSettings::instance().getString("ai.apiKey"), "sk-ant-...", "ai.apiKey");
    keyEdit_.setPasswordCharacter(0x2022);

    hint_.setFont(juce::FontOptions(12.0f));
    hint_.setColour(juce::Label::textColourId, Palette::textDim);
    hint_.setJustificationType(juce::Justification::topLeft);
    hint_.setText(tr("settings-ai.used-by-the-preset-genie",
       "Used by the Preset genie, the AI Assistant, and the Sample Lab. "
       "Ollama needs a model with tool support pulled on the server "
       "(llama3.1, qwen2.5, mistral-nemo...). Changes apply to the next request."),
                    juce::dontSendNotification);
    addAndMakeVisible(hint_);

    addAndMakeVisible(testBtn_);
    testBtn_.onClick = [this] {
        const auto base = [&] {
            auto e = endpointEdit_.getText().trim();
            return e.endsWithChar('/') ? e.dropLastCharacters(1) : e;
        }();
        hint_.setText(tr("settings-ai.testing", "Testing ") + base + juce::String::fromUTF8("\xe2\x80\xa6"), juce::dontSendNotification);
        const juce::Component::SafePointer<AiSettingsView> safe(this);
        juce::Thread::launch(
            [base, safe] {
            int status = 0;
            auto stream = juce::URL(base + "/api/version").createInputStream(
                juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                    .withConnectionTimeoutMs(5000)
                    .withStatusCode(&status));
            const juce::String result = stream != nullptr
                ? "Connected: " + stream->readEntireStreamAsString().trim()
                : "NOT reachable from Humus (status " + juce::String(status) + "). It "
                  "works in a browser/curl but not here? A network filter (Little "
                  "Snitch, VPN client) is likely blocking this app - allow "
                  "Humus there.";
            juce::MessageManager::callAsync([safe, result] {
                if (safe != nullptr)
                    safe->hint_.setText(result, juce::dontSendNotification);
            });
        });
    };
}


void AiSettingsView::resized() {
    auto panel = getLocalBounds().reduced(16, 12);
    title_.setBounds(panel.removeFromTop(26));
    panel.removeFromTop(10);
    auto row = panel.removeFromTop(26);
    providerLabel_.setBounds(row.removeFromLeft(130));
    providerCombo_.setBounds(row.removeFromLeft(220));
    panel.removeFromTop(10);
    row = panel.removeFromTop(26);
    endpointLabel_.setBounds(row.removeFromLeft(130));
    endpointEdit_.setBounds(row.removeFromLeft(260));
    row.removeFromLeft(8);
    testBtn_.setBounds(row.removeFromLeft(60));
    panel.removeFromTop(10);
    row = panel.removeFromTop(26);
    modelLabel_.setBounds(row.removeFromLeft(130));
    modelEdit_.setBounds(row.removeFromLeft(260));
    panel.removeFromTop(10);
    const bool claude = providerCombo_.getSelectedId() == 2;
    keyLabel_.setVisible(claude);
    keyEdit_.setVisible(claude);
    endpointLabel_.setEnabled(!claude);
    endpointEdit_.setEnabled(!claude);
    if (claude) {
        row = panel.removeFromTop(26);
        keyLabel_.setBounds(row.removeFromLeft(130));
        keyEdit_.setBounds(row.removeFromLeft(260));
        panel.removeFromTop(10);
    }
    panel.removeFromTop(6);
    hint_.setBounds(panel.removeFromTop(64));
}

}
