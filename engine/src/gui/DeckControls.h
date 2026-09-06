#pragma once
#include <array>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/EngineHost.h"
#include "gui/LookAndFeel.h"
#include "gui/Localisation.h"

namespace hum {

struct RollButton : juce::TextButton {
    std::function<void()> onDown, onUp;
    void mouseDown(const juce::MouseEvent& e) override {
        juce::TextButton::mouseDown(e); if (onDown) onDown();
    }
    void mouseUp(const juce::MouseEvent& e) override {
        if (onUp) onUp(); juce::TextButton::mouseUp(e);
    }
};

class DeckControls : public juce::Component, private juce::Timer {
public:
    DeckControls(EngineHost& host, std::string organism)
        : host_(host), name_(std::move(organism)),
          setCue_(tr("deck-controls.set-cue", "Set Cue")), cue_("Cue"), loopTgl_("Loop"), half_("/2"), dbl_("x2") {
        for (int i = 0; i < 8; ++i) {
            hot_[(size_t) i].setButtonText(juce::String(i + 1));
            styleBtn(hot_[(size_t) i]);
            hot_[(size_t) i].onClick = [this, i] {
                const std::string p = "HotCue_" + std::to_string(i + 1);
                if (juce::ModifierKeys::getCurrentModifiers().isShiftDown()) host_.decks().clearHotCue(name_, i + 1);
                else if (host_.liveParamValue(name_, p) > 0.0) host_.decks().jumpHotCue(name_, i + 1);
                else host_.decks().setHotCue(name_, i + 1);
            };
        }
        for (double b : { 1.0, 4.0, 8.0 }) loopSizes_.push_back(b);
        for (auto* b : { &setCue_, &cue_, &loopTgl_, &half_, &dbl_ }) styleBtn(*b);
        for (auto& b : hot_) b.setTriggeredOnMouseDown(true);
        setCue_.setTriggeredOnMouseDown(true);
        cue_.setTriggeredOnMouseDown(true);
        setCue_.onClick = [this] { host_.decks().setCue(name_); };
        cue_.onClick    = [this] { host_.decks().jumpCue(name_); };
        loopTgl_.onClick = [this] { host_.decks().toggleLoop(name_); };
        half_.onClick   = [this] { host_.decks().scaleBeatLoop(name_, 0.5); };
        dbl_.onClick    = [this] { host_.decks().scaleBeatLoop(name_, 2.0); };
        for (size_t i = 0; i < loopSizes_.size(); ++i) {
            loopBtns_[i].setButtonText(loopSizes_[i] < 1.0 ? juce::String(loopSizes_[i], 2)
                                                           : juce::String((int) loopSizes_[i]));
            styleBtn(loopBtns_[i]);
            const double sz = loopSizes_[i];
            loopBtns_[i].onClick = [this, sz] { host_.decks().setBeatLoop(name_, sz); };
        }
        rollLabel_.setText(tr("deck-controls.roll", "Roll"), juce::dontSendNotification);
        rollLabel_.setColour(juce::Label::textColourId, Palette::textDim);
        rollLabel_.setFont(juce::FontOptions(10.0f));
        addAndMakeVisible(rollLabel_);
        const char* rollLabels[4] = { "1/8", "1/4", "1/2", "1" };
        const double rollSz[4] = { 0.125, 0.25, 0.5, 1.0 };
        for (int i = 0; i < 4; ++i) {
            rollBtns_[(size_t) i].setButtonText(rollLabels[i]);
            styleBtn(rollBtns_[(size_t) i]);
            const double sz = rollSz[i];
            rollBtns_[(size_t) i].onDown = [this, sz] { host_.decks().beginLoopRoll(name_, sz); };
            rollBtns_[(size_t) i].onUp = [this] { host_.decks().endLoopRoll(name_); };
        }
        startTimerHz(20);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(2);
        auto row1 = r.removeFromTop(22);
        const int hw = row1.getWidth() / 8;
        for (int i = 0; i < 8; ++i) hot_[(size_t) i].setBounds(row1.removeFromLeft(hw).reduced(1));
        r.removeFromTop(3);
        auto row2 = r.removeFromTop(22);
        setCue_.setBounds(row2.removeFromLeft(56).reduced(1));
        cue_.setBounds(row2.removeFromLeft(44).reduced(1));
        row2.removeFromLeft(6);
        loopTgl_.setBounds(row2.removeFromLeft(48).reduced(1));
        half_.setBounds(row2.removeFromLeft(28).reduced(1));
        dbl_.setBounds(row2.removeFromLeft(28).reduced(1));
        row2.removeFromLeft(6);
        for (size_t i = 0; i < loopSizes_.size(); ++i)
            loopBtns_[i].setBounds(row2.removeFromLeft(34).reduced(1));
        r.removeFromTop(3);
        auto row3 = r.removeFromTop(22);
        auto rollLbl = row3.removeFromLeft(30);
        rollLabel_.setBounds(rollLbl);
        for (int i = 0; i < 4; ++i) rollBtns_[(size_t) i].setBounds(row3.removeFromLeft(38).reduced(1));
    }

private:
    void styleBtn(juce::Button& b) {
        b.setColour(juce::TextButton::buttonColourId, Palette::panelLight);
        b.setColour(juce::TextButton::buttonOnColourId, Palette::accent);
        b.setColour(juce::TextButton::textColourOffId, Palette::text);
        b.setColour(juce::TextButton::textColourOnId, Palette::background);
        addAndMakeVisible(b);
    }

    void timerCallback() override {
        for (int i = 0; i < 8; ++i)
            hot_[(size_t) i].setToggleState(host_.liveParamValue(name_, "HotCue_" + std::to_string(i + 1)) >= 0.0,
                                            juce::dontSendNotification);
        loopTgl_.setToggleState(host_.liveParamValue(name_, "Loop") >= 0.5, juce::dontSendNotification);
    }

    EngineHost& host_;
    std::string name_;
    std::array<juce::TextButton, 8> hot_;
    juce::TextButton setCue_, cue_, loopTgl_, half_, dbl_;
    std::vector<double> loopSizes_;
    std::array<juce::TextButton, 3> loopBtns_;
    juce::Label rollLabel_;
    std::array<RollButton, 4> rollBtns_;
};

}
