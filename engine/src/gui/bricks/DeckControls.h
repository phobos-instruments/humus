// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <array>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/bricks/MomentaryButton.h"
#include "gui/editor/AutomateMenu.h"
#include "gui/editor/ParamRanges.h"
#include "gui/editor/BrickBindings.h"
#include "gui/editor/Mappable.h"
#include "gui/host/BrickHost.h"
#include "gui/editor/decks/DeckModels.h"
#include "gui/editor/files/TransportModel.h"
#include "gui/host/EngineHostDeck.h"
#include "core/packs/Categories.h"
#include "gui/style/LookAndFeel.h"
#include "gui/common/Localisation.h"

namespace hum {

inline constexpr const char* kRollTriggers[4] = { "Roll_1_8", "Roll_1_4", "Roll_1_2", "Roll_1" };

using CueButton = Mappable<juce::TextButton>;
using StepPicker = Mappable<juce::ComboBox>;
using HeldButton = Mappable<MomentaryButton>;

inline constexpr const char* kClearAllTrigger = "PadClearAll";
inline constexpr int kCueRowGap = 3;

struct RollButton : CueButton {
    std::function<void()> onDown, onUp;
    void mouseDown(const juce::MouseEvent& e) override {
        CueButton::mouseDown(e);
        if (!e.mods.isPopupMenu() && onDown) onDown();
    }
    void mouseUp(const juce::MouseEvent& e) override {
        if (!e.mods.isPopupMenu() && onUp) onUp();
        CueButton::mouseUp(e);
    }
};

class DeckControls : public juce::Component, private juce::Timer {
public:
    DeckControls(BrickHost& host, std::string organism, const Bindings& bound)
        : host_(host), name_(organism),
          pad_(host, host.decks(), std::move(organism),
               {bound(bind::kBpm), bound(bind::kCue), bound(bind::kGridOffset), bound(bind::kHotCuePrefix),
                bound(bind::kLoop), bound(bind::kLoopBeats), bound(bind::kLoopIn), bound(bind::kLoopOut),
                bound(bind::kQuantize)}),
          setCue_(tr("deck-controls.set-cue", "Set Cue")), cue_("Cue"), loopTgl_("Loop"),
          half_("/2"), dbl_("x2"), clear_(tr("deck-controls.clear", "Clear")),
          clearAll_(kClearAllTrigger, tr("deck-controls.clear-all", "All"), false, true),
          nudgeBack_(juce::String::fromUTF8("\xe2\x80\x93")), nudgeOn_("+"),
          stepParam_(bound(bind::kNudgeStep)) {
        for (int i = 0; i < 8; ++i) {
            hot_[(size_t) i].setButtonText(juce::String(i + 1));
            styleBtn(hot_[(size_t) i], "Pad_" + std::to_string(i + 1));
            hot_[(size_t) i].onClick = [this, i] {
                if (juce::ModifierKeys::getCurrentModifiers().isShiftDown()) pad_.hotCue(i + 1, true);
                else pad_.fire("Pad_" + std::to_string(i + 1));
            };
        }
        for (double b : decks::DeckPad::kLoopBeats) loopSizes_.push_back(b);
        clear_.setTriggeredOnMouseDown(true);
        clear_.setTooltip(tr("deck-controls.clear-the-cue-you-last-used",
                             "Clear the cue you last used"));
        styleBtn(clear_, "PadClear");
        clear_.onClick = [this] { pad_.fire("PadClear"); };
        dressBtn(clearAll_);
        clearAll_.setTooltip(tr("deck-controls.hold-to-clear-every-cue",
                                "Hold to clear every cue on this deck"));
        clearAll_.setWriter([this](double value) { pad_.hold(kClearAllTrigger, value >= 0.5); }, {});
        markable(clearAll_, kClearAllTrigger);
        addAndMakeVisible(clearAll_);
        nudgeBack_.setTriggeredOnMouseDown(true);
        nudgeOn_.setTriggeredOnMouseDown(true);
        nudgeBack_.setTooltip(tr("deck-controls.nudge-cue-back",
                                 "Move the cue you last used a little earlier"));
        nudgeOn_.setTooltip(tr("deck-controls.nudge-cue-on",
                               "Move the cue you last used a little later"));
        styleBtn(nudgeBack_, "CueNudgeBack");
        styleBtn(nudgeOn_, "CueNudgeForward");
        nudgeBack_.onClick = [this] { pad_.fire("CueNudgeBack"); };
        nudgeOn_.onClick   = [this] { pad_.fire("CueNudgeForward"); };
        for (int i = 0; i < kNudgeSteps; ++i)
            nudgeStep_.addItem(juce::String(nudgeStepText(i)), i + 1);
        nudgeStep_.setTooltip(tr("deck-controls.how-far-each-nudge-moves-a-cue",
                                 "How far each nudge moves a cue"));
        nudgeStep_.onChange = [this] {
            host_.setParam(name_, stepParam_, (double) (nudgeStep_.getSelectedId() - 1));
        };
        markable(nudgeStep_, stepParam_);
        nudgeStep_.setSelectedId((int) host.liveParamValue(name_, stepParam_) + 1,
                                 juce::dontSendNotification);
        addAndMakeVisible(nudgeStep_);
        styleBtn(setCue_, "CueSet");
        styleBtn(cue_, "CueJump");
        styleBtn(loopTgl_, bound(bind::kLoop));
        styleBtn(half_, "LoopHalve");
        styleBtn(dbl_, "LoopDouble");
        for (auto& b : hot_) b.setTriggeredOnMouseDown(true);
        setCue_.setTriggeredOnMouseDown(true);
        cue_.setTriggeredOnMouseDown(true);
        setCue_.onClick = [this] { pad_.fire("CueSet"); };
        cue_.onClick    = [this] { pad_.fire("CueJump"); };
        loopTgl_.onClick = [this] { pad_.toggleLoop(); };
        half_.onClick   = [this] { pad_.fire("LoopHalve"); };
        dbl_.onClick    = [this] { pad_.fire("LoopDouble"); };
        for (size_t i = 0; i < loopSizes_.size(); ++i) {
            loopBtns_[i].setButtonText(loopSizes_[i] < 1.0 ? juce::String(loopSizes_[i], 2)
                                                           : juce::String((int) loopSizes_[i]));
            const auto trigger = "BeatLoop_" + juce::String(loopSizes_[i] < 1.0
                                     ? juce::String(loopSizes_[i], 2) : juce::String((int) loopSizes_[i]));
            styleBtn(loopBtns_[i], trigger.toStdString());
            loopBtns_[i].onClick = [this, trigger] { pad_.fire(trigger.toStdString()); };
        }
        auto rowLabel = [this](juce::Label& l, const juce::String& text) {
            l.setText(text, juce::dontSendNotification);
            l.setColour(juce::Label::textColourId, Palette::textDim);
            l.setFont(juce::FontOptions(10.0f));
            addAndMakeVisible(l);
        };
        rowLabel(cueLabel_, tr("deck-controls.cues", "Cues"));
        rowLabel(loopLabel_, tr("deck-controls.loop", "Loop"));
        rowLabel(rollLabel_, tr("deck-controls.roll", "Roll"));
        const char* rollLabels[4] = { "1/8", "1/4", "1/2", "1" };
        for (int i = 0; i < 4; ++i) {
            rollBtns_[(size_t) i].setButtonText(rollLabels[i]);
            const std::string trigger = kRollTriggers[i];
            styleBtn(rollBtns_[(size_t) i], trigger);
            rollBtns_[(size_t) i].onDown = [this, trigger] { pad_.hold(trigger, true); };
            rollBtns_[(size_t) i].onUp = [this, trigger] { pad_.hold(trigger, false); };
        }
        startTimerHz(20);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(2);
        constexpr int rows = 4, labelW = 30, rightW = 148;
        const int rowH = std::max(14, (r.getHeight() - kCueRowGap * (rows - 1)) / rows);
        auto nextRow = [&r, rowH](bool first) {
            if (!first) r.removeFromTop(kCueRowGap);
            return r.removeFromTop(rowH);
        };
        auto spread = [](juce::Rectangle<int>& row, auto& buttons, int from, int to) {
            for (int i = from; i < to; ++i)
                buttons[(size_t) i].setBounds(row.removeFromLeft(row.getWidth() / (to - i)).reduced(1));
        };

        auto top = nextRow(true);
        cueLabel_.setBounds(top.removeFromLeft(labelW));
        auto topRight = top.removeFromRight(rightW);
        nudgeStep_.setBounds(topRight.removeFromRight(72).reduced(1));
        clear_.setBounds(topRight.removeFromRight(50).reduced(1));
        nudgeBack_.setBounds(topRight.removeFromRight(24).reduced(1));
        spread(top, hot_, 0, 4);

        auto second = nextRow(false);
        second.removeFromLeft(labelW);
        auto secondRight = second.removeFromRight(rightW);
        secondRight.removeFromRight(72);
        clearAll_.setBounds(secondRight.removeFromRight(50).reduced(1));
        nudgeOn_.setBounds(secondRight.removeFromRight(24).reduced(1));
        spread(second, hot_, 4, 8);

        auto loops = nextRow(false);
        loopLabel_.setBounds(loops.removeFromLeft(labelW));
        setCue_.setBounds(loops.removeFromLeft(56).reduced(1));
        cue_.setBounds(loops.removeFromLeft(44).reduced(1));
        loops.removeFromLeft(6);
        loopTgl_.setBounds(loops.removeFromLeft(48).reduced(1));
        half_.setBounds(loops.removeFromLeft(28).reduced(1));
        dbl_.setBounds(loops.removeFromLeft(28).reduced(1));
        loops.removeFromLeft(6);
        for (size_t i = 0; i < loopSizes_.size(); ++i)
            loopBtns_[i].setBounds(loops.removeFromLeft(34).reduced(1));

        auto rolls = nextRow(false);
        rollLabel_.setBounds(rolls.removeFromLeft(labelW));
        for (int i = 0; i < 4; ++i) rollBtns_[(size_t) i].setBounds(rolls.removeFromLeft(38).reduced(1));
    }

private:
    template <typename Button>
    void dressBtn(Button& b) {
        const auto family = familyOf(className());
        b.onRestyle = [&b, family] {
            b.setColour(juce::TextButton::buttonColourId, Palette::panelLight);
            b.setColour(juce::TextButton::buttonOnColourId, Palette::familyAccent(family));
            b.setColour(juce::TextButton::textColourOffId, Palette::text);
            b.setColour(juce::TextButton::textColourOnId, Palette::background);
        };
        b.onRestyle();
    }

    std::string className() const {
        const auto* cm = host_.model().byName(name_);
        return cm != nullptr ? cm->classRaw : std::string();
    }

    void styleBtn(CueButton& b, const std::string& param) {
        dressBtn(b);
        markable(b, param);
        addAndMakeVisible(b);
    }

    template <typename Button>
    void markable(Button& b, const std::string& param) {
        b.onRightClick = [this, param](juce::Point<int> at) {
            showAutomateMenu(host_, name_, param, at, [this] { repaint(); });
        };
        learned_.push_back({[&b](bool controlled, bool locked, bool lit) {
                                b.setMarks(controlled, locked);
                                b.setLit(lit);
                            },
                            param});
    }

    void holdNudge() {
        const int way = nudgeBack_.isDown() ? -1 : nudgeOn_.isDown() ? 1 : 0;
        if (way == 0) {
            heldTicks_ = 0;
            return;
        }
        const int repeats = (int) (files::nudgeHoldStep(++heldTicks_) + 0.5);
        for (int i = 0; i < repeats; ++i)
            pad_.fire(way < 0 ? "CueNudgeBack" : "CueNudgeForward");
    }

    void timerCallback() override {
        for (int i = 0; i < 8; ++i)
            hot_[(size_t) i].setToggleState(pad_.hotCueLit(i + 1), juce::dontSendNotification);
        loopTgl_.setToggleState(pad_.looping(), juce::dontSendNotification);
        if (const int want = (int) host_.liveParamValue(name_, stepParam_) + 1;
            want != nudgeStep_.getSelectedId())
            nudgeStep_.setSelectedId(want, juce::dontSendNotification);
        holdNudge();
        for (const auto& [mark, param] : learned_)
            mark(paramIsControlled(host_, name_, param), host_.rollLocked(name_, param),
                 host_.firedRecently(name_, param));
    }

    struct Learned { std::function<void(bool, bool, bool)> mark; std::string param; };

    BrickHost& host_;
    std::string name_;
    decks::DeckPad pad_;
    std::array<CueButton, 8> hot_;
    CueButton setCue_, cue_, loopTgl_, half_, dbl_, clear_;
    HeldButton clearAll_;
    CueButton nudgeBack_, nudgeOn_;
    StepPicker nudgeStep_;
    std::string stepParam_;
    int heldTicks_ = 0;
    std::vector<double> loopSizes_;
    std::array<CueButton, 3> loopBtns_;
    juce::Label cueLabel_, loopLabel_, rollLabel_;
    std::array<RollButton, 4> rollBtns_;
    std::vector<Learned> learned_;
};

}
