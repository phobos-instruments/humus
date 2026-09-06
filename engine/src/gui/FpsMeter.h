#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/AppSettings.h"
#include "gui/LookAndFeel.h"

namespace hum {

inline bool fpsOverlayOn() {
    return AppSettings::instance().getInt("video.showFps", 0) != 0;
}

inline void setFpsOverlayOn(bool on) {
    AppSettings::instance().set("video.showFps", on ? 1 : 0);
}

class FpsMeter {
public:
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
        if (fps_ <= 0.0 || !fpsOverlayOn()) return;
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.setFont(juce::FontOptions(11.0f));
        g.drawText(label(), bounds.reduced(6).removeFromTop(14),
                   juce::Justification::centredRight, false);
    }

    static juce::Rectangle<int> buttonBounds(juce::Rectangle<int> brick) {
        return brick.reduced(6).removeFromTop(15).removeFromLeft(30);
    }

    static void paintButton(juce::Graphics& g, juce::Rectangle<int> brick) {
        const auto r = buttonBounds(brick).toFloat();
        const bool on = fpsOverlayOn();
        g.setColour(juce::Colours::black.withAlpha(0.45f));
        g.fillRoundedRectangle(r, 3.0f);
        g.setColour(on ? Palette::accent : juce::Colours::white.withAlpha(0.35f));
        g.drawRoundedRectangle(r.reduced(0.5f), 3.0f, 1.0f);
        g.setFont(juce::FontOptions(9.5f));
        g.drawText("FPS", r.toNearestInt(), juce::Justification::centred, false);
    }

    static bool clickToggles(juce::Point<int> at, juce::Rectangle<int> brick) {
        if (!buttonBounds(brick).contains(at)) return false;
        setFpsOverlayOn(!fpsOverlayOn());
        return true;
    }

private:
    double fps_ = 0.0, rateAtMs_ = 0.0, freshAtMs_ = 0.0;
    unsigned rateAtGen_ = 0, freshGen_ = ~0u;
};

}
