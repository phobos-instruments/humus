// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/BrickBindings.h"
#include "gui/style/Colours.h"
#include "gui/host/BrickHost.h"
#include "io/PatchDocument.h"
#include "gui/style/LookAndFeel.h"
#include "gui/bricks/PolledBrick.h"
#include "gui/editor/readouts/ScopeReadings.h"
#include "core/packs/Categories.h"

namespace hum {

class LfoScopeView : public PolledBrick {
public:
    LfoScopeView(BrickHost& host, std::string name, const Bindings& bound)
        : PolledBrick(host, std::move(name)),
          reading_(bound(bind::kWaveform), bound(bind::kAmplitude), bound(bind::kOffset)) {}

    int preferredContentWidth() const override { return 277; }
    int preferredContentHeight(int) const override { return 56; }

    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(Palette::background);
        g.fillRoundedRectangle(r, 4.0f);

        const auto shape = reading_.shape(host_, name_);

        auto toY = [&r](double v) {
            return r.getCentreY() - (float) juce::jlimit(-1.25, 1.25, v) * r.getHeight() * 0.4f;
        };

        g.setColour(Palette::border.withAlpha(alpha::dim));
        g.drawHorizontalLine((int) toY(0.0), r.getX() + 2.0f, r.getRight() - 2.0f);

        juce::Path p;
        const int steps = juce::jmax(2, (int) r.getWidth());
        for (int i = 0; i <= steps; ++i) {
            const double ph = (double) i / steps;
            const float x = r.getX() + (float) i / steps * r.getWidth();
            const float y = toY(reading_.valueAt(shape, ph));
            if (i == 0) p.startNewSubPath(x, y); else p.lineTo(x, y);
        }
        const auto ink = familyColour();
        g.setColour(ink.withAlpha(alpha::heavy));
        g.strokePath(p, juce::PathStrokeType(1.6f));

        const float x = r.getX() + reading_.phase() * r.getWidth();
        g.setColour(Palette::textDim.withAlpha(alpha::dim));
        g.drawVerticalLine((int) x, r.getY() + 2.0f, r.getBottom() - 2.0f);
        g.setColour(ink);
        g.fillEllipse(x - 2.5f, toY(reading_.valueAt(shape, reading_.phase())) - 2.5f, 5.0f, 5.0f);
    }

protected:
    void poll() override {
        if (reading_.poll(host_, name_, getWidth())) repaint();
    }

private:
    readout::LfoCycle reading_;

    juce::Colour familyColour() const {
        const auto* cm = host_.model().byName(name_);
        return Palette::familyAccent(cm ? familyOf(cm->displayClass) : Family::Motion);
    }
};

}
