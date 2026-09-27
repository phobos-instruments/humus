// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/midi/MidiFormat.h"
#include "core/params/Randomize.h"
#include "gui/style/Colours.h"
#include "gui/editor/BrickBindings.h"
#include "gui/host/BrickHost.h"
#include "gui/host/EngineHostPattern.h"
#include "io/PatchDocument.h"
#include "gui/editor/FineDrag.h"
#include "gui/style/LookAndFeel.h"
#include "gui/pianoroll/PianoNotePicker.h"
#include "gui/editor/grids/SequenceGridModel.h"
#include "gui/common/Localisation.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class SequenceGridBrick : public juce::Component, private juce::Timer {
public:
    static constexpr int kRows = grids::SequenceGridModel::kRows, kSteps = grids::SequenceGridModel::kSteps;

    SequenceGridBrick(BrickHost& host, std::string organism, const Bindings& bound, int rows = kRows)
        : host_(host), cn_(organism),
          grid_(host, host.patterns(), organism,
                {bound(bind::kEnablePrefix), bound(bind::kNotePrefix), bound(bind::kVelocityPrefix)}, rows) {
        for (int r = 0; r < grid_.rows(); ++r) {
            auto& k = vel_[(size_t) r];
            k.setSliderStyle(juce::Slider::RotaryVerticalDrag);
            k.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
            k.setRange(0.0, 1.0, 0.0);
            applyFineCrawl(k);
            k.setValue(grid_.velocity(r), juce::dontSendNotification);
            k.onValueChange = [this, r] { grid_.setVelocity(r, vel_[(size_t) r].getValue()); };
            addAndMakeVisible(k);
        }
        startTimerHz(30);
    }

    void paint(juce::Graphics& g) override {
        for (int r = 0; r < grid_.rows(); ++r) {
            const auto row = rowBounds(r);
            const bool en = grid_.enabled(r);
            const auto led = ledRect(r).toFloat();
            g.setColour(en ? ink::state::stepOn : Palette::panelLight);
            g.fillEllipse(led);
            g.setColour(Palette::border);
            g.drawEllipse(led, 1.0f);
            const auto plate = plateRect(r).toFloat();
            g.setColour(Palette::panelLight);
            g.fillRoundedRectangle(plate, 4.0f);
            g.setColour(Palette::border);
            g.drawRoundedRectangle(plate, 4.0f, 1.0f);
            g.setColour(Palette::text);
            g.setFont(juce::FontOptions(11.5f));
            g.drawText(grid_.targetName(r), plate.reduced(7.0f, 0.0f).toNearestInt(),
                       juce::Justification::centredLeft);
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(9.5f));
            g.drawText(midiNoteName(grid_.note(r)),
                       plate.reduced(5.0f, 0.0f).toNearestInt(),
                       juce::Justification::centredRight);
            const auto ticks = grid_.triggers(r);
            for (int s = 0; s < kSteps; ++s) {
                const auto cell = cellRect(r, s).toFloat();
                const bool on = std::find(ticks.begin(), ticks.end(), s * grids::SequenceGridModel::kStepTicks)
                                != ticks.end();
                const bool darkGroup = ((s / 4) & 1) != 0;
                juce::Colour c = on ? Palette::accent
                                    : darkGroup ? Palette::panelLight.darker(0.25f)
                                                : Palette::panelLight;
                if (grid_.playStep() == s) c = c.brighter(on ? 0.35f : 0.12f);
                if (!en) c = c.withAlpha(alpha::dim);
                g.setColour(c);
                g.fillRoundedRectangle(cell, 3.0f);
                g.setColour(Palette::border.withAlpha(alpha::strong));
                g.drawRoundedRectangle(cell, 3.0f, 1.0f);
            }
            juce::ignoreUnused(row);
        }
    }

    void resized() override {
        for (int r = 0; r < grid_.rows(); ++r) vel_[(size_t) r].setBounds(knobRect(r));
    }

    void mouseDown(const juce::MouseEvent& e) override {
        for (int r = 0; r < grid_.rows(); ++r) {
            if (ledRect(r).contains(e.getPosition())) {
                grid_.toggleEnabled(r);
                repaint();
                return;
            }
            if (plateRect(r).contains(e.getPosition())) {
                auto anchor = localAreaToGlobal(plateRect(r));
                showNotePicker(host_, cn_, grid_.noteParam(r), anchor, 0, kMidiMax,
                               [this] { repaint(); });
                return;
            }
            const int s = stepAt(r, e.getPosition());
            if (s >= 0) {
                if (e.mods.isPopupMenu()) { bankMenu(); return; }
                grid_.beginPaint(r, s);
                repaint();
                return;
            }
        }
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        for (int r = 0; r < grid_.rows(); ++r)
            if (const int s = stepAt(r, e.getPosition()); s >= 0 && grid_.paintAt(r, s)) repaint();
    }
    void mouseUp(const juce::MouseEvent&) override { grid_.endPaint(); }

    void mouseWheelMove(const juce::MouseEvent& e,
                        const juce::MouseWheelDetails& wheel) override {
        for (int r = 0; r < grid_.rows(); ++r)
            if (plateRect(r).contains(e.getPosition())) {
                grid_.stepNote(r, wheel.deltaY > 0 ? 1 : -1);
                repaint();
                return;
            }
    }

private:
    static constexpr int kRowH = 28, kRowGap = 3;
    static constexpr int kLedW = 14, kKnobW = 26, kPlateW = 116, kHeadGap = 6;

    juce::Rectangle<int> rowBounds(int r) const {
        return {0, r * (kRowH + kRowGap), getWidth(), kRowH};
    }
    juce::Rectangle<int> ledRect(int r) const {
        return {2, r * (kRowH + kRowGap) + (kRowH - 10) / 2, 10, 10};
    }
    juce::Rectangle<int> knobRect(int r) const {
        return {kLedW + 2, r * (kRowH + kRowGap) + 1, kKnobW, kRowH - 2};
    }
    juce::Rectangle<int> plateRect(int r) const {
        return {kLedW + kKnobW + 6, r * (kRowH + kRowGap) + 3, kPlateW, kRowH - 6};
    }
    int gridX() const { return kLedW + kKnobW + kPlateW + 6 + kHeadGap; }
    juce::Rectangle<int> cellRect(int r, int s) const {
        const int cw = (getWidth() - gridX() - 3 * 6) / kSteps;
        const int x = gridX() + s * cw + (s / 4) * 6;
        return {x, r * (kRowH + kRowGap) + 2, cw - 3, kRowH - 4};
    }
    int stepAt(int r, juce::Point<int> p) const {
        for (int s = 0; s < kSteps; ++s)
            if (cellRect(r, s).contains(p)) return s;
        return -1;
    }

    static juce::String bankLetter(int bank) { return juce::String::charToString((juce::juce_wchar) ('A' + bank)); }

    void bankMenu() {
        const int cur = grid_.bank();
        juce::PopupMenu m;
        m.addItem(1, tr("pattern-step-grid.random", "Random"));
        m.addItem(2, tr("pattern-step-grid.clear", "Clear"));
        m.addSeparator();
        for (int b = 0; b < kPatternBanks; ++b)
            if (b != cur)
                m.addItem(10 + b, tr("pattern-step-grid.copy-to-bank", "Copy to bank") + " " + bankLetter(b));
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
                        [this, cur](int r) {
            if (r == 0) return;
            using Action = grids::SequenceGridModel::BankAction;
            JuceDice dice(juce::Random::getSystemRandom());
            grid_.bankAction(r == 1 ? Action::Random : r == 2 ? Action::Clear : Action::CopyTo, r >= 10 ? r - 10 : cur,
                             dice);
            repaint();
        });
    }

    void timerCallback() override {
        if (grid_.followPlayhead()) repaint();
    }

    BrickHost& host_;
    std::string cn_;
    grids::SequenceGridModel grid_;
    juce::Slider vel_[kRows];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequenceGridBrick)
};

}
