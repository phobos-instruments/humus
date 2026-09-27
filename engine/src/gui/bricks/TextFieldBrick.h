// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/host/BrickHost.h"
#include "gui/style/LookAndFeel.h"
#include "gui/bricks/PolledBrick.h"
#include "gui/editor/inputs/NumberInputs.h"

namespace hum {

class TextFieldBrick : public PolledBrick {
public:
    TextFieldBrick(BrickHost& host, std::string organism, std::string param,
                   const juce::String& hint, int lines = 1)
        : PolledBrick(host, organism, 4), text_(host, organism, std::move(param)) {
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
        field_.setText(juce::String(text_.text()),
                       juce::dontSendNotification);
    }
    void poll() override {}
    int preferredContentWidth() const override { return 200; }
    int preferredContentHeight(int) const override { return 24; }
    void resized() override { field_.setBounds(getLocalBounds()); }

private:
    void commit() { text_.commit(field_.getText().toStdString()); }  // utf8-ok

    input::TextInput text_;
    juce::TextEditor field_;
};

}
