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
        : LevelMeterView(host, std::move(cn)), param_(std::move(param)) {
        last_ = (float) host_.liveParamValue(name_, param_);
    }

    std::function<void(juce::Point<int>)> onPopup;

    void reloadValues() override { repaint(); }
    int preferredContentWidth() const override { return 220; }
    int preferredContentHeight(int) const override { return 34; }

    void paint(juce::Graphics& g) override {
        LevelMeterView::paint(g);
        const float t = (float) host_.liveParamValue(name_, param_);
        const auto bars = barArea();
        const int x = bars.getX() + (int) std::lround(fillFor(t) * (float) bars.getWidth());

        auto strip = getLocalBounds().removeFromTop(kStrip);
        const bool markLeft = x < getWidth() / 2;
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(10.0f));
        g.drawText(readout(t), strip.reduced(2, 0),
                   markLeft ? juce::Justification::centredRight
                            : juce::Justification::centredLeft);

        const bool off = t <= 0.0f;
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
        host_.beginParamDrag(name_, param_);
        dragTo(e.x);
    }
    void mouseDrag(const juce::MouseEvent& e) override { dragTo(e.x); }
    void mouseUp(const juce::MouseEvent&) override { host_.endParamDrag(); }

private:
    static constexpr int kStrip = 13;

    juce::Rectangle<int> meterBounds() const override {
        return getLocalBounds().withTrimmedTop(kStrip);
    }

    static juce::String readout(float t) {
        if (t <= 0.0f) return tr("threshold-meter.start-at-any-sound", "Start at: any sound");
        return juce::String::fromUTF8("Start at: ")
             + juce::String(20.0f * std::log10(t), 1) + " dB";
    }

    void dragTo(int x) {
        const auto bars = barArea();
        if (bars.getWidth() <= 0) return;
        const float fill = (float) (x - bars.getX()) / (float) bars.getWidth();
        const float t = fill <= 0.03f ? 0.0f : juce::jlimit(0.0f, 1.0f, levelForFill(fill));
        if (std::abs(t - (float) host_.liveParamValue(name_, param_)) < 1.0e-6f) return;
        host_.editParam(name_, param_, (double) t);
        repaint();
    }

    void poll() override {
        LevelMeterView::poll();
        const float t = (float) host_.liveParamValue(name_, param_);
        if (std::abs(t - last_) > 1.0e-6f) { last_ = t; repaint(); }
    }

    std::string param_;
    float last_ = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThresholdMeterBrick)
};

}
