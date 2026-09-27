// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <string>
#include <utility>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/bricks/PolledBrick.h"
#include "gui/common/Localisation.h"
#include "gui/editor/readouts/ScopeReadings.h"
#include "gui/host/BrickHost.h"
#include "gui/style/Colours.h"
#include "gui/style/Contrast.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

class TimecodeReadoutView : public PolledBrick {
public:
    TimecodeReadoutView(BrickHost& host, std::string name)
        : PolledBrick(host, std::move(name)) {}

    int preferredContentWidth() const override { return 372; }
    int preferredContentHeight(int) const override { return 26; }

    void poll() override {
        if (reading_.poll(host_, name_)) repaint();
    }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds();
        g.setColour(wellColour(Palette::background, 0.25f));
        g.fillRoundedRectangle(r.toFloat(), 4.0f);
        g.setColour(Palette::border);
        g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 4.0f, 1.0f);
        r.reduce(6, 4);

        g.setFont(juce::FontOptions(10.0f));
        g.setColour(Palette::textDim);
        auto caption = r.removeFromLeft(38);
        g.drawText(tr("timecode-readout.signal", "signal"), caption,
                   juce::Justification::centredLeft, false);

        auto bar = r.removeFromLeft(96).reduced(0, 4);
        drawBar(g, bar);
        r.removeFromLeft(8);
        g.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
        g.setColour(reading_.present() ? Palette::text : Palette::textDim.withAlpha(alpha::dim));
        g.drawText(readingText(), r, juce::Justification::centredLeft, false);

        if (reading_.on() && reading_.restCarrierHz() > 1.0f) {
            g.setFont(juce::FontOptions(10.0f));
            g.setColour(Palette::textDim);
            g.drawText(juce::String(juce::roundToInt(reading_.restCarrierHz()))
                           + juce::String(tr("timecode-readout.hz-at-1x", " Hz = 1x")),
                       r, juce::Justification::centredRight, false);
        }
    }

    juce::String readingTextForTest() const { return readingText(); }

private:
    static juce::Colour wellColour(juce::Colour base, float deepen) {
        return contrast::isLight(base) ? base.darker(deepen * 0.35f) : base.darker(deepen);
    }

    void drawBar(juce::Graphics& g, juce::Rectangle<int> bar) const {
        g.setColour(wellColour(Palette::panel, 0.35f));
        g.fillRoundedRectangle(bar.toFloat(), 2.0f);
        const float f = juce::jlimit(0.0f, 1.0f, reading_.level() * 4.0f);
        if (f <= 0.0f) return;
        auto lit = bar.toFloat().withWidth(bar.toFloat().getWidth() * f);
        g.setColour(reading_.present() ? ink::state::ok : Palette::warnAmber());
        g.fillRoundedRectangle(lit, 2.0f);
    }

    juce::String readingText() const {
        if (!reading_.on())
            return tr("timecode-readout.timecode-off", "Timecode off");
        if (!reading_.present())
            return tr("timecode-readout.needle-up", "needle up");
        const float s = reading_.speed();
        const auto times = juce::String(std::abs(s), 2)
                           + juce::String(tr("timecode-readout.times", "x"));
        if (std::abs(s) < 0.02f) return times + juce::String(tr("timecode-readout.held", "  held"));
        return times + (s > 0.0f ? juce::String(tr("timecode-readout.forward", "  forward"))
                                 : juce::String(tr("timecode-readout.backwards", "  backwards")));
    }

    readout::Timecode reading_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimecodeReadoutView)
};

}
