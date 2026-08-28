#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/EngineHost.h"
#include "gui/LookAndFeel.h"

namespace hum {

class NotesView : public juce::Component {
public:
    explicit NotesView(EngineHost& host) : host_(host) {
        editor_.setMultiLine(true, true);
        editor_.setReturnKeyStartsNewLine(true);
        editor_.setTabKeyUsedAsCharacter(true);
        editor_.setScrollbarsShown(true);
        editor_.setFont(juce::Font(juce::FontOptions(14.0f)));
        editor_.setText(juce::String::fromUTF8(host_.notes().c_str()), false);
        editor_.onTextChange = [this] { host_.setNotes(editor_.getText().toStdString()); };
        addAndMakeVisible(editor_);
        setSize(460, 380);
    }

    void reload() {
        editor_.setText(juce::String::fromUTF8(host_.notes().c_str()), false);
    }

    void resized() override { editor_.setBounds(getLocalBounds().reduced(8)); }
    void paint(juce::Graphics& g) override { g.fillAll(Palette::background); }

private:
    EngineHost& host_;
    juce::TextEditor editor_;
};

}
