// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/params/ParamSchema.h"
#include "gui/editor/BrickBindings.h"
#include "gui/style/Colours.h"
#include "gui/host/BrickHost.h"
#include "hum/Chord.h"
#include "gui/style/LookAndFeel.h"
#include "gui/bricks/PolledBrick.h"
#include "gui/common/Localisation.h"

namespace hum {

class IntervalRowsBrick : public PolledBrick,
                          public juce::SettableTooltipClient {
public:
    static constexpr int kKeys = 13;

    IntervalRowsBrick(BrickHost& host, std::string cn, const std::string& className, const Bindings& bound)
        : PolledBrick(host, std::move(cn), 4), chordParam_(bound(bind::kChord)) {
        const auto prefix = bound(bind::kRowPrefix);
        for (const auto& d : schemaFor(className))
            if (!prefix.empty() && d.name.rfind(prefix, 0) == 0) rows_.push_back({d.name, d.name.substr(prefix.size())});
        lastSig_.assign(rows_.size() + 1, 0);
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
        const float rowH = (float) r.getHeight() / (float) std::max<size_t>(1, rows_.size());
        const float keyW = (float) (r.getWidth() - kLabelW) / kKeys;
        g.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
        for (int row = 0; row < (int) rows_.size(); ++row) {
            const float y = r.getY() + row * rowH;
            const bool overridden = overrideOf(row) >= 0;
            g.setColour(overridden ? Palette::accent : Palette::textDim);
            g.drawText(juce::String::fromUTF8(rows_[(size_t) row].label.c_str()), r.getX(), (int) y, kLabelW - 4,
                       (int) rowH, juce::Justification::centred);
            const int sel = effectiveOf(row);
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
        const bool release = overrideOf(row) == key;
        host_.editParam(name_, rows_[(size_t) row].param, release ? -1.0 : (double) key);
        repaint();
    }

private:
    static constexpr int kLabelW = 18;
    struct Row { std::string param, label; };
    static bool isBlackKey(int semi) {
        const int p = semi % 12;
        return p == 1 || p == 3 || p == 6 || p == 8 || p == 10;
    }

    int overrideOf(int row) const {
        const int v = (int) host_.liveParamValue(name_, rows_[(size_t) row].param);
        return v >= 0 && v < kKeys ? v : -1;
    }
    int effectiveOf(int row) const {
        const int ov = overrideOf(row);
        if (ov >= 0) return ov;
        const Chord chord = Chord::byId((int) host_.liveParamValue(name_, chordParam_));
        return (int) std::lround(chord.cents(row) / 100.0);
    }

    std::pair<int, int> cellAt(juce::Point<float> p) const {
        const auto r = getLocalBounds();
        const float rowH = (float) r.getHeight() / (float) std::max<size_t>(1, rows_.size());
        const float keyW = (float) (r.getWidth() - kLabelW) / kKeys;
        const int row = (int) ((p.y - r.getY()) / rowH);
        const int key = (int) ((p.x - r.getX() - kLabelW) / keyW);
        if (row < 0 || row >= (int) rows_.size() || key < 0 || key >= kKeys || p.x < r.getX() + kLabelW)
            return {-1, -1};
        return {row, key};
    }

    void poll() override {
        std::vector<int> sig(rows_.size() + 1, 0);
        for (size_t i = 0; i < rows_.size(); ++i) sig[i] = overrideOf((int) i);
        sig.back() = (int) host_.liveParamValue(name_, chordParam_);
        if (sig != lastSig_) { lastSig_ = sig; repaint(); }
    }

    std::string chordParam_;
    std::vector<Row> rows_;
    int hoverRow_ = -1, hoverKey_ = -1;
    std::vector<int> lastSig_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IntervalRowsBrick)
};

}
