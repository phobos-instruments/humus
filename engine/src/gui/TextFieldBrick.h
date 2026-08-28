#pragma once
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/EngineHost.h"
#include "gui/LookAndFeel.h"
#include "gui/PolledBrick.h"

namespace hum {

class TextFieldBrick : public PolledBrick {
public:
    TextFieldBrick(EngineHost& host, std::string organism, std::string param,
                   const juce::String& hint)
        : PolledBrick(host, std::move(organism), 4), pn_(std::move(param)) {
        field_.setColour(juce::TextEditor::backgroundColourId, Palette::background);
        field_.setColour(juce::TextEditor::textColourId, Palette::text);
        field_.setColour(juce::TextEditor::outlineColourId, Palette::panelLight);
        field_.setColour(juce::TextEditor::focusedOutlineColourId, Palette::accent);
        field_.setTextToShowWhenEmpty(hint, Palette::textDim);
        field_.onReturnKey = [this] { commit(); };
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
