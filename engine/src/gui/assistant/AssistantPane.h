// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <memory>
#include <utility>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/assistant/AssistantCard.h"
#include "gui/assistant/AssistantEngine.h"
#include "gui/assistant/AssistantTranscript.h"
#include "gui/assistant/AiClient.h"
#include "gui/host/AssistantHost.h"
#include "gui/app/FreeWindow.h"
#include "gui/app/TimedCard.h"
#include "gui/style/LookAndFeel.h"
#include "gui/common/Localisation.h"

namespace hum {

class WorkStrip : public juce::Component, private juce::Timer {
public:
    std::function<void()> onStop;

    WorkStrip() {
        stop_.setButtonText(tr("assistant-pane.stop", "Stop"));
        stop_.onClick = [this] { if (onStop) onStop(); };
        addAndMakeVisible(stop_);
        setVisible(false);
    }

    void begin() {
        ticks_ = 0;
        setVisible(true);
        startTimerHz(TimedCard::kTickHz);
        repaint();
    }

    void end() {
        stopTimer();
        setVisible(false);
    }

    int seconds() const { return ticks_ / TimedCard::kTickHz; }

    void paint(juce::Graphics& g) override {
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(11.0f));
        const auto label = tr("assistant-pane.working", "Working...") + " "
                         + juce::String(seconds()) + " s";
        g.drawText(label, 2, 0, getWidth() - kStopW - 8, 14, juce::Justification::centredLeft);
        TimedCard::paintWorkingBar(g, {2.0f, 16.0f, (float) (getWidth() - kStopW - 10), 4.0f},
                                   ticks_);
    }

    void resized() override {
        stop_.setBounds(getLocalBounds().removeFromRight(kStopW).reduced(0, 1));
    }

private:
    static constexpr int kStopW = 54;

    void timerCallback() override {
        ++ticks_;
        repaint();
    }

    juce::TextButton stop_;
    int ticks_ = 0;
};

class AssistantPane : public juce::Component {
public:
    AssistantPane(AssistantHost& host, std::function<void()> onPatchEdited)
        : host_(host), engine_(host) {
        viewport_.setViewedComponent(&transcript_, false);
        viewport_.setScrollBarsShown(true, false);
        addAndMakeVisible(viewport_);

        work_.onStop = [this] { engine_.stop(); };
        addChildComponent(work_);

        input_.setTextToShowWhenEmpty(
            tr("assistant-pane.ask", "Ask for a patch, a chain, a tweak...  (Enter sends)"),
            Palette::textDim);
        input_.onReturnKey = [this] { sendNow(); };
        addAndMakeVisible(input_);
        send_.setButtonText(tr("assistant-pane.send", "Send"));
        send_.onClick = [this] { sendNow(); };
        addAndMakeVisible(send_);

        engine_.onLine = [this](const juce::String& role, const juce::String& text) {
            if (role == "assistant") lastReply_ = text;
            transcript_.say(role, text);
            transcript_.layoutTo(viewport_.getMaximumVisibleWidth());
            viewport_.setViewPositionProportionately(0.0, 1.0);
        };
        engine_.onBusy = [this](bool b) {
            send_.setEnabled(!b);
            input_.setEnabled(!b);
            if (b) work_.begin(); else work_.end();
            resized();
        };
        engine_.onPatchEdited = std::move(onPatchEdited);

        transcript_.say("assistant",
                        tr("assistant-pane.greeting",
                           "I can build and edit this patch. Try **make a tremolo delay on the "
                           "master**, or ask **why is this silent?**"));
        setSize(480, 560);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(8);
        auto row = r.removeFromBottom(28);
        send_.setBounds(row.removeFromRight(64));
        row.removeFromRight(6);
        input_.setBounds(row);
        r.removeFromBottom(8);
        if (work_.isVisible()) {
            work_.setBounds(r.removeFromBottom(22));
            r.removeFromBottom(6);
        }
        viewport_.setBounds(r);
        transcript_.layoutTo(viewport_.getMaximumVisibleWidth());
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(Palette::panel);
        g.setColour(Palette::border);
        g.drawRect(viewport_.getBounds().expanded(1), 1);
    }

    bool busy() const { return engine_.busy(); }
    void stopNow() { engine_.stop(); }
    const juce::String& lastAsk() const { return lastAsk_; }
    const juce::String& lastReply() const { return lastReply_; }
    AssistantHost& host() { return host_; }
    void sendForTest() { sendNow(); }
    AssistantTranscript& transcriptForTest() { return transcript_; }
    juce::TextEditor& inputForTest() { return input_; }
    WorkStrip& workForTest() { return work_; }

private:
    void sendNow() {
        const auto text = input_.getText().trim();
        if (text.isEmpty() || engine_.busy()) return;
        if (AiClient::needsApiKey()) { askForKey(text); return; }
        input_.clear();
        lastAsk_ = text;
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
                sp->lastAsk_ = pendingText;
                sp->engine_.send(pendingText);
            }), false);
    }

    AssistantHost& host_;
    AssistantEngine engine_;
    AssistantTranscript transcript_;
    juce::Viewport viewport_;
    WorkStrip work_;
    juce::TextEditor input_;
    juce::TextButton send_;
    juce::String lastAsk_, lastReply_;
};

inline std::unique_ptr<AssistantCard> assistantCardFor(AssistantPane& pane,
                                                       std::function<void()> show) {
    juce::Component::SafePointer<AssistantPane> safe(&pane);
    AssistantCard::Hooks hooks;
    hooks.working = [safe] { return safe != nullptr && safe->busy(); };
    hooks.reply = [safe] { return safe != nullptr ? safe->lastReply() : juce::String(); };
    hooks.stop = [safe] { if (safe != nullptr) safe->stopNow(); };
    hooks.show = std::move(show);
    return std::make_unique<AssistantCard>(pane.lastAsk(), std::move(hooks));
}

class AssistantWindow : public juce::DocumentWindow {
public:
    using Present = std::function<void(std::unique_ptr<juce::Component>)>;

    AssistantWindow(AssistantHost& host, std::function<void()> onPatchEdited, Present present = {})
        : juce::DocumentWindow(tr("assistant-pane.ai-assistant", "AI Assistant"), Palette::panel,
                               juce::DocumentWindow::closeButton),
          present_(std::move(present)) {
        setUsingNativeTitleBar(true);
        pane_ = new AssistantPane(host, std::move(onPatchEdited));
        setContentOwned(pane_, true);
        setResizable(true, true);
        setResizeLimits(360, 320, 900, 1400);
        centreWithSize(480, 560);
    }

    void closeButtonPressed() override {
        setVisible(false);
        if (pane_ == nullptr || !pane_->busy() || !present_) return;
        juce::Component::SafePointer<AssistantWindow> safe(this);
        present_(assistantCardFor(*pane_, [safe] {
            if (safe == nullptr) return;
            safe->setVisible(true);
            safe->toFront(true);
        }));
    }

    bool keyPressed(const juce::KeyPress& k) override {
        if (isCloseWindowKey(k)) { closeButtonPressed(); return true; }
        return juce::DocumentWindow::keyPressed(k);
    }

    AssistantPane& paneForTest() { return *pane_; }

private:
    Present present_;
    AssistantPane* pane_ = nullptr;
};

}
