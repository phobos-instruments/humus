// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/pianoroll/PianoRollEditor.h"

#include <cmath>

#include "gui/common/Localisation.h"
#include "gui/editor/AutomateMenu.h"
#include "gui/host/EngineHostMidiControl.h"
#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

void PianoRollEditor::buildRecordButtons() {
    loop_.setClickingTogglesState(false);
    loop_.setTooltip(tr("piano-roll-editor.loop-recorder",
                        "Loop recorder: tap to record, tap to close the loop, "
                        "tap to overdub (R). Right-click: clear or map."));
    loop_.onClick = [this] { loopTap(); };
    loop_.onRightClick = [this] { showLoopMenu(); };
    addAndMakeVisible(loop_);

    record_.setClickingTogglesState(false);
    record_.setColour(juce::TextButton::buttonOnColourId, ink::state::armed);
    record_.setTooltip(tr("piano-roll-editor.record-incoming-midi-into-this",
                          "Record incoming MIDI into this roll. Right-click to map."));
    record_.onClick = [this] {
        host_.setParam(name_, params_.record, host_.midi().isRecordTarget(name_) ? 0.0 : 1.0);
        syncRecordButtons();
    };
    record_.onRightClick = [this] {
        showAutomateMenu(host_, name_, params_.record, juce::Desktop::getMousePosition(), nullptr, false);
    };
    addAndMakeVisible(record_);

    quantize_.setClickingTogglesState(false);
    quantize_.setTooltip(tr("piano-roll-editor.quantize-recorded-notes-to-the",
                            "Quantize recorded notes to the snap grid. Right-click to map."));
    quantize_.onClick = [this] {
        host_.setParam(name_, params_.quantize, quantize_.getToggleState() ? 0.0 : 1.0);
        syncRecordButtons();
    };
    quantize_.onRightClick = [this] {
        showAutomateMenu(host_, name_, params_.quantize, juce::Desktop::getMousePosition(), nullptr);
    };
    addAndMakeVisible(quantize_);

    host_.midi().setRecordGrid(name_, snapTicks());
    shownLoop_ = loopState();
    syncRecordButtons();
    updateLoopButton();
}

void PianoRollEditor::loopTap() {
    host_.setParam(name_, params_.loop, 1.0);
    host_.setParam(name_, params_.loop, 0.0);
    syncRecordButtons();
}

void PianoRollEditor::showLoopMenu() {
    enum { kClear = 1, kMap };
    juce::PopupMenu m;
    m.addItem(kClear, tr("piano-roll-editor.clear-loop", "Clear loop"));
    m.addItem(kMap, tr("piano-roll-editor.map-loop", "Map Loop"));
    const auto at = juce::Desktop::getMousePosition();
    juce::Component::SafePointer<PianoRollEditor> self(this);
    m.showMenuAsync(juce::PopupMenu::Options(), [self, at](int r) {
        if (self == nullptr) return;
        if (r == kClear) {
            self->host_.midi().clearLoop(self->name_);
            self->syncRecordButtons();
        } else if (r == kMap) {
            showAutomateMenu(self->host_, self->name_, self->params_.loop, at, nullptr, false);
        }
    });
}

void PianoRollEditor::syncRecordButtons() {
    const bool armed = host_.midi().isRecordTarget(name_);
    if (record_.getToggleState() != armed) record_.setToggleState(armed, juce::dontSendNotification);
    const bool quantized = host_.liveParamValue(name_, params_.quantize) >= 0.5;
    if (quantize_.getToggleState() != quantized) quantize_.setToggleState(quantized, juce::dontSendNotification);
    if (const auto st = loopState(); st != shownLoop_) {
        shownLoop_ = st;
        reloadValues();
        updateLoopButton();
    }
}

void PianoRollEditor::updateLoopButton() {
    const auto st = loopState();
    const char* text = "Loop";
    juce::Colour on = Palette::accentDim;
    bool lit = true;
    switch (st) {
        case LooperState::Empty: text = "Loop"; lit = false; break;
        case LooperState::Rec:   text = "Rec";  on = ink::state::armed; break;
        case LooperState::Play:  text = "Play"; on = Palette::accentDim; break;
        case LooperState::Dub:   text = "Dub";  on = ink::state::overdubbing; break;
    }
    juce::String t(text);
    if (st == LooperState::Rec)
        t << " " << juce::jmax(1, (int) std::lround(host_.positionBeats() / 4.0));
    loop_.setButtonText(t);
    loop_.setColour(juce::TextButton::buttonOnColourId, on);
    loop_.setToggleState(lit, juce::dontSendNotification);
}

}
