// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/host/BrickHost.h"
#include "gui/style/LookAndFeel.h"
#include "gui/bricks/PolledBrick.h"

namespace hum {

class TextFieldBrick : public PolledBrick {
public:
    TextFieldBrick(BrickHost& host, std::string organism, std::string param,
                   const juce::String& hint, int lines = 1)
        : PolledBrick(host, std::move(organism), 4), pn_(std::move(param)) {
        if (lines > 1) {
            field_.setMultiLine(true, false);
            field_.setReturnKeyStartsNewLine(true);
        }
        field_.setColour(juce::TextEditor::backgroundColourId, Palette::background);
        field_.setColour(juce::TextEditor::textColourId, Palette::text);
        field_.setColour(juce::TextEditor::outlineColourId, Palette::panelLight);
        field_.setColour(juce::TextEditor::focusedOutlineColourId, Palette::accent);
        field_.setTextToShowWhenEmpty(hint, Palette::textDim);
        if (lines <= 1) field_.onReturnKey = [this] { commit(); };
        field_.onFocusLost = [this] { commit(); };
        addAndMakeVisible(field_);
        reloadValues();
    }

    void reloadValues() override {
        if (field_.hasKeyboardFocus(true)) return;
        field_.setText(juce::String(juce::CharPointer_UTF8(
                           host_.liveParamText(name_, pn_).c_str())),
                       juce::dontSendNotification);
    }
    void poll() override {}
    int preferredContentWidth() const override { return 200; }
    int preferredContentHeight(int) const override { return 24; }
    void resized() override { field_.setBounds(getLocalBounds()); }

private:
    void commit() { host_.setParamText(name_, pn_, field_.getText().toStdString()); }  // utf8-ok

    std::string pn_;
    juce::TextEditor field_;
};

}
