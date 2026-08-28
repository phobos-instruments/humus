#pragma once
#include <cmath>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/EngineHost.h"
#include "gui/LookAndFeel.h"
#include "gui/PolledBrick.h"
#include "hum/Capabilities.h"
#include "core/Categories.h"
#include "hum/dsp/Lfo.h"

namespace hum {

class LfoScopeView : public PolledBrick {
public:
    LfoScopeView(EngineHost& host, std::string name)
        : PolledBrick(host, std::move(name)) {}

    int preferredContentWidth() const override { return 277; }
    int preferredContentHeight(int) const override { return 56; }

    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(Palette::background);
        g.fillRoundedRectangle(r, 4.0f);

        const int wave = (int) juce::jlimit(0.0, 5.0, host_.liveParamValue(name_, "Waveform"));
        const double amp = juce::jlimit(0.0, 1.0, host_.liveParamValue(name_, "Amplitude"));
        const double off = juce::jlimit(-1.0, 1.0, host_.liveParamValue(name_, "Offset"));

        auto toY = [&r](double v) {
            return r.getCentreY() - (float) juce::jlimit(-1.25, 1.25, v) * r.getHeight() * 0.4f;
        };

        g.setColour(Palette::border.withAlpha(0.5f));
        g.drawHorizontalLine((int) toY(0.0), r.getX() + 2.0f, r.getRight() - 2.0f);

        juce::Path p;
        const int steps = juce::jmax(2, (int) r.getWidth());
        for (int i = 0; i <= steps; ++i) {
            const double ph = (double) i / steps;
            const double v = wave == 5 ? (double) held_ * 2.0 - 1.0 : shapeAt(wave, ph);
            const float x = r.getX() + (float) i / steps * r.getWidth();
            const float y = toY(off + amp * v);
            if (i == 0) p.startNewSubPath(x, y); else p.lineTo(x, y);
        }
        const auto ink = familyColour();
        g.setColour(ink.withAlpha(0.85f));
        g.strokePath(p, juce::PathStrokeType(1.6f));

        const float x = r.getX() + phase_ * r.getWidth();
        g.setColour(Palette::textDim.withAlpha(0.45f));
        g.drawVerticalLine((int) x, r.getY() + 2.0f, r.getBottom() - 2.0f);
        const double v = wave == 5 ? (double) held_ * 2.0 - 1.0 : shapeAt(wave, phase_);
        g.setColour(ink);
        g.fillEllipse(x - 2.5f, toY(off + amp * v) - 2.5f, 5.0f, 5.0f);
    }

protected:
    void poll() override {
        float ph = phase_, held = held_;
        if (auto* cs = live<ControlSource>()) {
            ControlSource::ControlVal vals[4];
            const int n = cs->controlValues(vals, 4);
            for (int i = 0; i < n; ++i) {
                if (juce::String(vals[i].name) == "phase") ph = vals[i].value;
                if (juce::String(vals[i].name) == "wave") held = vals[i].value;
            }
        }
        const int w = juce::jmax(1, getWidth());
        if ((int) (ph * w) == (int) (phase_ * w) && std::abs(held - held_) < 0.004f) return;
        phase_ = ph;
        held_ = held;
        repaint();
    }

private:
    juce::Colour familyColour() const {
        const auto* cm = host_.model().byName(name_);
        return Palette::familyAccent(cm ? familyOf(cm->displayClass) : Family::Motion);
    }

    static double shapeAt(int wave, double p) {
        return wave == 1 ? Lfo::triangle(p)
             : wave == 2 ? Lfo::square(p)
             : wave == 3 ? Lfo::sawUp(p)
             : wave == 4 ? Lfo::sawDown(p)
                         : Lfo::sine(p);
    }

    float phase_ = 0.0f, held_ = 0.5f;
};

}
