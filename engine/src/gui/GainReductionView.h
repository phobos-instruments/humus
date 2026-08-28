#pragma once
#include <algorithm>
#include <cmath>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/EngineHost.h"
#include "gui/LookAndFeel.h"
#include "gui/PolledBrick.h"
#include "hum/Capabilities.h"

namespace hum {

class GainReductionView : public PolledBrick {
public:
    static constexpr float kFullScaleDb = 24.0f;

    GainReductionView(EngineHost& host, std::string name)
        : PolledBrick(host, std::move(name)) {}

    void reloadValues() override {}
    int preferredContentWidth() const override { return 28; }
    int preferredContentHeight(int) const override { return 84; }

    void poll() override {
        const float db = grDb();
        if (std::abs(db - shown_) > 0.1f) { shown_ = db; repaint(); }
    }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().reduced(1);
        g.setColour(Palette::background.darker(0.25f));
        g.fillRoundedRectangle(r.toFloat(), 3.0f);
        auto inner = r.reduced(2);
        const float frac = std::clamp(grDb() / kFullScaleDb, 0.0f, 1.0f);
        if (frac > 0.0f) {
            auto bar = inner.removeFromTop((int) std::round(frac * inner.getHeight()));
            g.setColour(juce::Colour(0xffe0a03c));
            g.fillRoundedRectangle(bar.toFloat(), 2.0f);
        }
    }

private:
    float grDb() const {
        if (auto* s = live<GainReductionSource>()) return s->grDb();
        return 0.0f;
    }
    float shown_ = -1.0f;
};

}
