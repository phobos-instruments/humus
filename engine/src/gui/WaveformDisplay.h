#pragma once
#include <cmath>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/EngineHost.h"
#include "gui/LookAndFeel.h"

namespace hum {

class WaveformDisplay : public juce::Component {
public:
    WaveformDisplay(EngineHost& host, std::string name, std::vector<std::string> ampParams)
        : host_(host), name_(std::move(name)), amps_(std::move(ampParams)) {}

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat().reduced(2.0f);
        g.setColour(Palette::background);
        g.fillRect(r);
        g.setColour(Palette::border);
        g.drawRect(r, 1.0f);
        g.setColour(Palette::panelLight);
        g.drawHorizontalLine((int) r.getCentreY(), r.getX(), r.getRight());

        const int N = (int) amps_.size();
        if (N == 0 || r.getWidth() < 2) return;
        std::vector<double> a((size_t) N);
        double peak = 0.0;
        for (int k = 0; k < N; ++k) { a[(size_t) k] = host_.liveParamValue(name_, amps_[(size_t) k]); peak += std::abs(a[(size_t) k]); }
        if (peak <= 1e-9) peak = 1.0;

        juce::Path path;
        const int steps = (int) r.getWidth();
        for (int i = 0; i <= steps; ++i) {
            const double t = (double) i / steps;
            double y = 0.0;
            for (int k = 0; k < N; ++k)
                y += a[(size_t) k] * std::sin(2.0 * juce::MathConstants<double>::pi * (k + 1) * t);
            y /= peak;
            const float px = r.getX() + (float) i;
            const float py = r.getCentreY() - (float) (y * r.getHeight() * 0.45);
            if (i == 0) path.startNewSubPath(px, py); else path.lineTo(px, py);
        }
        g.setColour(Palette::accent);
        g.strokePath(path, juce::PathStrokeType(1.4f));
    }

private:
    EngineHost& host_;
    std::string name_;
    std::vector<std::string> amps_;
};

}
