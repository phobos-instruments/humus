// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <utility>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/app/TimedCard.h"
#include "gui/common/Localisation.h"
#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

class AssistantCard : public TimedCard, private juce::Timer {
public:
    enum class State { Working, Done, Stopped };

    struct Hooks {
        std::function<bool()> working;
        std::function<juce::String()> reply;
        std::function<void()> stop;
        std::function<void()> show;
    };

    static constexpr double kDoneLingerSeconds = 10.0;

    AssistantCard(juce::String asked, Hooks hooks)
        : asked_(std::move(asked)), hooks_(std::move(hooks)) {
        stop_.setButtonText(tr("assistant-card.stop", "Stop"));
        stop_.onClick = [this] { stopped(); };
        addAndMakeVisible(stop_);
        show_.setButtonText(tr("assistant-card.show", "Show"));
        show_.onClick = [this] {
            auto open = hooks_.show;
            if (open) open();
            dismiss();
        };
        addAndMakeVisible(show_);
        hideDismiss();
        keep();
        startTimerHz(kTickHz);
        setSize(kCardWidth, 112);
        layout();
    }

    State state() const { return state_; }
    const juce::String& message() const { return message_; }
    juce::TextButton& stopButton() { return stop_; }
    juce::TextButton& showButton() { return show_; }
    int secondsForTest() const { return ticks_ / kTickHz; }
    void tickForTest(int ticks) {
        for (int i = 0; i < ticks; ++i) timerCallback();
    }

    void paint(juce::Graphics& g) override {
        paintBody(g, true);
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        g.drawText(tr("assistant-card.title", "AI Assistant"), 14, 10, titleWidth(), 20,
                   juce::Justification::centredLeft);
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(12.0f));
        g.drawText("\"" + asked_ + "\"", 14, 30, getWidth() - 28, 16,
                   juce::Justification::centredLeft, true);
        if (state_ == State::Working) {
            g.setColour(Palette::text);
            g.drawText(tr("assistant-card.working", "Still working...") + " "
                           + juce::String(ticks_ / kTickHz) + " s",
                       14, 48, getWidth() - 28, 16, juce::Justification::centredLeft);
            paintWorkingBar(g, {14.0f, 67.0f, (float) getWidth() - 28.0f, 4.0f}, ticks_);
        } else {
            g.setColour(state_ == State::Done ? Palette::text : Palette::textDim);
            g.drawFittedText(message_, 14, 48, getWidth() - 28, 26,
                             juce::Justification::topLeft, 2);
        }
    }

    void layout() override {
        stop_.setVisible(state_ == State::Working);
        placeButtons(buttonRow(), {&stop_, &show_});
    }

private:
    void stopped() {
        if (state_ != State::Working) return;
        auto stop = hooks_.stop;
        if (stop) stop();
        settle(State::Stopped, tr("assistant-card.stopped", "Stopped. The turn was left where it was."));
    }

    void landed() {
        const auto reply = hooks_.reply ? hooks_.reply() : juce::String();
        settle(State::Done, reply.isNotEmpty()
                                ? reply
                                : tr("assistant-card.finished", "Finished. Open it to read the reply."));
    }

    void settle(State s, juce::String message) {
        state_ = s;
        message_ = std::move(message);
        stopTimer();
        layout();
        repaint();
        expireIn(kDoneLingerSeconds);
    }

    void timerCallback() override {
        ++ticks_;
        if (state_ == State::Working && hooks_.working && !hooks_.working()) { landed(); return; }
        repaint();
    }

    juce::String asked_, message_;
    Hooks hooks_;
    State state_ = State::Working;
    int ticks_ = 0;
    juce::TextButton stop_, show_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AssistantCard)
};

}
