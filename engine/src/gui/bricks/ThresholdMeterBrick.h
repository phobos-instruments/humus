// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <string>

#include "gui/style/Colours.h"
#include "gui/bricks/LevelMeterView.h"
#include "gui/common/Localisation.h"

namespace hum {

class ThresholdMeterBrick : public LevelMeterView {
public:
    ThresholdMeterBrick(BrickHost& host, std::string cn, std::string param)
        : LevelMeterView(host, cn), threshold_(host, cn, std::move(param)) {}

    std::function<void(juce::Point<int>)> onPopup;

    void reloadValues() override { repaint(); }
    int preferredContentWidth() const override { return 220; }
    int preferredContentHeight(int) const override { return 34; }

    void paint(juce::Graphics& g) override {
        LevelMeterView::paint(g);
        const auto bars = barArea();
        const int x = bars.getX() + (int) std::lround(threshold_.fill() * (float) bars.getWidth());

        auto strip = getLocalBounds().removeFromTop(kStrip);
        const bool markLeft = x < getWidth() / 2;
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(10.0f));
        g.drawText(readoutText(), strip.reduced(2, 0),
                   markLeft ? juce::Justification::centredRight
                            : juce::Justification::centredLeft);

        const bool off = threshold_.off();
        g.setColour(off ? Palette::textDim.withAlpha(alpha::dim) : ink::state::caution);
        g.fillRect((float) x - 1.0f, (float) bars.getY() - 2.0f, off ? 1.0f : 2.0f,
                   (float) bars.getHeight() + 4.0f);
        if (!off) {
            juce::Path flag;
            flag.addTriangle((float) x - 4.0f, (float) strip.getBottom() - 5.0f,
                             (float) x + 4.0f, (float) strip.getBottom() - 5.0f,
                             (float) x, (float) strip.getBottom() + 1.0f);
            g.fillPath(flag);
        }
    }

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu()) {
            if (onPopup) onPopup(e.getScreenPosition());
            return;
        }
        threshold_.begin();
        dragTo(e.x);
    }
    void mouseDrag(const juce::MouseEvent& e) override { dragTo(e.x); }
    void mouseUp(const juce::MouseEvent&) override { threshold_.end(); }

private:
    static constexpr int kStrip = 13;

    juce::Rectangle<int> meterBounds() const override {
        return getLocalBounds().withTrimmedTop(kStrip);
    }

    juce::String readoutText() const {
        if (threshold_.off()) return tr("threshold-meter.start-at-any-sound", "Start at: any sound");
        return juce::String::fromUTF8("Start at: ") + juce::String(threshold_.decibels(), 1) + " dB";
    }

    void dragTo(int x) {
        const auto bars = barArea();
        if (bars.getWidth() <= 0) return;
        if (threshold_.dragToFill((float) (x - bars.getX()) / (float) bars.getWidth())) repaint();
    }

    void poll() override {
        LevelMeterView::poll();
        if (threshold_.poll()) repaint();
    }

    readout::Threshold threshold_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThresholdMeterBrick)
};

}
