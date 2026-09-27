// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/midi/MidiFormat.h"
#include "gui/editor/grids/HandGestureModel.h"
#include "gui/editor/readouts/ScopeReadings.h"
#include "gui/style/Colours.h"
#include "gui/editor/BrickBindings.h"
#include "gui/host/BrickHost.h"
#include "hum/Organism.h"
#include "io/PatchDocument.h"
#include "gui/style/LookAndFeel.h"
#include "hum/caps/Video.h"
#include "gui/common/Localisation.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class HandGestureBrick : public juce::Component, private juce::Timer {
public:
    HandGestureBrick(BrickHost& host, std::string organism, const Bindings& bound)
        : model_(host, std::move(organism),
                 {bound(bind::kGestures), bound(bind::kTolerance), bound(bind::kNotePrefix),
                  bound(bind::kThresholdPrefix)}) {
        for (int k = 0; k < gvec::kSlots; ++k) {
            auto& learn = learn_[(size_t) k];
            learn.setButtonText(tr("hand-gesture.learn", "Learn"));
            learn.onClick = [this, k] { startLearn(k, false); };
            addAndMakeVisible(learn);
            auto& reinf = reinforce_[(size_t) k];
            reinf.setButtonText(tr("hand-gesture.reinf", "Reinf"));
            reinf.setTooltip(tr("hand-gesture.reinforce",
                                "Capture a variation of the same pose and fold it "
                                "into the template (running average)"));
            reinf.onClick = [this, k] { startLearn(k, true); };
            addAndMakeVisible(reinf);
            auto& clear = clear_[(size_t) k];
            clear.setButtonText(juce::String::fromUTF8("\xc3\x97"));
            clear.onClick = [this, k] { clearSlot(k); };
            addAndMakeVisible(clear);
        }
        startTimerHz(20);
    }

    void paint(juce::Graphics& g) override {
        const auto set = model_.set();
        auto* src = model_.source();
        float matches[gvec::kSlots] = {};
        const bool present = model_.matches(matches);
        for (int k = 0; k < gvec::kSlots; ++k) {
            const auto row = rowBounds(k);
            g.setColour(Palette::text);
            g.setFont(juce::FontOptions(11.5f));
            g.drawText(juce::String(k + 1), row.withWidth(14), juce::Justification::centredLeft);
            const auto bar = barBounds(k).toFloat();
            g.setColour(Palette::panelLight);
            g.fillRoundedRectangle(bar, 3.0f);
            if (model_.learning() == k) {
                g.setColour(Palette::accent.withAlpha(alpha::dim));
                g.fillRoundedRectangle(bar.withWidth(bar.getWidth()
                                                     * (float) model_.samples() / (float) grids::HandGestureModel::kNeed), 3.0f);
                g.setColour(Palette::text);
                g.setFont(juce::FontOptions(10.0f));
                g.drawText(src == nullptr ? "..."
                           : present ? src->gestureHoldPrompt() : src->gestureAbsentPrompt(),
                           bar.toNearestInt(), juce::Justification::centred);
            } else if (set.learned[(size_t) k]) {
                const float thr = model_.threshold(k);
                const float m = matches[k];
                g.setColour(m > thr ? Palette::accent : Palette::accentDim);
                g.fillRoundedRectangle(bar.withWidth(std::max(2.0f, bar.getWidth() * m)), 3.0f);
                const float tx = bar.getX() + bar.getWidth() * thr;
                g.setColour(Palette::text);
                g.fillRect(tx - 1.0f, bar.getY() - 2.0f, 2.0f, bar.getHeight() + 4.0f);
                if (set.count[(size_t) k] > 1) {
                    g.setColour(Palette::textDim);
                    g.setFont(juce::FontOptions(9.0f));
                    g.drawText("x" + juce::String(set.count[(size_t) k]),
                               bar.toNearestInt().reduced(4, 0),
                               juce::Justification::centredRight);
                }
            } else {
                g.setColour(Palette::textDim);
                g.setFont(juce::FontOptions(10.0f));
                g.drawText("empty", bar.toNearestInt(), juce::Justification::centred);
            }
            g.setColour(Palette::border);
            g.drawRoundedRectangle(bar, 3.0f, 1.0f);
            const auto nr = noteRect(k);
            g.setColour(Palette::panelLight);
            g.fillRoundedRectangle(nr.toFloat(), 3.0f);
            g.setColour(Palette::border);
            g.drawRoundedRectangle(nr.toFloat(), 3.0f, 1.0f);
            g.setColour(Palette::text);
            g.setFont(juce::FontOptions(10.5f));
            g.drawText(juce::String(noteName(model_.note(k))), nr, juce::Justification::centred);
        }
    }

    void resized() override {
        for (int k = 0; k < gvec::kSlots; ++k) {
            auto row = rowBounds(k);
            row.removeFromLeft(16);
            learn_[(size_t) k].setBounds(row.removeFromLeft(46).reduced(0, 1));
            row.removeFromLeft(2);
            reinforce_[(size_t) k].setBounds(row.removeFromLeft(46).reduced(0, 1));
            clear_[(size_t) k].setBounds(row.removeFromRight(22).reduced(0, 1));
        }
    }

private:
    static constexpr int kRowH = 24, kGap = 4;

    juce::Rectangle<int> rowBounds(int k) const {
        return {0, k * (kRowH + kGap), getWidth(), kRowH};
    }
    juce::Rectangle<int> barBounds(int k) const {
        auto r = rowBounds(k);
        r.removeFromLeft(16 + 46 + 2 + 46 + 6);
        r.removeFromRight(22 + 6 + 38 + 4);
        return r.reduced(0, 4);
    }
    juce::Rectangle<int> noteRect(int k) const {
        auto r = rowBounds(k);
        r.removeFromRight(22 + 6);
        return r.removeFromRight(38).reduced(0, 3);
    }

    void startLearn(int slot, bool asReinforce) {
        model_.startLearn(slot, asReinforce);
        repaint();
    }

    void clearSlot(int slot) {
        model_.clear(slot);
        repaint();
    }

    void timerCallback() override {
        model_.tick();
        repaint();
    }

    void mouseDown(const juce::MouseEvent& e) override {
        dragThr_ = dragNote_ = -1;
        for (int k = 0; k < gvec::kSlots; ++k) {
            if (barBounds(k).expanded(0, 3).contains(e.getPosition())) {
                dragThr_ = k;
                applyThr(k, e.x);
                return;
            }
            if (noteRect(k).contains(e.getPosition())) {
                dragNote_ = k;
                dragStartY_ = e.y;
                dragStartNote_ = model_.note(k);
                return;
            }
        }
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        if (dragThr_ >= 0) applyThr(dragThr_, e.x);
        else if (dragNote_ >= 0) {
            model_.setNote(dragNote_, dragStartNote_ + (dragStartY_ - e.y) / 4);
            repaint();
        }
    }
    void mouseUp(const juce::MouseEvent&) override { dragThr_ = dragNote_ = -1; }
    void mouseWheelMove(const juce::MouseEvent& e,
                        const juce::MouseWheelDetails& wheel) override {
        for (int k = 0; k < gvec::kSlots; ++k)
            if (noteRect(k).contains(e.getPosition())) {
                model_.setNote(k, model_.note(k) + (wheel.deltaY > 0 ? 1 : -1));
                repaint();
                return;
            }
    }

    void applyThr(int k, int px) {
        const auto bar = barBounds(k);
        model_.setThreshold(k, (double) (px - bar.getX()) / (double) juce::jmax(1, bar.getWidth()));
        repaint();
    }

    grids::HandGestureModel model_;
    int dragThr_ = -1, dragNote_ = -1;
    int dragStartY_ = 0, dragStartNote_ = 60;
    std::array<juce::TextButton, gvec::kSlots> learn_;
    std::array<juce::TextButton, gvec::kSlots> reinforce_;
    std::array<juce::TextButton, gvec::kSlots> clear_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HandGestureBrick)
};

}
