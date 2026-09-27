// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <string>
#include <utility>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/bricks/PolledBrick.h"
#include "gui/common/Localisation.h"
#include "gui/editor/readouts/MeterReadings.h"
#include "gui/style/Colours.h"
#include "gui/style/StatusDot.h"

namespace hum {

class InletLamp : public PolledBrick {
public:
    static constexpr float kClipAt = 0.98f;
    static constexpr int kClipHolds = 14;
    static constexpr float kFloorWash = 0.22f;

    InletLamp(BrickHost& host, std::string organism) : PolledBrick(host, std::move(organism)) {}

    void reloadValues() override { repaint(); }
    void refreshAutomatedValues() override {}
    int preferredContentWidth() const override { return 46; }
    int preferredContentHeight(int) const override { return 22; }

    void paint(juce::Graphics& g) override {
        StatusDot lamp{tr("inlet-lamp.aud", "AUD"), tint(), false};
        lamp.paint(g, getLocalBounds().withSizeKeepingCentre(
                          juce::jmin(getWidth(), lamp.width()), StatusDot::kHeight));
    }

private:
    void poll() override {
        float lv[LevelMeter::kMax];
        const int n = host_.nodeInletMeter(name_, lv, LevelMeter::kMax);
        float loudest = 0.0f;
        for (int c = 0; c < n; ++c) loudest = std::max(loudest, lv[c]);
        if (loudest >= kClipAt) clipHeld_ = kClipHolds;
        else if (clipHeld_ > 0) --clipHeld_;
        const float fill = n > 0 ? readout::LevelBars::fillFor(loudest) : 0.0f;
        if (readout::LevelBars::moved(loudest, seen_) || (clipHeld_ > 0) != wasClipping_) {
            seen_ = loudest;
            fill_ = fill;
            wasClipping_ = clipHeld_ > 0;
            repaint();
        }
    }

    juce::Colour tint() const {
        if (wasClipping_) return ink::state::danger;
        if (fill_ <= 0.0f) return Palette::border;
        return ink::state::ok.withMultipliedAlpha(kFloorWash + (1.0f - kFloorWash) * fill_);
    }

    float seen_ = 0.0f, fill_ = 0.0f;
    int clipHeld_ = 0;
    bool wasClipping_ = false;
};

}
