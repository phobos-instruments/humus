// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/host/BrickHost.h"
#include "gui/style/LookAndFeel.h"
#include "gui/bricks/PolledBrick.h"
#include "hum/caps/Graph.h"

namespace hum {

class TextReadoutView : public PolledBrick {
public:
    static constexpr int kMaxLines = 4;

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
        const int n = std::max(1, count_);
        const int lineH = std::max(12, r.getHeight() / n);
        g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::plain));
        for (int i = 0; i < n; ++i) {
            g.setColour(i < count_ ? Palette::text : Palette::textDim);
            g.drawText(i < count_ ? lines_[i] : juce::String("-"), r.removeFromTop(lineH),
                       juce::Justification::centredLeft, true);
        }
    }

private:
    void poll() override {
        std::string fresh[kMaxLines];
        auto* src = live<TextSource>();
        const int n = src ? src->textLines(fresh, kMaxLines) : 0;
        bool changed = n != count_;
        for (int i = 0; i < n && !changed; ++i) changed = lines_[i] != juce::String(fresh[i]);
        if (!changed) return;
        count_ = n;
        for (int i = 0; i < n; ++i) lines_[i] = juce::String(fresh[i]);
        repaint();
    }

    int count_ = 0;
    juce::String lines_[kMaxLines];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextReadoutView)
};

}
