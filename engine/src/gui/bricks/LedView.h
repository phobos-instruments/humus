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
#include "gui/editor/readouts/ControlReadings.h"

namespace hum {

class LedView : public PolledBrick {
public:
    LedView(BrickHost& host, std::string name, std::string shows)
        : PolledBrick(host, std::move(name)), reading_(std::move(shows)) {}

    int preferredContentWidth() const override { return 24; }
    int preferredContentHeight(int) const override { return 24; }

    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        const float side = std::min(r.getWidth(), r.getHeight());
        const auto face = juce::Rectangle<float>(side, side).withCentre(r.getCentre()).reduced(1.5f);
        g.setColour(Palette::background.darker(0.35f));
        g.fillEllipse(face);
        g.setColour(ink::state::controlCord.withAlpha(0.12f + 0.88f * reading_.glow()));
        g.fillEllipse(face.reduced(2.5f));
        g.setColour(Palette::border);
        g.drawEllipse(face, 1.0f);
    }

private:
    void poll() override {
        if (reading_.poll(host_, name_)) repaint();
    }

    readout::Led reading_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LedView)
};

}
