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
#include "gui/bricks/StepGridReadout.h"
#include "gui/editor/AutomateMenu.h"
#include "gui/editor/inputs/StripInputs.h"

namespace hum {

class StepStripBrick : public PolledBrick, public juce::SettableTooltipClient {
public:
    StepStripBrick(BrickHost& host, std::string cn, const Bindings& bound)
        : PolledBrick(host, cn), strip_(host, cn, bound(bind::kMute), bound(bind::kChancePrefix)) {
        setTooltip(strip_.hasChances()
                       ? tr("step-strip.click-or-chance",
                            "Click a step to silence it; drag to sweep. Cmd-drag a step up or down for its chance. Right-click for more")
                       : tr("step-strip.click-a-step-to-silence", "Click a step to silence it; drag to sweep. Right-click for the whole strip"));
    }

    void reloadValues() override { repaint(); }
    void refreshAutomatedValues() override { repaint(); }
    int preferredContentWidth() const override { return 522; }
    int preferredContentHeight(int) const override { return 16; }

    void mouseDown(const juce::MouseEvent& e) override {
        const int cell = strip_.cellAt(e.position.x, getWidth());
        if (e.mods.isPopupMenu()) { stripMenu(cell, e.getScreenPosition()); return; }
        if (strip_.hasChances() && e.mods.isCommandDown()) {
            chanceCell_ = cell;
            chanceFrom_ = strip_.chance(cell);
            chanceY_ = e.y;
            repaint();
            return;
        }
        strip_.press(cell);
        repaint();
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        if (chanceCell_ >= 0) {
            strip_.setChance(chanceCell_, chanceFrom_ + (chanceY_ - e.y));
            repaint();
            return;
        }
        if (strip_.sweep(strip_.cellAt(e.position.x, getWidth()))) repaint();
    }
    void mouseUp(const juce::MouseEvent&) override {
        if (chanceCell_ < 0) return;
        chanceCell_ = -1;
        repaint();
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
                stepgrid::paintChanceFade(g, cell, strip_.chance(k), Palette::background);
            } else if (rests) {
                g.setColour(Palette::background);
                g.fillRoundedRectangle(cell.reduced(0.0f, cell.getHeight() * 0.3f), 2.0f);
            } else {
                g.setColour(Palette::panelLight);
                g.fillRoundedRectangle(cell, 2.0f);
                stepgrid::paintChanceFade(g, cell, strip_.chance(k), Palette::background);
                g.setColour(Palette::border);
                g.drawRoundedRectangle(cell.reduced(0.5f), 2.0f, 1.0f);
            }
            if (((mute >> k) & 1) == 0) stepgrid::paintChanceTag(g, cell.withTrimmedTop(1.0f), strip_.chance(k), Palette::textDim);
        }
        if (chanceCell_ >= 0) {
            const auto held = juce::Rectangle<float>(r.getX() + chanceCell_ * cellW, r.getY(), cellW - 2.0f, r.getHeight());
            g.setColour(Palette::background.withAlpha(alpha::nearOpaque));
            g.fillRoundedRectangle(held, 2.0f);
            g.setColour(Palette::text);
            g.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
            g.drawText(stepgrid::percentText(strip_.chance(chanceCell_)), held, juce::Justification::centred, false);
        }
    }

private:
    void stripMenu(int cell, juce::Point<int> at) {
        if (!strip_.hasChances()) { stripMenu(); return; }
        juce::PopupMenu head;
        head.addSectionHeader(tr("pattern-step-grid.step-chance-title", "Step {step} chance", {{"step", juce::String(cell + 1)}}));
        const int now = strip_.chance(cell);
        for (int value : {100, 75, 50, 25, 10, 0})
            head.addItem(kChanceItems + value, stepgrid::percentText(value), true, now == value);
        head.addSeparator();
        head.addItem(kStripItems + 1, tr("step-strip.play-every-step", "Play every step"), strip_.mask() != 0);
        head.addItem(kStripItems + 2, tr("step-strip.invert", "Invert"));
        head.addItem(kStripItems + 3, tr("step-strip.silence-the-off-beats", "Silence the off-beats"));
        showAutomateMenu(host_, name_, strip_.chanceParam(cell), at,
                         [safe = juce::Component::SafePointer<StepStripBrick>(this)] { if (safe != nullptr) safe->repaint(); },
                         true, {}, std::move(head),
                         [safe = juce::Component::SafePointer<StepStripBrick>(this), cell](int r) {
                             if (safe == nullptr) return;
                             if (r >= kChanceItems && r <= kChanceItems + kFullChance) safe->strip_.setChance(cell, r - kChanceItems);
                             if (r == kStripItems + 1) safe->strip_.playEvery();
                             if (r == kStripItems + 2) safe->strip_.invert();
                             if (r == kStripItems + 3) safe->strip_.silenceOffBeats();
                             safe->repaint();
                         });
    }

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

    static constexpr int kChanceItems = kAutomateMenuFirstOwnId;
    static constexpr int kStripItems = kAutomateMenuFirstOwnId + 200;

    input::StepStripModel strip_;
    int chanceCell_ = -1, chanceFrom_ = 0, chanceY_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StepStripBrick)
};

}
