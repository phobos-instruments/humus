// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/params/ParamSchema.h"
#include "core/assistant/PresetGenie.h"
#include "gui/assistant/AiClient.h"
#include "gui/app/TimedCard.h"
#include "gui/style/Colours.h"
#include "gui/host/BrickHost.h"
#include "gui/common/Localisation.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

class GenieCard : public TimedCard, private juce::Timer {
public:
    enum class State { Thinking, Done, Failed, Stopped };
    using Reply = std::function<void(juce::String text, juce::String error)>;
    using Send = std::function<void(AiClient::Request, Reply, AiClient::TicketPtr)>;

    static constexpr int kDoneLingerSeconds = 6;

    GenieCard(BrickHost& host, std::string node, juce::String prompt,
              std::function<void()> onChanged, Send send = sendToModel)
        : host_(host), node_(std::move(node)), prompt_(std::move(prompt)),
          onChanged_(std::move(onChanged)), send_(std::move(send)) {
        action_.onClick = [this] { state_ == State::Thinking ? stop() : start(); };
        addAndMakeVisible(action_);
        setSize(kCardWidth, 112);
    }

    ~GenieCard() override {
        if (ticket_ != nullptr) ticket_->cancel();
    }

    void start() {
        const auto* cm = host_.model().byName(node_);
        if (cm == nullptr) { finish(State::Failed, "That organism is no longer in the patch."); return; }
        const auto schema = schemaFor(cm->classRaw);
        ticket_ = std::make_shared<AiClient::Ticket>();
        AiClient::Request req;
        req.system = genieSystemPrompt(cm->displayClass, schema);
        req.user = prompt_;
        ticks_ = 0;
        enter(State::Thinking, {});
        send_(std::move(req),
              [safe = juce::Component::SafePointer<GenieCard>(this), ticket = ticket_, schema](
                  juce::String text, juce::String error) {
                  if (safe == nullptr || ticket->cancelled() || safe->ticket_ != ticket) return;
                  safe->land(text, error, schema);
              },
              ticket_);
    }

    void stop() {
        if (state_ != State::Thinking) return;
        if (ticket_ != nullptr) ticket_->cancel();
        finish(State::Stopped, "Stopped. Nothing was changed.");
    }

    State state() const { return state_; }
    const juce::String& message() const { return message_; }
    juce::TextButton& actionButton() { return action_; }

    void paint(juce::Graphics& g) override {
        paintBody(g, state_ != State::Failed);
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        g.drawText("Preset Genie - " + juce::String(node_), 14, 10, titleWidth(), 20,
                   juce::Justification::centredLeft);
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(12.0f));
        g.drawText("\"" + prompt_ + "\"", 14, 30, getWidth() - 28, 16,
                   juce::Justification::centredLeft, true);
        if (state_ == State::Thinking) {
            g.setColour(Palette::text);
            g.drawText("Thinking... " + juce::String(ticks_ / kTickHz) + " s", 14, 48,
                       getWidth() - 28, 16, juce::Justification::centredLeft);
            paintWorkingBar(g, {14.0f, 67.0f, (float) getWidth() - 28.0f, 4.0f}, ticks_);
        } else {
            g.setColour(state_ == State::Done ? Palette::text : Palette::textDim);
            g.drawFittedText(message_, 14, 48, getWidth() - 28, 22,
                             juce::Justification::topLeft, 2);
        }
    }

    void layout() override {
        action_.setVisible(state_ != State::Done);
        placeButtons(buttonRow(), {&action_});
    }

private:
    static void sendToModel(AiClient::Request req, Reply reply, AiClient::TicketPtr ticket) {
        AiClient::complete(std::move(req), std::move(reply), std::move(ticket));
    }

    void land(const juce::String& text, const juce::String& error,
              const std::vector<ParamDesc>& schema) {
        if (state_ != State::Thinking) return;
        if (error.isNotEmpty()) { finish(State::Failed, error); return; }
        if (host_.model().byName(node_) == nullptr) {
            finish(State::Failed, "That organism is no longer in the patch.");
            return;
        }
        const auto vals = parseGenieReply(text, schema);
        if (vals.empty()) { finish(State::Failed, "The reply had no parameters this organism knows."); return; }
        host_.pushUndo();
        for (const auto& [param, value] : vals) host_.setParam(node_, param, value);
        if (onChanged_) onChanged_();
        finish(State::Done, vals.size() == 1 ? juce::String("1 parameter set. Undo takes it back.")
                                             : juce::String((int) vals.size())
                                                   + " parameters set. Undo takes them back.");
    }

    void enter(State s, juce::String message) {
        keep();
        state_ = s;
        message_ = std::move(message);
        action_.setButtonText(s == State::Thinking ? tr("genie-card.stop", "Stop")
                                                   : tr("genie-card.try-again", "Try again"));
        if (s == State::Thinking) startTimerHz(kTickHz); else stopTimer();
        layout();
        repaint();
    }

    void finish(State s, juce::String message) {
        ticks_ = 0;
        enter(s, std::move(message));
        if (s == State::Done) expireIn(kDoneLingerSeconds);
    }

    void timerCallback() override {
        ++ticks_;
        repaint();
    }

    BrickHost& host_;
    std::string node_;
    juce::String prompt_;
    std::function<void()> onChanged_;
    Send send_;
    AiClient::TicketPtr ticket_;
    State state_ = State::Thinking;
    juce::String message_;
    int ticks_ = 0;
    juce::TextButton action_;
};

}
