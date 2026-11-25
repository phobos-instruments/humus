// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/midi/MidiFormat.h"
#include "core/params/Randomize.h"
#include "gui/editor/AutomateMenu.h"
#include "gui/editor/ParamRanges.h"
#include "gui/editor/ParamSlider.h"
#include "gui/style/Colours.h"
#include "gui/style/ParamMarks.h"
#include "gui/editor/BrickBindings.h"
#include "gui/host/BrickHost.h"
#include "gui/host/EngineHostPattern.h"
#include "io/PatchDocument.h"
#include "gui/editor/FineDrag.h"
#include "gui/style/LookAndFeel.h"
#include "gui/pianoroll/PianoNotePicker.h"
#include "gui/editor/grids/SequenceGridModel.h"
#include "gui/editor/grids/StepGridModel.h"
#include "gui/bricks/StepGridReadout.h"
#include "gui/common/Localisation.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class SequenceGridBrick : public juce::Component, private juce::Timer {
public:
    static constexpr int kRows = grids::SequenceGridModel::kRows, kSteps = grids::SequenceGridModel::kSteps;

    SequenceGridBrick(BrickHost& host, std::string organism, const Bindings& bound, int rows = kRows)
        : host_(host), cn_(organism), viewParam_(bound(bind::kView)), chancePrefix_(bound(bind::kChancePrefix)),
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
            k.setParamId(cn_, grid_.velocityParam(r));
            k.onPopup = [this, r](juce::Point<int> at) {
                showAutomateMenu(host_, cn_, grid_.velocityParam(r), at, [this] { reload(); });
            };
            addAndMakeVisible(k);
            if (!chancePrefix_.empty()) setUpChanceKnob(r);
        }
        reload();
        startTimerHz(30);
    }

    void reload() {
        for (int r = 0; r < grid_.rows(); ++r) {
            auto& k = vel_[(size_t) r];
            const auto param = grid_.velocityParam(r);
            if (!k.isMouseButtonDown()) k.setValue(grid_.velocity(r), juce::dontSendNotification);
            k.setExternallyControlled(paramIsControlled(host_, cn_, param));
            k.setRollLocked(host_.rollLocked(cn_, param));
            if (chancePrefix_.empty()) continue;
            auto& c = chance_[(size_t) r];
            if (!c.isMouseButtonDown()) c.setValue(host_.liveParamValue(cn_, chanceParam(r)), juce::dontSendNotification);
            c.setExternallyControlled(paramIsControlled(host_, cn_, chanceParam(r)));
        }
        repaint();
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
            const bool locked = grid_.rowLocked(r);
            if (locked) paintParamMarks(g, plate.translated(0.0f, -1.5f), false, true);
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(9.5f));
            g.drawText(midiNoteName(grid_.note(r)),
                       plate.reduced(5.0f, 0.0f).withTrimmedRight(locked ? 10.0f : 0.0f).toNearestInt(),
                       juce::Justification::centredRight);
            const auto ticks = grid_.triggers(r);
            const auto chances = grid_.chances(r);
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
                if (!on) continue;
                const int chance = chanceAt(chances, s);
                if (chanceMode()) {
                    stepgrid::paintChanceBar(g, cell.reduced(3.0f), chance, Palette::background);
                    continue;
                }
                stepgrid::paintChanceFade(g, cell, chance, darkGroup ? Palette::panelLight.darker(0.25f) : Palette::panelLight);
                stepgrid::paintChanceTag(g, cell.reduced(0.0f, 2.0f), chance,
                                         chance < kFullChance / 2 ? Palette::text : Palette::background);
            }
            juce::ignoreUnused(row);
        }
        if (chanceRow_ >= 0)
            stepgrid::paintReadout(g, stepgrid::percentText(grid_.chance(chanceRow_, chanceStep_)),
                                   cellRect(chanceRow_, chanceStep_).toFloat().getCentreX(),
                                   (float) cellRect(chanceRow_, chanceStep_).getY(), (float) getWidth(),
                                   Palette::accent);
    }

    void resized() override {
        for (int r = 0; r < grid_.rows(); ++r) {
            vel_[(size_t) r].setBounds(knobRect(r));
            chance_[(size_t) r].setBounds(chanceKnobRect(r));
        }
    }

    void mouseDown(const juce::MouseEvent& e) override {
        for (int r = 0; r < grid_.rows(); ++r) {
            if (e.mods.isPopupMenu() && (ledRect(r).contains(e.getPosition()) || plateRect(r).contains(e.getPosition()))) {
                gridMenu(r);
                return;
            }
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
                if (e.mods.isPopupMenu()) {
                    if (chanceMode() && !chancePrefix_.empty()) rowChanceMenu(r, e.getScreenPosition());
                    else gridMenu(r);
                    return;
                }
                if (chanceMode() || e.mods.isCommandDown()) {
                    if (grid_.on(r, s)) beginChance(r, s, e.y);
                    return;
                }
                grid_.beginPaint(r, s);
                repaint();
                return;
            }
        }
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        if (chanceRow_ >= 0) {
            const int raw = chanceFrom_ + (chanceY_ - e.y);
            const int step = grids::StepGridModel::kChanceStep;
            if (grid_.setChance(chanceRow_, chanceStep_, (int) std::lround(raw / (double) step) * step)) repaint();
            return;
        }
        for (int r = 0; r < grid_.rows(); ++r)
            if (const int s = stepAt(r, e.getPosition()); s >= 0 && grid_.paintAt(r, s)) repaint();
    }
    void mouseUp(const juce::MouseEvent&) override {
        grid_.endPaint();
        if (chanceRow_ >= 0) {
            chanceRow_ = chanceStep_ = -1;
            repaint();
        }
    }

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
    juce::Rectangle<int> chanceKnobRect(int r) const {
        if (chancePrefix_.empty()) return {};
        return {kLedW + kKnobW + 4, r * (kRowH + kRowGap) + 1, kKnobW, kRowH - 2};
    }
    int plateX() const { return kLedW + kKnobW + 6 + (chancePrefix_.empty() ? 0 : kKnobW + 2); }
    juce::Rectangle<int> plateRect(int r) const {
        return {plateX(), r * (kRowH + kRowGap) + 3, kPlateW + kLedW + kKnobW + 6 - plateX(), kRowH - 6};
    }
    std::string chanceParam(int r) const { return chancePrefix_ + std::to_string(r + 1); }

    void rowChanceMenu(int r, juce::Point<int> at) {
        juce::PopupMenu head;
        head.addSectionHeader(tr("sequence-grid.row-chance-title", "Row {row} chance", {{"row", juce::String(r + 1)}}));
        showAutomateMenu(host_, cn_, chanceParam(r), at, [this] { reload(); }, true, {}, std::move(head));
    }

    void setUpChanceKnob(int r) {
        auto& c = chance_[(size_t) r];
        c.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        c.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        c.setRange(0.0, (double) kFullChance, (double) grids::StepGridModel::kChanceStep);
        applyFineCrawl(c);
        c.onValueChange = [this, r] { host_.editParam(cn_, chanceParam(r), chance_[(size_t) r].getValue()); };
        c.setParamId(cn_, chanceParam(r));
        c.setTooltip(tr("sequence-grid.row-chance", "Row chance: how often this row's steps play"));
        c.onPopup = [this, r](juce::Point<int> at) {
            showAutomateMenu(host_, cn_, chanceParam(r), at, [this] { reload(); });
        };
        addAndMakeVisible(c);
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

    void beginChance(int row, int step, int y) {
        host_.pushUndo();
        chanceRow_ = row;
        chanceStep_ = step;
        chanceY_ = y;
        chanceFrom_ = grid_.chance(row, step);
        repaint();
    }

    static juce::String bankLetter(int bank) { return juce::String::charToString((juce::juce_wchar) ('A' + bank)); }

    void gridMenu(int row) {
        const int cur = grid_.bank();
        const bool locked = grid_.rowLocked(row);
        juce::PopupMenu m;
        m.addItem(3, tr("sequence-grid.exclude-row", "Exclude Row from Random"), true, locked);
        m.addSeparator();
        m.addItem(1, tr("pattern-step-grid.random", "Random"));
        m.addItem(2, tr("pattern-step-grid.clear", "Clear"));
        m.addSeparator();
        for (int b = 0; b < kPatternBanks; ++b)
            if (b != cur)
                m.addItem(10 + b, tr("pattern-step-grid.copy-to-bank", "Copy to bank") + " " + bankLetter(b));
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
                        [this, cur, row, locked](int r) {
            if (r == 0) return;
            if (r == 3) {
                grid_.setRowLocked(row, !locked);
                reload();
                return;
            }
            using Action = grids::SequenceGridModel::BankAction;
            JuceDice dice(juce::Random::getSystemRandom());
            grid_.bankAction(r == 1 ? Action::Random : r == 2 ? Action::Clear : Action::CopyTo, r >= 10 ? r - 10 : cur,
                             dice);
            repaint();
        });
    }

    bool chanceMode() const {
        return !viewParam_.empty() && stepgrid::viewOf(host_.liveParamValue(cn_, viewParam_)) == stepgrid::kChanceView;
    }

    void timerCallback() override {
        const bool chance = chanceMode();
        if (grid_.followPlayhead() || chance != chanceShown_) repaint();
        chanceShown_ = chance;
    }

    BrickHost& host_;
    std::string cn_;
    std::string viewParam_;
    std::string chancePrefix_;
    grids::SequenceGridModel grid_;
    ParamSlider vel_[kRows];
    ParamSlider chance_[kRows];
    bool chanceShown_ = false;
    int chanceRow_ = -1, chanceStep_ = -1, chanceY_ = 0, chanceFrom_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequenceGridBrick)
};

}
