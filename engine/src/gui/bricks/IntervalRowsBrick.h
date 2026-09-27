// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/juce/JuceGeometry.h"
#include "gui/editor/inputs/StripInputs.h"
#include "gui/editor/BrickBindings.h"
#include "gui/style/Colours.h"
#include "gui/host/BrickHost.h"
#include "gui/style/LookAndFeel.h"
#include "gui/bricks/PolledBrick.h"
#include "gui/common/Localisation.h"
#include "core/midi/MidiFormat.h"

namespace hum {

class IntervalRowsBrick : public PolledBrick,
                          public juce::SettableTooltipClient {
public:
    static constexpr int kKeys = input::IntervalRowsModel::kKeys;

    IntervalRowsBrick(BrickHost& host, std::string cn, const std::string& className, const Bindings& bound)
        : PolledBrick(host, cn, 4), grid_(host, cn, className, bound(bind::kRowPrefix), bound(bind::kChord)) {
        setTooltip(tr("interval-rows.tooltip",
                      "Each row's interval over the root - click a key to override the chord "
                      "for that row; click the lit key again to follow the chord"));
    }

    void reloadValues() override { repaint(); }
    void refreshAutomatedValues() override { repaint(); }
    int preferredContentWidth() const override { return 520; }
    int preferredContentHeight(int) const override { return 88; }

    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds();
        const auto& rows = grid_.rows();
        const float rowH = (float) r.getHeight() / (float) std::max<size_t>(1, rows.size());
        const float keyW = (float) (r.getWidth() - kLabelW) / kKeys;
        g.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
        for (int row = 0; row < (int) rows.size(); ++row) {
            const float y = r.getY() + row * rowH;
            const bool overridden = grid_.overrideOf(row) >= 0;
            g.setColour(overridden ? Palette::accent : Palette::textDim);
            g.drawText(juce::String::fromUTF8(rows[(size_t) row].label.c_str()), r.getX(), (int) y, kLabelW - 4,
                       (int) rowH, juce::Justification::centred);
            const int sel = grid_.effectiveOf(row);
            for (int k = 0; k < kKeys; ++k) {
                juce::Rectangle<float> cell(r.getX() + kLabelW + k * keyW, y + 1.0f,
                                            keyW - 1.0f, rowH - 2.0f);
                const bool black = isBlackKey(k);
                if (k == sel) {
                    g.setColour(overridden ? Palette::accent : Palette::accentDim);
                    g.fillRoundedRectangle(cell, 2.0f);
                    g.setColour(Palette::background);
                } else {
                    g.setColour(black ? Palette::background : Palette::panelLight);
                    g.fillRoundedRectangle(cell, 2.0f);
                    g.setColour(Palette::border);
                    g.drawRoundedRectangle(cell.reduced(0.5f), 2.0f, 1.0f);
                    g.setColour(black ? Palette::textDim.withAlpha(alpha::mid) : Palette::textDim);
                }
                if (row == hoverRow_ && k == hoverKey_ && k != sel) {
                    g.setColour(Palette::accent.withAlpha(alpha::muted));
                    g.drawRoundedRectangle(cell.reduced(0.5f), 2.0f, 1.2f);
                    g.setColour(black ? Palette::textDim.withAlpha(alpha::mid) : Palette::textDim);
                }
                g.setFont(juce::FontOptions(9.0f));
                g.drawText(juce::String(k), cell.toNearestInt(), juce::Justification::centred);
                g.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
            }
        }
    }

    void mouseMove(const juce::MouseEvent& e) override {
        const auto [row, key] = cellAt(e.position);
        if (row != hoverRow_ || key != hoverKey_) { hoverRow_ = row; hoverKey_ = key; repaint(); }
    }
    void mouseExit(const juce::MouseEvent&) override { hoverRow_ = hoverKey_ = -1; repaint(); }

    void mouseDown(const juce::MouseEvent& e) override {
        const auto [row, key] = cellAt(e.position);
        if (row < 0 || key < 0) return;
        grid_.press(row, key);
        repaint();
    }

private:
    static constexpr int kLabelW = input::IntervalRowsModel::kLabelW;

    std::pair<int, int> cellAt(juce::Point<float> p) const {
        const auto r = getLocalBounds();
        return grid_.cellAt(p.x, p.y, rectOf(r));
    }

    void poll() override {
        if (grid_.poll()) repaint();
    }

    input::IntervalRowsModel grid_;
    int hoverRow_ = -1, hoverKey_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IntervalRowsBrick)
};

}
