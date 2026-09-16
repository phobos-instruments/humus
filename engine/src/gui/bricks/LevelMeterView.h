// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/Colours.h"
#include "gui/bricks/PolledBrick.h"
#include "gui/host/BrickHost.h"
#include "gui/style/LookAndFeel.h"
#include "hum/dsp/LevelMeter.h"

namespace hum {

class LevelMeterView : public PolledBrick {
public:
    LevelMeterView(BrickHost& host, std::string name)
        : PolledBrick(host, std::move(name)) {
    }

    void reloadValues() override {}
    void refreshAutomatedValues() override {}
    int preferredContentWidth() const override { return 124; }
    int preferredContentHeight(int) const override { return 22; }

    void paint(juce::Graphics& g) override {
        auto r = meterBounds();
        const int n = std::max(1, (int) levels_.size());
        g.setColour(Palette::background.darker(0.25f));
        g.fillRoundedRectangle(r.toFloat(), 3.0f);

        auto inner = r.reduced(2);
        const int gap = n > 1 ? 2 : 0;
        const int rowH = std::max(3, (inner.getHeight() - (n - 1) * gap) / n);
        const bool tags = rowH >= 11 && !levels_.empty();
        for (int c = 0; c < n; ++c) {
            juce::Rectangle<int> row(inner.getX(), inner.getY() + c * (rowH + gap),
                                     inner.getWidth(), rowH);
            if (tags) {
                auto tagArea = row.removeFromLeft(12);
                g.setColour(Palette::textDim);
                g.setFont(juce::FontOptions(9.0f));
                g.drawText(levels_.size() == 2 ? (c == 0 ? "L" : "R") : juce::String(c + 1),
                           tagArea, juce::Justification::centred);
            }
            drawBar(g, row, c < (int) levels_.size() ? std::max(0.0f, levels_[(size_t) c]) : 0.0f);
        }
    }

    static float fillFor(float level) {
        const float db = 20.0f * std::log10(std::max(level, 1.0e-5f));
        return juce::jlimit(0.0f, 1.0f, (db + 48.0f) / 48.0f);
    }
    static bool moved(float a, float b) { return std::abs(fillFor(a) - fillFor(b)) > 0.02f; }
    static int litCells(float level, int width) {
        const int cells = juce::jlimit(8, 24, width / 6);
        return (int) std::lround(fillFor(level) * cells);
    }

protected:
    static float levelForFill(float fill) {
        return std::pow(10.0f, (juce::jlimit(0.0f, 1.0f, fill) * 48.0f - 48.0f) / 20.0f);
    }

    virtual juce::Rectangle<int> meterBounds() const { return getLocalBounds(); }

    juce::Rectangle<int> barArea() const {
        auto inner = meterBounds().reduced(2);
        const int n = std::max(1, (int) levels_.size());
        const int gap = n > 1 ? 2 : 0;
        const int rowH = std::max(3, (inner.getHeight() - (n - 1) * gap) / n);
        if (rowH >= 11 && !levels_.empty()) inner.removeFromLeft(12);
        return inner;
    }

    void poll() override {
        float lv[LevelMeter::kMax];
        const int n = host_.nodeMeter(name_, lv, LevelMeter::kMax);
        bool dirty = (int) levels_.size() != n;
        levels_.resize((size_t) n);
        for (int c = 0; c < n; ++c) {
            if (moved(lv[c], levels_[(size_t) c])) dirty = true;
            levels_[(size_t) c] = lv[c];
        }
        if (dirty) repaint();
    }

    std::vector<float> levels_;

private:
    static void drawBar(juce::Graphics& g, juce::Rectangle<int> r, float level) {
        const int cells = juce::jlimit(8, 24, r.getWidth() / 6);
        const int lit = litCells(level, r.getWidth());
        const float cellW = (float) r.getWidth() / cells;
        for (int i = 0; i < cells; ++i) {
            const float frac = (float) i / (cells - 1);
            juce::Colour on = frac < 0.6f ? ink::state::ok
                            : frac < 0.85f ? ink::state::caution
                                           : ink::state::danger;
            juce::Rectangle<float> cell(r.getX() + i * cellW + 0.5f, (float) r.getY(),
                                        cellW - 1.0f, (float) r.getHeight());
            g.setColour(i < lit ? on : on.withAlpha(alpha::mist));
            g.fillRect(cell);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeterView)
};

}
