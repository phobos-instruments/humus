// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <cstdint>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/BrickBindings.h"
#include "gui/host/BrickHost.h"
#include "gui/style/LookAndFeel.h"
#include "gui/bricks/PolledBrick.h"
#include "gui/common/Localisation.h"
#include "hum/caps/Midi.h"

namespace hum {

class StepStripBrick : public PolledBrick, public juce::SettableTooltipClient {
public:
    StepStripBrick(BrickHost& host, std::string cn, const Bindings& bound)
        : PolledBrick(host, std::move(cn)), muteParam_(bound(bind::kMute)) {
        setTooltip(tr("step-strip.click-a-step-to-silence", "Click a step to silence it; drag to sweep. Right-click for the whole strip"));
    }

    void reloadValues() override { repaint(); }
    void refreshAutomatedValues() override { repaint(); }
    int preferredContentWidth() const override { return 522; }
    int preferredContentHeight(int) const override { return 16; }

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu()) { stripMenu(); return; }
        const int k = cellAt(e.position.x);
        silencing_ = ((mask() >> k) & 1) == 0;
        setMask(silencing_ ? (mask() | (1 << k)) : (mask() & ~(1 << k)));
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        const int k = cellAt(e.position.x);
        const int want = silencing_ ? (mask() | (1 << k)) : (mask() & ~(1 << k));
        if (want != mask()) setMask(want);
    }

    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        const int steps = stepCount();
        const float cellW = r.getWidth() / (float) steps;
        const auto abs = absoluteStep();
        const int cur = abs < 0 ? -1 : (int) (abs % steps);
        const auto lap = abs < 0 ? 0 : abs - abs % steps;
        auto* strip = live<StepStrip>();
        const int mute = mask();
        for (int k = 0; k < steps; ++k) {
            auto cell = juce::Rectangle<float>(r.getX() + k * cellW, r.getY(),
                                               cellW - 2.0f, r.getHeight());
            const bool rests = abs >= 0 && strip != nullptr && strip->stripStepRests(lap + k);
            if (((mute >> k) & 1) != 0) {
                g.setColour(Palette::background.darker(0.7f));
                g.fillRoundedRectangle(cell, 2.0f);
                g.setColour(Palette::panel);
                g.drawRoundedRectangle(cell.reduced(0.5f), 2.0f, 1.0f);
                if (k == cur) {
                    g.setColour(Palette::accentDim);
                    g.drawRoundedRectangle(cell.reduced(0.5f), 2.0f, 1.0f);
                }
            } else if (k == cur) {
                g.setColour(rests ? Palette::accentDim : Palette::accent);
                g.fillRoundedRectangle(cell, 2.0f);
            } else if (rests) {
                g.setColour(Palette::background);
                g.fillRoundedRectangle(cell.reduced(0.0f, cell.getHeight() * 0.3f), 2.0f);
            } else {
                g.setColour(Palette::panelLight);
                g.fillRoundedRectangle(cell, 2.0f);
                g.setColour(Palette::border);
                g.drawRoundedRectangle(cell.reduced(0.5f), 2.0f, 1.0f);
            }
        }
    }

private:
    static constexpr int kDefaultSteps = 16;

    int stepCount() const {
        auto* strip = live<StepStrip>();
        return juce::jlimit(1, 30, strip != nullptr ? strip->stripSteps() : kDefaultSteps);
    }
    int allSteps() const { return (1 << stepCount()) - 1; }
    int cellAt(float x) const {
        const int steps = stepCount();
        return juce::jlimit(0, steps - 1, (int) (x * steps / juce::jmax(1, getWidth())));
    }
    int mask() const { return (int) host_.liveParamValue(name_, muteParam_); }
    void setMask(int m) {
        host_.editParam(name_, muteParam_, (double) (m & allSteps()));
        repaint();
    }

    void stripMenu() {
        juce::PopupMenu m;
        m.addItem(1, tr("step-strip.play-every-step", "Play every step"), mask() != 0);
        m.addItem(2, tr("step-strip.invert", "Invert"));
        m.addItem(3, tr("step-strip.silence-the-off-beats", "Silence the off-beats"));
        m.showMenuAsync(juce::PopupMenu::Options(),
                        [safe = juce::Component::SafePointer<StepStripBrick>(this)](int r) {
                            if (safe == nullptr || r == 0) return;
                            if (r == 1) safe->setMask(0);
                            if (r == 2) safe->setMask(~safe->mask());
                            if (r == 3) safe->setMask(0xAAAA);
                        });
    }

    std::int64_t absoluteStep() {
        auto* strip = live<StepStrip>();
        if (!host_.isPlaying() || strip == nullptr) return -1;
        return strip->stripStepAt(host_.positionBeats());
    }

    int restsOfLap(std::int64_t abs) {
        auto* strip = live<StepStrip>();
        if (abs < 0 || strip == nullptr) return 0;
        const int steps = stepCount();
        int rests = 0;
        for (int k = 0; k < steps; ++k)
            if (strip->stripStepRests(abs - abs % steps + k)) rests |= 1 << k;
        return rests;
    }

    void poll() override {
        const auto step = absoluteStep();
        const int rests = restsOfLap(step);
        const int mute = mask();
        if (step != lastStep_ || rests != lastRests_ || mute != lastMute_) {
            lastStep_ = step;
            lastRests_ = rests;
            lastMute_ = mute;
            repaint();
        }
    }

    std::string muteParam_;
    std::int64_t lastStep_ = -2;
    int lastRests_ = -1;
    int lastMute_ = -1;
    bool silencing_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StepStripBrick)
};

}
