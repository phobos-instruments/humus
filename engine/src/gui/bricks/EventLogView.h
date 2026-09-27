// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/host/BrickHost.h"
#include "gui/editor/readouts/LogReadings.h"
#include "gui/style/LookAndFeel.h"
#include "gui/bricks/PolledBrick.h"

namespace hum {

class EventLogView : public PolledBrick {
public:
    EventLogView(BrickHost& host, std::string name)
        : PolledBrick(host, std::move(name), 2) {
        pause_.setClickingTogglesState(true);
        addAndMakeVisible(pause_);
        addAndMakeVisible(clear_);
        clear_.onClick = [this] { log_.clear(); repaint(); };
    }

    void reloadValues() override {}
    void refreshAutomatedValues() override {}
    int preferredContentWidth() const override { return 368; }
    int preferredContentHeight(int) const override { return 246; }

    void resized() override {
        auto r = getLocalBounds();
        auto top = r.removeFromTop(22);
        pause_.setBounds(top.removeFromRight(64).reduced(2, 0));
        clear_.setBounds(top.removeFromRight(58).reduced(2, 0));
    }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds();
        auto top = r.removeFromTop(22);
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(11.0f));
        g.drawText(headerText(), top.reduced(4, 0), juce::Justification::centredLeft);
        g.setColour(Palette::background.darker(0.15f));
        g.fillRoundedRectangle(r.toFloat(), 5.0f);
        g.setColour(Palette::border);
        g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 5.0f, 1.0f);
        r.reduce(8, 5);
        g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 11.0f,
                                    juce::Font::plain));
        const int rowH = 15;
        const int visible = juce::jmax(1, r.getHeight() / rowH);
        const auto& lines = log_.lines();
        const int first = juce::jmax(0, (int) lines.size() - visible);
        int y = r.getY();
        for (size_t i = (size_t) first; i < lines.size(); ++i) {
            const auto& ln = lines[i];
            g.setColour(ln.accent ? Palette::accent : Palette::text);
            g.drawText(juce::String::fromUTF8(ln.text.c_str()), r.getX(), y, r.getWidth(), rowH,
                       juce::Justification::centredLeft, true);
            y += rowH;
        }
        if (lines.empty()) {
            g.setColour(Palette::textDim);
            g.drawText(emptyText(), r, juce::Justification::centred);
        }
    }

protected:
    virtual bool drain(readout::EventLog& log) = 0;
    virtual juce::String headerText() const = 0;
    virtual juce::String emptyText() const = 0;

    bool paused() const { return pause_.getToggleState(); }
    size_t lineCount() const { return log_.lines().size(); }

private:
    void poll() override {
        if (!drain(log_)) return;
        log_.trim();
        repaint();
    }

    readout::EventLog log_;
    juce::TextButton pause_{"Pause"}, clear_{"Clear"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EventLogView)
};

}
