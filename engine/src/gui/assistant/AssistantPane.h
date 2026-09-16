// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/assistant/AssistantEngine.h"
#include "gui/assistant/AiClient.h"
#include "gui/host/AssistantHost.h"
#include "gui/app/FreeWindow.h"
#include "gui/style/LookAndFeel.h"
#include "gui/common/Localisation.h"

namespace hum {

class AssistantPane : public juce::Component {
public:
    AssistantPane(AssistantHost& host, std::function<void()> onPatchEdited)
        : engine_(host) {
        transcript_.setMultiLine(true);
        transcript_.setReadOnly(true);
        transcript_.setScrollbarsShown(true);
        transcript_.setCaretVisible(false);
        transcript_.setColour(juce::TextEditor::backgroundColourId, Palette::background);
        transcript_.setColour(juce::TextEditor::outlineColourId, Palette::border);
        transcript_.setFont(juce::FontOptions(13.0f));
        addAndMakeVisible(transcript_);

        input_.setTextToShowWhenEmpty("Ask for a patch, a chain, a tweak...  (Enter sends)",
                                      Palette::textDim);
        input_.onReturnKey = [this] { sendNow(); };
        addAndMakeVisible(input_);
        send_.onClick = [this] { sendNow(); };
        addAndMakeVisible(send_);

        engine_.onLine = [this](const juce::String& role, const juce::String& text) {
            appendLine(role, text);
        };
        engine_.onBusy = [this](bool b) {
            send_.setButtonText(b ? "..." : tr("assistant-pane.send", "Send"));
            send_.setEnabled(!b);
            input_.setEnabled(!b);
        };
        engine_.onPatchEdited = std::move(onPatchEdited);

        appendLine("assistant",
                   juce::String::fromUTF8(
                       "I can build and edit this patch - try \"make a tremolo "
                       "delay on the master\" or \"why is this silent?\"."));
        setSize(460, 520);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(8);
        auto row = r.removeFromBottom(28);
        send_.setBounds(row.removeFromRight(64));
        row.removeFromRight(6);
        input_.setBounds(row);
        r.removeFromBottom(8);
        transcript_.setBounds(r);
    }

    void paint(juce::Graphics& g) override { g.fillAll(Palette::panel); }

    bool busy() const { return engine_.busy(); }

private:
    void sendNow() {
        const auto text = input_.getText().trim();
        if (text.isEmpty() || engine_.busy()) return;
        if (AiClient::needsApiKey()) { askForKey(text); return; }
        input_.clear();
        engine_.send(text);
    }

    void askForKey(const juce::String& pendingText) {
        auto* w = new juce::AlertWindow(tr("assistant-pane.ai-assistant", "AI Assistant"),
                                        tr("assistant-pane.paste-your-key",
                                           "Paste your Claude API key (stored for next time)."),
                                        juce::MessageBoxIconType::NoIcon);
        w->addTextEditor("key", "", tr("assistant-pane.api-key", "API key:"));
        w->addButton(tr("assistant-pane.save", "Save"), 1, juce::KeyPress(juce::KeyPress::returnKey));
        w->addButton(tr("assistant-pane.cancel", "Cancel"), 0, juce::KeyPress(juce::KeyPress::escapeKey));
        w->enterModalState(true, juce::ModalCallbackFunction::create(
            [w, sp = juce::Component::SafePointer<AssistantPane>(this), pendingText](int r) {
                const auto key = w->getTextEditorContents("key").trim();
                w->exitModalState(r);
                w->setVisible(false);
                delete w;
                if (r != 1 || key.isEmpty() || sp == nullptr) return;
                AiClient::setApiKey(key);
                sp->input_.clear();
                sp->engine_.send(pendingText);
            }), false);
    }

    void appendLine(const juce::String& role, const juce::String& text) {
        juce::String prefix = role == "you"       ? "You:  "
                            : role == "assistant" ? "Humus:  "
                            : role == "tool"      ? juce::String::fromUTF8("  \xc2\xb7 ")
                                                  : "  ! ";
        transcript_.moveCaretToEnd();
        transcript_.insertTextAtCaret(prefix + text.trimEnd() + "\n"
                                      + (role == "assistant" || role == "you" ? "\n" : ""));
        transcript_.moveCaretToEnd();
    }

    AssistantEngine engine_;
    juce::TextEditor transcript_, input_;
    juce::TextButton send_{"Send"};
};

class AssistantWindow : public juce::DocumentWindow {
public:
    AssistantWindow(AssistantHost& host, std::function<void()> onPatchEdited)
        : juce::DocumentWindow(tr("assistant-pane.ai-assistant", "AI Assistant"), Palette::panel,
                               juce::DocumentWindow::closeButton) {
        setUsingNativeTitleBar(true);
        pane_ = new AssistantPane(host, std::move(onPatchEdited));
        setContentOwned(pane_, true);
        setResizable(true, true);
        setResizeLimits(340, 300, 900, 1400);
        centreWithSize(460, 520);
    }
    void closeButtonPressed() override { setVisible(false); }

    bool keyPressed(const juce::KeyPress& k) override {
        if (isCloseWindowKey(k)) { closeButtonPressed(); return true; }
        return juce::DocumentWindow::keyPressed(k);
    }

private:
    AssistantPane* pane_ = nullptr;
};

}
