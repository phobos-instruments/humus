// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/Colours.h"
#include "gui/style/Contrast.h"
#include "gui/bricks/PolledBrick.h"
#include "gui/host/BrickHost.h"
#include "hum/Organism.h"
#include "gui/style/LookAndFeel.h"
#include "gui/editor/readouts/ScopeReadings.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class FieldScopeView : public PolledBrick {
public:
    FieldScopeView(BrickHost& host, std::string name)
        : PolledBrick(host, std::move(name)) {
    }

    void reloadValues() override {}
    void refreshAutomatedValues() override {}
    int preferredContentWidth() const override { return 165; }
    int preferredContentHeight(int) const override { return 140; }

    static juce::Colour ground() {
        return contrast::isLight(Palette::panel) ? Palette::panel.darker(0.07f)
                                                 : ink::field::ground;
    }
    static juce::Colour strand() {
        return contrast::isLight(Palette::panel) ? Palette::panel.darker(0.14f)
                                                 : ink::field::strip;
    }
    static juce::Colour edge() {
        return contrast::isLight(Palette::panel) ? Palette::panel.darker(0.22f)
                                                 : ink::field::edge;
    }

    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        g.setColour(ground());
        g.fillRoundedRectangle(r, 4.0f);
        g.setColour(edge());
        g.drawRoundedRectangle(r.reduced(0.5f), 4.0f, 1.0f);

        const float cx = r.getCentreX();
        const float floorY = r.getBottom() - 10.0f;
        g.setColour(Palette::text.withAlpha(alpha::mist));
        g.drawLine(cx, r.getY() + 4.0f, cx, floorY, 1.0f);
        g.drawLine(cx, floorY, r.getX() + 6.0f, r.getY() + 8.0f, 1.0f);
        g.drawLine(cx, floorY, r.getRight() - 6.0f, r.getY() + 8.0f, 1.0f);
        g.setColour(Palette::textDim.withAlpha(alpha::mid));
        g.setFont(juce::FontOptions(8.0f));
        g.drawText("L", (int) r.getX() + 5, (int) r.getY() + 4, 10, 10, juce::Justification::left);
        g.drawText("R", (int) r.getRight() - 15, (int) r.getY() + 4, 10, 10, juce::Justification::right);

        const bool haveCloud_ = reading_.hasCloud();
        const float cloudFade_ = reading_.fade(), corr_ = reading_.correlation();
        const int pairs_ = reading_.pairs();
        if (haveCloud_) {
            const float span = r.getWidth() * 0.46f;
            for (int i = 0; i < pairs_; ++i) {
                const float side = reading_.side(i);
                const float mid = reading_.mid(i);
                const float px = cx + juce::jlimit(-1.2f, 1.2f, side) * span;
                const float py = floorY - juce::jlimit(-0.1f, 1.3f, std::abs(mid)) * (floorY - r.getY() - 8.0f) * 0.8f;
                const float t = (float) i / (float) juce::jmax(1, pairs_ - 1);
                const float a = (0.10f + 0.65f * t * t) * cloudFade_;
                if (a < 0.02f) continue;
                g.setColour(Palette::accent.withAlpha(a));
                const float rad = t > 0.9f ? 1.6f : 1.1f;
                g.fillEllipse(px - rad, py - rad, rad * 2.0f, rad * 2.0f);
            }
        }

        const juce::Rectangle<float> strip(r.getX() + 2.0f, r.getBottom() - 8.0f,
                                           r.getWidth() - 4.0f, 6.0f);
        g.setColour(strand());
        g.fillRect(strip);
        g.setColour(Palette::text.withAlpha(alpha::heavy));
        g.fillRect(cx - 0.5f, strip.getY(), 1.0f, strip.getHeight());
        if (haveCloud_ && std::abs(corr_) > 0.01f) {
            const float w = std::abs(corr_) * (strip.getWidth() * 0.5f - 2.0f) * cloudFade_;
            if (corr_ >= 0.0f) {
                g.setColour(Palette::accent.withAlpha(0.8f * cloudFade_));
                g.fillRect(cx + 0.5f, strip.getY() + 1.0f, w, strip.getHeight() - 2.0f);
            } else {
                g.setColour(Palette::textDim.withAlpha(0.8f * cloudFade_));
                g.fillRect(cx - 0.5f - w, strip.getY() + 1.0f, w, strip.getHeight() - 2.0f);
            }
        }
    }

private:
    void poll() override {
        if (reading_.poll(host_, name_)) repaint();
    }

    readout::StereoField reading_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FieldScopeView)
};

}
