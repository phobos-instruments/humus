// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <utility>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/app/AppSettings.h"
#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"
#include "gui/common/Localisation.h"

namespace hum {

inline juce::String fpsOverlayKey(const std::string& node) {
    return "video.showFps." + juce::String(node);
}

inline bool fpsOverlayOn(const std::string& node) {
    return AppSettings::instance().getInt(fpsOverlayKey(node), 0) != 0;
}

inline void setFpsOverlayOn(const std::string& node, bool on) {
    AppSettings::instance().set(fpsOverlayKey(node), on ? 1 : 0);
}

class FpsMeter {
public:
    void watches(std::string node) { node_ = std::move(node); }

    void note(unsigned gen) {
        const double now = juce::Time::getMillisecondCounterHiRes();
        if (gen != freshGen_ || freshAtMs_ <= 0.0) {
            freshGen_ = gen;
            freshAtMs_ = now;
        }
        if (rateAtMs_ <= 0.0) {
            rateAtMs_ = now;
            rateAtGen_ = gen;
            return;
        }
        const double span = now - rateAtMs_;
        if (span < 500.0) return;
        const double rate = (double) (gen - rateAtGen_) * 1000.0 / span;
        fps_ = fps_ > 0.0 && rate > 0.0 ? fps_ + 0.5 * (rate - fps_) : rate;
        rateAtMs_ = now;
        rateAtGen_ = gen;
    }

    void reset() {
        fps_ = 0.0;
        rateAtMs_ = 0.0;
        freshAtMs_ = 0.0;
    }

    void keepFresh() { freshAtMs_ = juce::Time::getMillisecondCounterHiRes(); }

    bool stalled() const {
        return freshAtMs_ > 0.0
               && juce::Time::getMillisecondCounterHiRes() - freshAtMs_ > 2000.0;
    }

    double fps() const { return fps_; }

    juce::String label() const {
        return juce::String(fps_, fps_ < 10.0 ? 1 : 0) + " fps";
    }

    void paint(juce::Graphics& g, juce::Rectangle<int> bounds) const {
        if (fps_ <= 0.0 || !fpsOverlayOn(node_)) return;
        g.setColour(juce::Colours::white.withAlpha(alpha::mid));
        g.setFont(juce::FontOptions(11.0f));
        g.drawText(label(), bounds.reduced(6).removeFromTop(14),
                   juce::Justification::centredRight, false);
    }

    static juce::Rectangle<int> buttonBounds(juce::Rectangle<int> brick) {
        return brick.reduced(6).removeFromTop(15).removeFromLeft(30);
    }

    void paintButton(juce::Graphics& g, juce::Rectangle<int> brick) const {
        const auto r = buttonBounds(brick).toFloat();
        const bool on = fpsOverlayOn(node_);
        g.setColour(juce::Colours::black.withAlpha(alpha::dim));
        g.fillRoundedRectangle(r, 3.0f);
        g.setColour(on ? Palette::accent : juce::Colours::white.withAlpha(alpha::muted));
        g.drawRoundedRectangle(r.reduced(0.5f), 3.0f, 1.0f);
        g.setFont(juce::FontOptions(9.5f));
        g.drawText(tr("fps-meter.fps", "FPS"), r.toNearestInt(), juce::Justification::centred, false);
    }

    bool clickToggles(juce::Point<int> at, juce::Rectangle<int> brick) const {
        if (!buttonBounds(brick).contains(at)) return false;
        setFpsOverlayOn(node_, !fpsOverlayOn(node_));
        return true;
    }

private:
    std::string node_;
    double fps_ = 0.0, rateAtMs_ = 0.0, freshAtMs_ = 0.0;
    unsigned rateAtGen_ = 0, freshGen_ = ~0u;
};

}
