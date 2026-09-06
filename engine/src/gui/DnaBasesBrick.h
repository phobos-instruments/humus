#pragma once
#include <cmath>
#include <array>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/EngineHost.h"
#include "hum/Chord.h"
#include "gui/LookAndFeel.h"
#include "gui/PolledBrick.h"
#include "gui/Localisation.h"

namespace hum {

class DnaBasesBrick : public PolledBrick,
                      public juce::SettableTooltipClient {
public:
    static constexpr int kRows = 4, kKeys = 13;

    DnaBasesBrick(EngineHost& host, std::string cn) : PolledBrick(host, std::move(cn), 4) {
        setTooltip(juce::String::fromUTF8(
            "Each base's interval over Root - click a key to override the "
            "Chord preset for that base; click the lit key again to follow the "
            "preset once more"));
    }

    void reloadValues() override { repaint(); }
    void refreshAutomatedValues() override { repaint(); }
    int preferredContentWidth() const override { return 520; }
    int preferredContentHeight(int) const override { return 88; }

    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds();
        const float rowH = (float) r.getHeight() / kRows;
        const float keyW = (float) (r.getWidth() - kLabelW) / kKeys;
        g.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
        for (int row = 0; row < kRows; ++row) {
            const float y = r.getY() + row * rowH;
            const bool overridden = overrideOf(row) >= 0;
            g.setColour(overridden ? Palette::accent : Palette::textDim);
            g.drawText(juce::String(kLetters[row]), r.getX(), (int) y, kLabelW - 4,
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
                    g.setColour(black ? Palette::textDim.withAlpha(0.6f) : Palette::textDim);
                }
                if (row == hoverRow_ && k == hoverKey_ && k != sel) {
                    g.setColour(Palette::accent.withAlpha(0.35f));
                    g.drawRoundedRectangle(cell.reduced(0.5f), 2.0f, 1.2f);
                    g.setColour(black ? Palette::textDim.withAlpha(0.6f) : Palette::textDim);
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
        host_.editParam(name_, kParams[row], release ? -1.0 : (double) key);
        repaint();
    }

private:
    static constexpr int kLabelW = 18;
    static constexpr const char* kParams[kRows] = {"Base A", "Base C", "Base G", "Base T"};
    static constexpr const char* kLetters[kRows] = {"A", "C", "G", "T"};
    static bool isBlackKey(int semi) {
        const int p = semi % 12;
        return p == 1 || p == 3 || p == 6 || p == 8 || p == 10;
    }

    int overrideOf(int row) const {
        const int v = (int) host_.liveParamValue(name_, kParams[row]);
        return v >= 0 && v < kKeys ? v : -1;
    }
    int effectiveOf(int row) const {
        const int ov = overrideOf(row);
        if (ov >= 0) return ov;
        const Chord chord = Chord::byId((int) host_.liveParamValue(name_, "Chord"));
        return (int) std::lround(chord.cents(row) / 100.0);
    }

    std::pair<int, int> cellAt(juce::Point<float> p) const {
        const auto r = getLocalBounds();
        const float rowH = (float) r.getHeight() / kRows;
        const float keyW = (float) (r.getWidth() - kLabelW) / kKeys;
        const int row = (int) ((p.y - r.getY()) / rowH);
        const int key = (int) ((p.x - r.getX() - kLabelW) / keyW);
        if (row < 0 || row >= kRows || key < 0 || key >= kKeys || p.x < r.getX() + kLabelW)
            return {-1, -1};
        return {row, key};
    }

    void poll() override {
        std::array<int, kRows + 1> sig{};
        for (int i = 0; i < kRows; ++i) sig[(size_t) i] = overrideOf(i);
        sig[kRows] = (int) host_.liveParamValue(name_, "Chord");
        if (sig != lastSig_) { lastSig_ = sig; repaint(); }
    }

    int hoverRow_ = -1, hoverKey_ = -1;
    std::array<int, kRows + 1> lastSig_{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DnaBasesBrick)
};

}
