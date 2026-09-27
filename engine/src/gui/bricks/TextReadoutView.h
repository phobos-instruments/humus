// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/host/BrickHost.h"
#include "gui/style/LookAndFeel.h"
#include "gui/bricks/PolledBrick.h"
#include "gui/editor/readouts/ControlReadings.h"

namespace hum {

class TextReadoutView : public PolledBrick {
public:
    TextReadoutView(BrickHost& host, std::string name) : PolledBrick(host, std::move(name), 4) {}

    void reloadValues() override {}
    void refreshAutomatedValues() override {}
    int preferredContentWidth() const override { return 277; }
    int preferredContentHeight(int) const override { return 40; }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds();
        g.setColour(Palette::background.darker(0.25f));
        g.fillRoundedRectangle(r.toFloat(), 4.0f);
        g.setColour(Palette::border);
        g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 4.0f, 1.0f);
        r.reduce(8, 3);
        const int count = reading_.count();
        const int n = std::max(1, count);
        const int lineH = std::max(12, r.getHeight() / n);
        g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::plain));
        for (int i = 0; i < n; ++i) {
            g.setColour(i < count ? Palette::text : Palette::textDim);
            g.drawText(i < count ? juce::String(reading_.line(i)) : juce::String("-"), r.removeFromTop(lineH),
                       juce::Justification::centredLeft, true);
        }
    }

private:
    void poll() override {
        if (reading_.poll(host_, name_)) repaint();
    }

    readout::TextLines reading_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextReadoutView)
};

}
