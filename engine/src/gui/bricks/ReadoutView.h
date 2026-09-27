// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/host/BrickHost.h"
#include "gui/style/LookAndFeel.h"
#include "gui/bricks/PolledBrick.h"
#include "gui/editor/readouts/ControlReadings.h"

namespace hum {

class ReadoutView : public PolledBrick {
public:
    ReadoutView(BrickHost& host, std::string name, int decimals)
        : PolledBrick(host, std::move(name), 2), reading_(decimals) {}

    void reloadValues() override {}
    void refreshAutomatedValues() override {}
    int preferredContentWidth() const override { return 277; }
    int preferredContentHeight(int) const override { return 24; }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds();
        g.setColour(Palette::background.darker(0.25f));
        g.fillRoundedRectangle(r.toFloat(), 4.0f);
        g.setColour(Palette::border);
        g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 4.0f, 1.0f);
        r.reduce(8, 2);
        const int count = reading_.count();
        const int n = std::max(1, count);
        const int cols = n > 4 ? 2 : 1;
        const int rows = (n + cols - 1) / cols;
        const int cellW = r.getWidth() / cols;
        const int cellH = std::max(14, r.getHeight() / rows);
        for (int i = 0; i < n; ++i) {
            auto cell = juce::Rectangle<int>(r.getX() + (i / rows) * cellW,
                                             r.getY() + (i % rows) * cellH, cellW, cellH);
            const bool have = i < count;
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(10.0f));
            g.drawText(have ? juce::String::fromUTF8(reading_.name(i).c_str()) : juce::String("-"), cell.removeFromLeft(cell.getWidth() / 2),
                       juce::Justification::centredLeft);
            g.setColour(have ? Palette::text : Palette::textDim);
            g.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
            g.drawText(have ? juce::String(reading_.value(i), reading_.decimals()) : juce::String("-"), cell,
                       juce::Justification::centredRight);
        }
    }

private:
    void poll() override {
        if (reading_.poll(host_, name_)) repaint();
    }

    readout::ControlValues reading_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReadoutView)
};

}
