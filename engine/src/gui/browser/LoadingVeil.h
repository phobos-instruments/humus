// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/common/Localisation.h"
#include "gui/style/Colours.h"

namespace hum::browser {

class LoadingVeil : public juce::Component {
public:
    static constexpr float kWheel = 30.0f, kStroke = 3.0f, kTurnMs = 900.0f, kSweep = 0.3f;

    LoadingVeil() {
        setInterceptsMouseClicks(false, false);
        setComponentID("browser-loading");
    }

    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        const juce::Point<float> centre(r.getCentreX(), r.getCentreY() - kWheel * 0.5f);
        const float radius = kWheel * 0.5f;
        const float turn = (float) std::fmod(juce::Time::getMillisecondCounterHiRes() / kTurnMs, 1.0);
        const float from = turn * juce::MathConstants<float>::twoPi;
        g.setColour(Palette::border);
        g.drawEllipse(juce::Rectangle<float>(kWheel, kWheel).withCentre(centre), kStroke);
        juce::Path arc;
        arc.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, from,
                          from + kSweep * juce::MathConstants<float>::twoPi, true);
        g.setColour(Palette::accent);
        g.strokePath(arc, juce::PathStrokeType(kStroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(13.0f));
        g.drawText(tr("browser.loading", "Loading..."),
                   juce::Rectangle<float>(r.getX(), centre.y + radius + 10.0f, r.getWidth(), 18.0f),
                   juce::Justification::centred, false);
    }
};

}
