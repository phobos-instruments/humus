#pragma once
#include <memory>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/HelpBody.h"
#include "gui/HelpDocs.h"
#include "gui/LookAndFeel.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class HelpView : public juce::Component {
public:
    HelpView(const juce::String& heading, const HelpDoc& doc, const std::string& cls)
        : body_(doc, cls) {
        heading_.setText(heading, juce::dontSendNotification);
        heading_.setFont(juce::Font(juce::FontOptions(14.0f).withStyle("Bold")));
        heading_.setColour(juce::Label::textColourId, Palette::accent);
        addAndMakeVisible(heading_);

        viewport_.setViewedComponent(&body_, false);
        viewport_.setScrollBarsShown(true, false);
        addAndMakeVisible(viewport_);
        setSize(500, kA4Hz);
    }

    void paint(juce::Graphics& g) override { g.fillAll(Palette::background); }

    void resized() override {
        auto r = getLocalBounds().reduced(10);
        heading_.setBounds(r.removeFromTop(22));
        r.removeFromTop(4);
        viewport_.setBounds(r);
        body_.layoutTo(viewport_.getMaximumVisibleWidth());
    }

    static void show(const std::string& displayClass, juce::Rectangle<int> anchorScreen) {
        auto v = std::make_unique<HelpView>(
            juce::String(displayClass) + juce::String(" - Help"),
            loadOrganismDoc(displayClass), displayClass);
        juce::CallOutBox::launchAsynchronously(std::move(v), anchorScreen, nullptr);
    }

private:
    juce::Label heading_;
    juce::Viewport viewport_;
    HelpBody body_;
};

}
