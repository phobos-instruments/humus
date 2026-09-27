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
#include "gui/editor/inputs/StripInputs.h"

namespace hum {

class StepStripBrick : public PolledBrick, public juce::SettableTooltipClient {
public:
    StepStripBrick(BrickHost& host, std::string cn, const Bindings& bound)
        : PolledBrick(host, cn), strip_(host, cn, bound(bind::kMute)) {
        setTooltip(tr("step-strip.click-a-step-to-silence", "Click a step to silence it; drag to sweep. Right-click for the whole strip"));
    }

    void reloadValues() override { repaint(); }
    void refreshAutomatedValues() override { repaint(); }
    int preferredContentWidth() const override { return 522; }
    int preferredContentHeight(int) const override { return 16; }

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu()) { stripMenu(); return; }
        strip_.press(strip_.cellAt(e.position.x, getWidth()));
        repaint();
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        if (strip_.sweep(strip_.cellAt(e.position.x, getWidth()))) repaint();
    }

    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        const int steps = strip_.steps();
        const float cellW = r.getWidth() / (float) steps;
        const auto abs = strip_.absoluteStep();
        const int cur = abs < 0 ? -1 : (int) (abs % steps);
        const auto lap = abs < 0 ? 0 : abs - abs % steps;
        const int mute = strip_.mask();
        for (int k = 0; k < steps; ++k) {
            auto cell = juce::Rectangle<float>(r.getX() + k * cellW, r.getY(),
                                               cellW - 2.0f, r.getHeight());
            const bool rests = abs >= 0 && strip_.rests(lap + k);
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
    void stripMenu() {
        juce::PopupMenu m;
        m.addItem(1, tr("step-strip.play-every-step", "Play every step"), strip_.mask() != 0);
        m.addItem(2, tr("step-strip.invert", "Invert"));
        m.addItem(3, tr("step-strip.silence-the-off-beats", "Silence the off-beats"));
        m.showMenuAsync(juce::PopupMenu::Options(),
                        [safe = juce::Component::SafePointer<StepStripBrick>(this)](int r) {
                            if (safe == nullptr || r == 0) return;
                            if (r == 1) safe->strip_.playEvery();
                            if (r == 2) safe->strip_.invert();
                            if (r == 3) safe->strip_.silenceOffBeats();
                            safe->repaint();
                        });
    }

    void poll() override {
        if (strip_.poll()) repaint();
    }

    input::StepStripModel strip_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StepStripBrick)
};

}
