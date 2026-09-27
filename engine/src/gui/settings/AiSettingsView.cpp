// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/settings/AiSettingsView.h"

#include "gui/app/AppSettings.h"
#include "gui/assistant/AiClient.h"
#include "gui/common/Localisation.h"

namespace hum {

namespace {

juce::String connectedText(const juce::String& versionJson, const juce::StringArray& models) {
    const auto version = juce::JSON::parse(versionJson)["version"].toString();
    auto text = tr("settings-ai.connected", "Connected to Ollama") + " " + version + ". ";
    if (models.isEmpty())
        return text + tr("settings-ai.no-models", "No models pulled yet - pull one on the server (llama3.1, qwen2.5, mistral-nemo...).");
    return text + juce::String(models.size()) + " " + tr("settings-ai.models-pulled", "models pulled - pick one below.");
}

juce::String unreachableText(int status) {
    return tr("settings-ai.not-reachable", "NOT reachable from Humus (status ") + juce::String(status) + "). "
           + tr("settings-ai.filter-hint",
                "It works in a browser/curl but not here? A network filter (Little Snitch, VPN client) "
                "is likely blocking this app - allow Humus there.");
}

}

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
    providerCombo_.onChange = [this] { onProviderChanged(); };
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
              s.getString("ai.endpoint", kDefaultAiEndpoint), "http://host:11434", "ai.endpoint");
    setupText(keyEdit_, keyLabel_, tr("settings-ai.claude-api-key", "Claude API key"),
              s.getString("ai.apiKey"), "sk-ant-...", "ai.apiKey");
    keyEdit_.setPasswordCharacter(0x2022);

    modelLabel_.setText(tr("settings-ai.model", "Model"), juce::dontSendNotification);
    addAndMakeVisible(modelLabel_);
    modelCombo_.setEditableText(true);
    modelCombo_.setTextWhenNothingSelected(
        tr("settings-ai.model-default", "empty = llama3.1 (Ollama) / claude-sonnet-5 (Claude)"));
    modelCombo_.setText(s.getString("ai.model"), juce::dontSendNotification);
    modelCombo_.onChange = [this] {
        AppSettings::instance().set("ai.model", modelCombo_.getText().trim());
    };
    addAndMakeVisible(modelCombo_);

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
    testBtn_.onClick = [this] { probe(true); };
    if (providerCombo_.getSelectedId() == 1) probe(false);
}

juce::String AiSettingsView::endpointBase() const {
    const auto e = endpointEdit_.getText().trim();
    return e.endsWithChar('/') ? e.dropLastCharacters(1) : e;
}

void AiSettingsView::fillModels(const juce::StringArray& names) {
    const auto current = modelCombo_.getText();
    modelCombo_.clear(juce::dontSendNotification);
    int id = 1;
    for (const auto& n : names) modelCombo_.addItem(n, id++);
    modelCombo_.setText(current, juce::dontSendNotification);
}

void AiSettingsView::onProviderChanged() {
    const bool claude = providerCombo_.getSelectedId() == 2;
    AppSettings::instance().set("ai.provider", juce::String(claude ? "claude" : "ollama"));
    if (claude) fillModels({}); else probe(false);
    resized();
}

void AiSettingsView::probe(bool announce) {
    const auto base = endpointBase();
    if (announce)
        hint_.setText(tr("settings-ai.testing", "Testing ") + base + juce::String::fromUTF8("\xe2\x80\xa6"),
                      juce::dontSendNotification);
    const juce::Component::SafePointer<AiSettingsView> safe(this);
    juce::Thread::launch([base, safe, announce] {
        int status = 0;
        auto stream = juce::URL(base + "/api/version").createInputStream(
            juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                .withConnectionTimeoutMs(5000)
                .withStatusCode(&status));
        const bool connected = stream != nullptr;
        const auto version = connected ? stream->readEntireStreamAsString().trim() : juce::String();
        juce::String error;
        const auto models = connected ? AiClient::ollamaModels(base, error) : juce::StringArray();
        juce::MessageManager::callAsync([safe, announce, connected, version, models, status] {
            if (safe == nullptr) return;
            if (connected) safe->fillModels(models);
            if (!announce) return;
            safe->hint_.setText(connected ? connectedText(version, models) : unreachableText(status),
                                juce::dontSendNotification);
        });
    });
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
    modelCombo_.setBounds(row.removeFromLeft(260));
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
