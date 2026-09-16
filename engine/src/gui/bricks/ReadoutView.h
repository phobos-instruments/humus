// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/host/BrickHost.h"
#include "gui/style/LookAndFeel.h"
#include "gui/bricks/PolledBrick.h"
#include "hum/caps/Graph.h"

namespace hum {

class ReadoutView : public PolledBrick {
public:
    static constexpr int kMaxValues = 8;

    ReadoutView(BrickHost& host, std::string name, int decimals)
        : PolledBrick(host, std::move(name), 2), decimals_(std::max(0, std::min(6, decimals))) {}

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
        const int n = std::max(1, count_);
        const int cols = n > 4 ? 2 : 1;
        const int rows = (n + cols - 1) / cols;
        const int cellW = r.getWidth() / cols;
        const int cellH = std::max(14, r.getHeight() / rows);
        for (int i = 0; i < n; ++i) {
            auto cell = juce::Rectangle<int>(r.getX() + (i / rows) * cellW,
                                             r.getY() + (i % rows) * cellH, cellW, cellH);
            const bool have = i < count_;
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(10.0f));
            g.drawText(have ? names_[i] : juce::String("-"), cell.removeFromLeft(cell.getWidth() / 2),
                       juce::Justification::centredLeft);
            g.setColour(have ? Palette::text : Palette::textDim);
            g.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
            g.drawText(have ? juce::String(values_[i], decimals_) : juce::String("-"), cell,
                       juce::Justification::centredRight);
        }
    }

private:
    void poll() override {
        ControlSource::ControlVal vals[kMaxValues];
        auto* src = live<ControlSource>();
        const int n = src ? src->controlValues(vals, kMaxValues) : 0;
        bool changed = n != count_;
        for (int i = 0; i < n && !changed; ++i)
            changed = std::abs(vals[i].value - values_[i]) > 0.5f * std::pow(10.0f, (float) -decimals_)
                   || names_[i] != juce::String(vals[i].name);
        if (!changed) return;
        count_ = n;
        for (int i = 0; i < n; ++i) {
            values_[i] = vals[i].value;
            names_[i] = juce::String(vals[i].name);
        }
        repaint();
    }

    const int decimals_;
    int count_ = 0;
    float values_[kMaxValues]{};
    juce::String names_[kMaxValues];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReadoutView)
};

}
