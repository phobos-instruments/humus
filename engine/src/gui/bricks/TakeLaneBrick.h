// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/BrickBindings.h"
#include "gui/style/Colours.h"
#include "gui/host/BrickHost.h"
#include "gui/host/EngineHostFiles.h"
#include "gui/style/LookAndFeel.h"
#include "gui/bricks/PolledBrick.h"
#include "gui/editor/inputs/RecorderInputs.h"

namespace hum {

class TakeLaneBrick : public PolledBrick {
public:
    TakeLaneBrick(BrickHost& host, std::string name, int track, const Bindings& bound)
        : PolledBrick(host, name), lane_(host, name, track, bound(bind::kChannelsPrefix)) {}

    int preferredContentWidth() const override { return 208; }
    int preferredContentHeight(int) const override { return 22; }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().reduced(1);
        paintLamp(g, r.removeFromLeft(kLampW));
        r.removeFromLeft(2);
        auto clock = r.removeFromRight(kClockW);
        auto meter = r.removeFromRight(kMeterW);
        r.removeFromRight(3);
        paintHistory(g, r.toFloat());
        paintMeter(g, meter.reduced(2, 1).toFloat());
        paintClock(g, clock);
    }

    double secondsForTest() const { return lane_.seconds(); }
    bool rollingForTest() const { return lane_.rolling(); }
    float historyPeakForTest() const {
        return *std::max_element(lane_.history().begin(), lane_.history().end());
    }
    int firstChannelForTest() const { return lane_.firstChannel(); }
    int channelsForTest() const { return lane_.channels(); }
    int trackForTest() const { return lane_.track(); }

private:
    static constexpr int kHistory = input::TakeLane::kHistory;
    static constexpr int kLampW = 10;
    static constexpr int kMeterW = 26;
    static constexpr int kClockW = 50;

    void poll() override {
        lane_.poll(host_.files().isRecorderActive(name_), juce::Time::getMillisecondCounterHiRes());
        repaint();
    }

    static float bar(float linear) { return input::TakeLane::bar(linear); }

    void paintHistory(juce::Graphics& g, juce::Rectangle<float> r) {
        g.setColour(Palette::background.darker(0.25f));
        g.fillRoundedRectangle(r, 3.0f);
        const auto inner = r.reduced(2.0f);
        const float step = inner.getWidth() / (float) kHistory;
        const float mid = inner.getCentreY();
        const bool rolling = lane_.rolling();
        const auto& history = lane_.history();
        g.setColour(rolling ? Palette::accent : Palette::accentDim.withAlpha(alpha::mid));
        for (int i = 0; i < kHistory; ++i) {
            const float h = bar(history[(size_t) i]) * inner.getHeight() * 0.5f;
            if (h <= 0.0f) continue;
            g.fillRect(inner.getX() + (float) i * step, mid - h, std::max(1.0f, step - 0.5f),
                       h * 2.0f);
        }
        g.setColour(Palette::border.withAlpha(alpha::mist));
        g.drawHorizontalLine((int) mid, inner.getX(), inner.getRight());
    }

    void paintMeter(juce::Graphics& g, juce::Rectangle<float> r) {
        g.setColour(Palette::background.darker(0.25f));
        g.fillRoundedRectangle(r, 2.0f);
        const auto inner = r.reduced(1.0f);
        const float level = lane_.level();
        const float lit = bar(level) * inner.getWidth();
        if (lit <= 0.0f) return;
        g.setColour(level >= 1.0f ? ink::state::danger
                                   : level > 0.7f ? ink::state::caution : ink::state::ok);
        g.fillRect(inner.withWidth(lit));
    }

    void paintLamp(juce::Graphics& g, juce::Rectangle<int> r) {
        const auto dot = r.withSizeKeepingCentre(kLampW - 4, kLampW - 4).toFloat();
        const bool rolling = lane_.rolling();
        if (rolling) {
            g.setColour(Palette::recordRed().withAlpha(alpha::muted));
            g.fillEllipse(dot.expanded(2.5f));
        }
        g.setColour(rolling ? Palette::recordRed() : Palette::border);
        rolling ? g.fillEllipse(dot) : g.drawEllipse(dot, 1.0f);
    }

    void paintClock(juce::Graphics& g, juce::Rectangle<int> r) {
        g.setColour(lane_.rolling() ? Palette::text : Palette::textDim);
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        g.drawText(juce::String(lane_.clockText()), r, juce::Justification::centredRight);
    }

    input::TakeLane lane_;
};

}
