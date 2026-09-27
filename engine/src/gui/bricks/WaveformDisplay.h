// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/host/BrickHost.h"
#include "gui/editor/readouts/ScopeReadings.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

class WaveformDisplay : public juce::Component {
public:
    WaveformDisplay(BrickHost& host, std::string name, std::vector<std::string> ampParams)
        : host_(host), name_(std::move(name)), reading_(std::move(ampParams)) {}

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat().reduced(2.0f);
        g.setColour(Palette::background);
        g.fillRect(r);
        g.setColour(Palette::border);
        g.drawRect(r, 1.0f);
        g.setColour(Palette::panelLight);
        g.drawHorizontalLine((int) r.getCentreY(), r.getX(), r.getRight());

        if (reading_.empty() || r.getWidth() < 2) return;
        double peak = 0.0;
        const auto a = reading_.read(host_, name_, peak);

        juce::Path path;
        const int steps = (int) r.getWidth();
        for (int i = 0; i <= steps; ++i) {
            const double t = (double) i / steps;
            const double y = readout::Harmonics::valueAt(a, peak, t);
            const float px = r.getX() + (float) i;
            const float py = r.getCentreY() - (float) (y * r.getHeight() * 0.45);
            if (i == 0) path.startNewSubPath(px, py); else path.lineTo(px, py);
        }
        g.setColour(Palette::accent);
        g.strokePath(path, juce::PathStrokeType(1.4f));
    }

private:
    BrickHost& host_;
    std::string name_;
    readout::Harmonics reading_;
};

}
