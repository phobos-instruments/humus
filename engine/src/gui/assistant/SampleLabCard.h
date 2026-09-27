// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/app/AppPaths.h"
#include "core/assistant/RecipeSynth.h"
#include "core/packs/Roles.h"
#include "io/PatchDocument.h"
#include "gui/app/TimedCard.h"
#include "gui/assistant/AiClient.h"
#include "gui/common/Localisation.h"
#include "gui/host/AssistantHost.h"
#include "gui/style/LookAndFeel.h"
#include "io/WavWriter.h"

#include "hum/dsp/DspMath.h"

namespace hum {

namespace samplelab {

struct Landing {
    bool ok = false;
    juce::String message;
};

inline juce::File sampleFolder() {
    auto dir = userLibraryRoot().getChildFile("Library").getChildFile("Samples");
    dir.createDirectory();
    return dir;
}

inline juce::String sanitize(const std::string& name) {
    return juce::File::createLegalFileName(juce::String(name)).replaceCharacter(' ', '-');
}

inline Landing applyRecipes(AssistantHost& host, const std::string& targetSampler,
                            const std::vector<Recipe>& recipes) {
    if (recipes.empty())
        return {false, tr("sample-lab.the-reply-contained-no-usable", "The reply contained no usable sounds.")};
    const auto stamp = juce::Time::getCurrentTime().formatted("%H%M%S");
    std::vector<juce::String> paths;
    for (const auto& r : recipes) {
        const auto mono = renderRecipe(r, kDefaultSampleRate);
        const auto f = sampleFolder().getChildFile(sanitize(r.name) + "-" + stamp + ".wav");
        if (writeWav(f.getFullPathName().toStdString(), {mono}, kDefaultSampleRate))
            paths.push_back(f.getFullPathName());
    }
    if (paths.empty())
        return {false, tr("sample-lab.could-not-write-the-sample", "Could not write the sample files.")};

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
    return {true, tr("sample-lab.landed",
                     "{n} sounds on '{sampler}'. Keys from C4 up play them. Undo takes them back.")
                      .replace("{n}", juce::String((int) paths.size()))
                      .replace("{sampler}", juce::String(sampler))};
}

}

class SampleLabCard : public TimedCard, private juce::Timer {
public:
    enum class State { Designing, Done, Failed, Stopped };
    using Reply = std::function<void(juce::String text, juce::String error)>;
    using Send = std::function<void(AiClient::Request, Reply, AiClient::TicketPtr)>;

    static constexpr double kDoneLingerSeconds = 10.0;

    SampleLabCard(AssistantHost& host, std::string targetSampler, juce::String prompt,
                  std::function<void()> onChanged, Send send = sendToModel)
        : host_(host), target_(std::move(targetSampler)), prompt_(std::move(prompt)),
          onChanged_(std::move(onChanged)), send_(std::move(send)) {
        action_.onClick = [this] { state_ == State::Designing ? stop() : start(); };
        addAndMakeVisible(action_);
        setSize(kCardWidth, 112);
    }

    ~SampleLabCard() override {
        if (ticket_ != nullptr) ticket_->cancel();
    }

    void start() {
        ticket_ = std::make_shared<AiClient::Ticket>();
        AiClient::Request req;
        req.system = recipeSystemPrompt();
        req.user = prompt_;
        req.maxTokens = 2000;
        ticks_ = 0;
        enter(State::Designing, {});
        send_(std::move(req),
              [safe = juce::Component::SafePointer<SampleLabCard>(this), ticket = ticket_](
                  juce::String text, juce::String error) {
                  if (safe == nullptr || ticket->cancelled() || safe->ticket_ != ticket) return;
                  safe->land(text, error);
              },
              ticket_);
    }

    void stop() {
        if (state_ != State::Designing) return;
        if (ticket_ != nullptr) ticket_->cancel();
        finish(State::Stopped, tr("sample-lab-card.stopped", "Stopped. Nothing was added."));
    }

    State state() const { return state_; }
    const juce::String& message() const { return message_; }
    juce::TextButton& actionButton() { return action_; }

    void paint(juce::Graphics& g) override {
        paintBody(g, state_ != State::Failed);
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        g.drawText(tr("sample-lab-card.title", "Sample Lab"), 14, 10, titleWidth(), 20,
                   juce::Justification::centredLeft);
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(12.0f));
        g.drawText("\"" + prompt_ + "\"", 14, 30, getWidth() - 28, 16,
                   juce::Justification::centredLeft, true);
        if (state_ == State::Designing) {
            g.setColour(Palette::text);
            g.drawText(tr("sample-lab-card.designing", "Designing sounds...") + " "
                           + juce::String(ticks_ / kTickHz) + " s",
                       14, 48, getWidth() - 28, 16, juce::Justification::centredLeft);
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

    void land(const juce::String& text, const juce::String& error) {
        if (state_ != State::Designing) return;
        if (error.isNotEmpty()) { finish(State::Failed, error); return; }
        const auto landing = samplelab::applyRecipes(host_, target_, parseRecipes(text));
        if (!landing.ok) { finish(State::Failed, landing.message); return; }
        if (onChanged_) onChanged_();
        finish(State::Done, landing.message);
    }

    void enter(State s, juce::String message) {
        keep();
        state_ = s;
        message_ = std::move(message);
        action_.setButtonText(s == State::Designing ? tr("sample-lab-card.stop", "Stop")
                                                    : tr("sample-lab-card.try-again", "Try again"));
        if (s == State::Designing) startTimerHz(kTickHz); else stopTimer();
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

    AssistantHost& host_;
    std::string target_;
    juce::String prompt_;
    std::function<void()> onChanged_;
    Send send_;
    AiClient::TicketPtr ticket_;
    State state_ = State::Designing;
    juce::String message_;
    int ticks_ = 0;
    juce::TextButton action_;
};

}
