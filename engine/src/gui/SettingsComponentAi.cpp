#include "gui/SettingsComponent.h"

#include "gui/AiClient.h"
#include "gui/AppSettings.h"

namespace hum {

void SettingsComponent::buildAiPanel() {
    auto& s = AppSettings::instance();
    aiTitle_.setText("AI", juce::dontSendNotification);
    aiTitle_.setFont(juce::FontOptions(16.0f).withStyle("Bold"));
    addAndMakeVisible(aiTitle_);

    providerLabel_.setText("Provider", juce::dontSendNotification);
    addAndMakeVisible(providerLabel_);
    providerCombo_.addItem("Ollama (local server)", 1);
    providerCombo_.addItem("Claude API", 2);
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
    setupText(endpointEdit_, endpointLabel_, "Ollama endpoint",
              AppSettings::instance().getString("ai.endpoint", kDefaultAiEndpoint),
              "http://host:11434", "ai.endpoint");
    setupText(modelEdit_, modelLabel_, "Model",
              AppSettings::instance().getString("ai.model"),
              "empty = llama3.1 (Ollama) / claude-sonnet-5 (Claude)", "ai.model");
    setupText(keyEdit_, keyLabel_, "Claude API key",
              AppSettings::instance().getString("ai.apiKey"), "sk-ant-...", "ai.apiKey");
    keyEdit_.setPasswordCharacter(0x2022);

    aiHint_.setFont(juce::FontOptions(12.0f));
    aiHint_.setColour(juce::Label::textColourId, Palette::textDim);
    aiHint_.setJustificationType(juce::Justification::topLeft);
    aiHint_.setText("Used by the Preset genie, the AI Assistant, and the Sample Lab. "
                    "Ollama needs a model with tool support pulled on the server "
                    "(llama3.1, qwen2.5, mistral-nemo...). Changes apply to the next request.",
                    juce::dontSendNotification);
    addAndMakeVisible(aiHint_);

    addAndMakeVisible(testBtn_);
    testBtn_.onClick = [this] {
        const auto base = [&] {
            auto e = endpointEdit_.getText().trim();
            return e.endsWithChar('/') ? e.dropLastCharacters(1) : e;
        }();
        aiHint_.setText("Testing " + base + juce::String::fromUTF8("\xe2\x80\xa6"), juce::dontSendNotification);
        const juce::Component::SafePointer<SettingsComponent> safe(this);
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
                    safe->aiHint_.setText(result, juce::dontSendNotification);
            });
        });
    };
}

}
