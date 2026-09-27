// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/Colours.h"
#include "gui/host/BrickHost.h"
#include "gui/style/LookAndFeel.h"
#include "gui/bricks/PolledBrick.h"
#include "gui/editor/readouts/ControlReadings.h"

namespace hum {

class GainReductionView : public PolledBrick {
public:
    GainReductionView(BrickHost& host, std::string name)
        : PolledBrick(host, std::move(name)) {}

    void reloadValues() override {}
    int preferredContentWidth() const override { return 28; }
    int preferredContentHeight(int) const override { return 84; }

    void poll() override {
        if (reading_.poll(host_, name_)) repaint();
    }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().reduced(1);
        g.setColour(Palette::background.darker(0.25f));
        g.fillRoundedRectangle(r.toFloat(), 3.0f);
        auto inner = r.reduced(2);
        const float frac = readout::GainReduction::fill(readout::GainReduction::reductionDb(host_, name_));
        if (frac > 0.0f) {
            auto bar = inner.removeFromTop((int) std::round(frac * inner.getHeight()));
            g.setColour(ink::state::warning);
            g.fillRoundedRectangle(bar.toFloat(), 2.0f);
        }
    }

private:
    readout::GainReduction reading_;
};

}
