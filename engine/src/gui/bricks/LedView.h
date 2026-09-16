// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/bricks/PolledBrick.h"
#include "gui/host/BrickHost.h"
#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"
#include "hum/caps/Graph.h"

namespace hum {

class LedView : public PolledBrick {
public:
    LedView(BrickHost& host, std::string name, std::string shows)
        : PolledBrick(host, std::move(name)), shows_(std::move(shows)) {}

    int preferredContentWidth() const override { return 24; }
    int preferredContentHeight(int) const override { return 24; }

    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        const float side = std::min(r.getWidth(), r.getHeight());
        const auto face = juce::Rectangle<float>(side, side).withCentre(r.getCentre()).reduced(1.5f);
        g.setColour(Palette::background.darker(0.35f));
        g.fillEllipse(face);
        g.setColour(ink::state::controlCord.withAlpha(0.12f + 0.88f * glow_));
        g.fillEllipse(face.reduced(2.5f));
        g.setColour(Palette::border);
        g.drawEllipse(face, 1.0f);
    }

private:
    void poll() override {
        const float now = shown();
        const bool struck = settled_ && now != last_;
        settled_ = true;
        last_ = now;
        const float was = glow_;
        glow_ = struck ? 1.0f : glow_ * 0.7f;
        if (glow_ < 0.01f) glow_ = 0.0f;
        if (std::abs(glow_ - was) > 0.004f) repaint();
    }

    float shown() const {
        ControlSource::ControlVal vals[8];
        auto* src = live<ControlSource>();
        const int n = src ? src->controlValues(vals, 8) : 0;
        for (int i = 0; i < n; ++i)
            if (shows_.empty() || shows_ == vals[i].name) return vals[i].value;
        return 0.0f;
    }

    const std::string shows_;
    float last_ = 0.0f;
    float glow_ = 0.0f;
    bool settled_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LedView)
};

}
