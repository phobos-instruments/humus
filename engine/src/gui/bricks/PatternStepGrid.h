// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/packs/Categories.h"
#include "core/midi/MidiFormat.h"
#include "core/params/Randomize.h"
#include "gui/style/Colours.h"
#include "gui/style/DropBadge.h"
#include "gui/host/BrickHost.h"
#include "gui/host/EngineHostPattern.h"
#include "io/PatchDocument.h"
#include "gui/style/LookAndFeel.h"
#include "gui/bricks/StepCellPaint.h"
#include "gui/bricks/StepChanceView.h"
#include "gui/editor/AutomateMenu.h"
#include "gui/bricks/StepGridReadout.h"
#include "gui/bricks/StepPlayhead.h"
#include "gui/common/UiTicker.h"
#include "gui/common/Localisation.h"
#include "gui/bricks/RiffFileImport.h"
#include "gui/editor/grids/StepGridModel.h"

namespace hum {

class PatternStepGrid : public juce::Component, public juce::FileDragAndDropTarget, public juce::TooltipClient {
public:
    using Mode = grids::StepGridModel::Mode;

    PatternStepGrid(BrickHost& host, std::string name, Mode mode, std::string nudgeParam = {},
                    std::string transposeParam = {}, std::string viewParam = {}, std::string chancePrefix = {})
        : host_(host), name_(name), mode_(mode),
          grid_(host, host.patterns(), name, mode, std::move(nudgeParam), std::move(transposeParam),
                std::move(chancePrefix)),
          chance_(host, name, std::move(viewParam), grid_) {
        tickerId_ = UiTicker::instance().add([this] {
            pollPlayhead();
            pollBank();
            if (chance_.changed()) repaint();
        });
    }
    ~PatternStepGrid() override { UiTicker::instance().remove(tickerId_); }

    void reload() { repaint(); }

    void paint(juce::Graphics& g) override {
        g.fillAll(Palette::panel.darker(0.06f));
        if (mode_ == Mode::Bassline) paintBassline(g);
        else                         paintArp(g);
        chance_.paint(g, playingSteps(), (float) gridWidth(), (float) getHeight(), familyColour());
        paintPlayhead(g);
        if (chance_.available()) stepgrid::paintChanceKey(g, chanceKey(), chance_.on(), familyColour());
    }

    juce::String getTooltip() override {
        return chance_.available() && chanceKey().contains(getMouseXYRelative().toFloat()) ? stepgrid::chanceKeyTip()
                                                                                          : juce::String();
    }


    bool isInterestedInFileDrag(const juce::StringArray& files) override {
        if (mode_ != Mode::Bassline) return false;
        for (const auto& f : files)
            if (isRiffFile(f)) return true;
        return false;
    }
    void fileDragEnter(const juce::StringArray&, int, int) override { dropHot_ = true; repaint(); }
    void fileDragExit(const juce::StringArray&) override { dropHot_ = false; repaint(); }
    void paintOverChildren(juce::Graphics& g) override {
        if (dropHot_) drop::paintTarget(g, getLocalBounds().toFloat());
    }
    void filesDropped(const juce::StringArray& files, int, int) override {
        dropHot_ = false;
        importRiffFiles(host_, name_, files);
        repaint();
    }

    void mouseMove(const juce::MouseEvent& e) override { setMouseCursor(cursorFor(e, false)); }

    void mouseDown(const juce::MouseEvent& e) override {
        if (chance_.available() && chanceKey().contains(e.position)) {
            if (!e.mods.isPopupMenu()) {
                chance_.toggle();
                repaint();
            }
            return;
        }
        if (e.x >= gridWidth()) return;
        if (chance_.on() && e.mods.isPopupMenu()) {
            chanceMenu(grid_.columnAt(e.x, gridWidth()), e.getScreenPosition());
            return;
        }
        if (mode_ == Mode::Arp && e.mods.isPopupMenu()) { stampMenu(); return; }
        if (!e.mods.isPopupMenu() && (chance_.on() || (chance_.available() && e.mods.isCommandDown()))) {
            grid_.pressed();
            chance_.setDragging(true);
            grid_.setChanceAt(e.x, e.y, gridWidth(), getHeight());
            repaint();
            return;
        }
        setMouseCursor(cursorFor(e, !e.mods.isPopupMenu()));
        grid_.pressed();
        if (mode_ == Mode::Bassline && e.mods.isAltDown()) {
            grid_.beginSlide(e.x);
            return;
        }
        apply(e, true);
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        if (e.mouseDownPosition.x >= (float) gridWidth()) return;
        if (chance_.dragging()) { grid_.setChanceAt(e.x, e.y, gridWidth(), getHeight()); repaint(); return; }
        if (grid_.sliding()) { grid_.slideTo(e.x, gridWidth()); repaint(); return; }
        apply(e, false);
    }
    void mouseUp(const juce::MouseEvent& e) override {
        setMouseCursor(cursorFor(e, false));
        grid_.endSlide();
        chance_.setDragging(false);
        if (grid_.releaseDrag()) repaint();
    }

    int gridWidth() const { return std::max(1, getWidth() - stepgrid::kChanceKeyW - 4); }
    juce::Rectangle<float> chanceKey() const { return StepChanceView::key(getWidth(), mode_ == Mode::Arp ? 4.0f : 0.0f); }

    juce::MouseCursor cursorFor(const juce::MouseEvent& e, bool held) const {
        if (chanceKey().contains(e.position)) return juce::MouseCursor::PointingHandCursor;
        if (chance_.on() || (chance_.available() && e.mods.isCommandDown())) return juce::MouseCursor::UpDownResizeCursor;
        if (mode_ != Mode::Bassline) return juce::MouseCursor::PointingHandCursor;
        if (e.mods.isAltDown()) return juce::MouseCursor::LeftRightResizeCursor;
        if (e.y >= getHeight() - 2 * kRowH) return juce::MouseCursor::PointingHandCursor;
        return held ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::UpDownResizeCursor;
    }

private:
    static constexpr int kRowH = grids::StepGridModel::kRowH;

    void paintBassline(juce::Graphics& g) {
        const auto steps = grid_.shownBassline();
        const int n = std::max(1, (int) steps.size());
        const float cw = gridWidth() / (float) n;
        const int laneBottom = getHeight() - 2 * kRowH;
        const auto fam = familyColour();
        for (int i = 0; i < (int) steps.size(); ++i) {
            const float x = i * cw;
            if (i % 4 == 0) { g.setColour(Palette::panelLight.withAlpha(alpha::dim)); g.fillRect(x, 0.0f, cw, (float) getHeight()); }
            g.setColour(Palette::background.withAlpha(alpha::mid));
            g.drawVerticalLine((int) x, 0.0f, (float) getHeight());
            const auto& s = steps[(size_t) i];
            if (s.gate) {
                const float ny = grids::StepGridModel::noteY(s.note, laneBottom);
                const juce::Rectangle<float> bar(x + 1, ny - 3, cw - 2, 6.0f);
                if (s.accent) {
                    g.setColour(fam.withAlpha(alpha::scrim));
                    g.fillRoundedRectangle(bar.expanded(2.0f, 2.5f), 4.0f);
                    g.setColour(fam.brighter(0.3f));
                } else {
                    g.setColour(Palette::text);
                }
                g.fillRoundedRectangle(bar, 2.5f);
                if (i != grid_.dragColumn()) paintNoteLabel(g, s.note, x, cw, ny, laneBottom, s.accent, fam);
            }
            auto cell = [&](int row, bool on, const char* lbl) {
                juce::Rectangle<float> r(x + 1, (float) (laneBottom + row * kRowH) + 1, cw - 2, (float) kRowH - 2);
                if (on) paintSporeCap(g, r, fam);
                else    paintSoilCell(g, r);
                g.setColour(on ? Palette::background : Palette::textDim);
                g.setFont(10.0f); g.drawText(lbl, r, juce::Justification::centred);
            };
            cell(0, s.accent, "A");
            cell(1, s.slide, "S");
        }
        g.setColour(Palette::background.withAlpha(alpha::mid));
        g.drawHorizontalLine(laneBottom, 0.0f, (float) gridWidth());
        const int dragCol = grid_.dragColumn();
        if (!chance_.dragging() && dragCol >= 0 && dragCol < (int) steps.size() && steps[(size_t) dragCol].gate)
            paintNoteReadout(g, steps[(size_t) dragCol].note, dragCol * cw + cw * 0.5f,
                             grids::StepGridModel::noteY(steps[(size_t) dragCol].note, laneBottom), fam);
    }

    void paintNoteLabel(juce::Graphics& g, int note, float x, float cw, float barY, int laneBottom, bool accent,
                        juce::Colour fam) {
        static constexpr float kLabelH = 11.0f;
        const float top = grids::StepGridModel::noteLabelTop(barY, laneBottom, kLabelH);
        g.setFont(juce::FontOptions(9.5f));
        g.setColour(accent ? fam.brighter(0.3f) : Palette::textDim);
        g.drawText(juce::String(midiNoteName(note)), juce::Rectangle<float>(x, top, cw, kLabelH),
                   juce::Justification::centred, false);
    }

    juce::String noteReadoutText(int note) const { return juce::String(grid_.noteReadout(note)); }

    void paintNoteReadout(juce::Graphics& g, int note, float cx, float barY, juce::Colour fam) {
        stepgrid::paintReadout(g, noteReadoutText(note), cx, barY, (float) gridWidth(), fam);
    }

    std::vector<bool> playingSteps() const {
        std::vector<bool> plays;
        if (mode_ == Mode::Bassline)
            for (const auto& s : grid_.shownBassline()) plays.push_back(s.gate);
        else
            for (const auto& s : grid_.arpSteps()) plays.push_back(s.trigger);
        return plays;
    }

    void chanceMenu(int col, juce::Point<int> at) {
        const auto param = grid_.chanceParamAtColumn(col);
        if (param.empty()) { gridMenu(at); return; }
        juce::PopupMenu head;
        head.addSectionHeader(tr("pattern-step-grid.step-chance-title", "Step {step} chance", {{"step", juce::String(col + 1)}}));
        showAutomateMenu(host_, name_, param, at, [safe = juce::Component::SafePointer<PatternStepGrid>(this)] {
            if (safe != nullptr) safe->repaint();
        }, true, {}, std::move(head));
    }

    void gridMenu(juce::Point<int> at) {
        if (mode_ == Mode::Bassline) bankMenu(at);
        else stampMenu();
    }

    void applyBassline(const juce::MouseEvent& e, bool down) {
        const auto hit = grid_.bassline(e.x, e.y, gridWidth(), getHeight(), down, e.mods.isPopupMenu());
        if (hit == grids::StepGridModel::Hit::BankMenu) bankMenu(e.getScreenPosition());
        if (hit == grids::StepGridModel::Hit::Changed) repaint();
    }

    static juce::String bankLetter(int bank) { return juce::String::charToString((juce::juce_wchar) ('A' + bank)); }

    void bankMenu(juce::Point<int> at) {
        const int cur = grid_.bank();
        juce::PopupMenu m;
        m.addItem(1, tr("pattern-step-grid.random", "Random"));
        m.addItem(2, tr("pattern-step-grid.clear", "Clear"));
        m.addItem(4, tr("pattern-step-grid.nudge-left", "Nudge left (a step earlier)"));
        m.addItem(5, tr("pattern-step-grid.nudge-right", "Nudge right (a step later)"));
        m.addItem(3, tr("pattern-step-grid.import", "Load riffs..."));
        m.addSeparator();
        for (int b = 0; b < kPatternBanks; ++b)
            if (b != cur)
                m.addItem(10 + b, tr("pattern-step-grid.copy-to-bank", "Copy to bank") + " " + bankLetter(b));
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
                        [this, cur, at](int r) {
            if (r == 0) return;
            if (r == 3) { browseRiffs(at); return; }
            using Action = grids::StepGridModel::BankAction;
            const auto action = r == 1 ? Action::Random : r == 2 ? Action::Clear : r == 4 ? Action::NudgeLeft
                              : r == 5 ? Action::NudgeRight : Action::CopyTo;
            JuceDice dice(juce::Random::getSystemRandom());
            grid_.bankAction(action, r >= 10 ? r - 10 : cur, dice);
            repaint();
        });
    }

    void browseRiffs(juce::Point<int> at) {
        juce::Component::SafePointer<PatternStepGrid> safe(this);
        hum::browseRiffs(host_, name_, juce::Rectangle<int>(at.x, at.y, 1, 1),
                         [safe] { if (safe != nullptr) safe->chooseRiffFiles(); },
                         [safe] { if (safe != nullptr) safe->repaint(); });
    }

    void chooseRiffFiles() {
        hum::chooseRiffFiles(host_, name_, picker_,
                             [safe = juce::Component::SafePointer<PatternStepGrid>(this)] {
            if (safe != nullptr) safe->repaint();
        });
    }

    juce::Colour familyColour() const {
        const auto* cm = host_.model().byName(name_);
        return Palette::familyAccent(cm ? familyOf(cm->displayClass) : Family::Voice);
    }

    void paintArp(juce::Graphics& g) {
        auto steps = grid_.arpSteps();
        auto ups = grid_.arpUps();
        const int n = std::max(1, (int) steps.size());
        const float cw = gridWidth() / (float) n;
        const int upTop = getHeight() - 2 * kRowH;
        const int tieTop = getHeight() - kRowH;
        const auto fam = familyColour();
        for (int i = 0; i < (int) steps.size(); ++i) {
            const float x = i * cw;
            if (i % 4 == 0) { g.setColour(Palette::panelLight.withAlpha(alpha::dim)); g.fillRect(x, 0.0f, cw, (float) getHeight()); }
            const auto& s = steps[(size_t) i];
            juce::Rectangle<float> cell(x + 2, 4.0f, cw - 4, (float) (upTop - 8));
            if (s.trigger) paintSporeCap(g, cell, fam);
            else           paintSoilCell(g, cell);
        }
        g.setFont(juce::FontOptions(9.0f).withStyle("Bold"));
        for (int i = 0; i < (int) steps.size(); ++i) {
            const float x = i * cw;
            const bool up = (size_t) i < ups.size() && ups[(size_t) i];
            const auto socket = juce::Rectangle<float>(x + 1, (float) upTop + 1,
                                                       cw - 2, (float) kRowH - 2)
                                    .reduced(cw * 0.28f, 4.0f);
            if (up) paintSporeCap(g, socket, fam);
            else    paintSoilCell(g, socket);
            g.setColour(up ? Palette::background : Palette::textDim.withAlpha(alpha::mid));
            g.drawText("^", socket.expanded(2.0f, 3.0f), juce::Justification::centred);
        }
        for (int i = 0; i < (int) steps.size(); ++i) {
            const float x = i * cw;
            const auto& s = steps[(size_t) i];
            const auto socket = juce::Rectangle<float>(x + 1, (float) tieTop + 1,
                                                       cw - 2, (float) kRowH - 2)
                                    .reduced(cw * 0.28f, 4.0f);
            if (s.tie) paintSporeCap(g, socket, fam);
            else       paintSoilCell(g, socket);
        }
        g.setColour(Palette::background.withAlpha(alpha::mid));
        g.drawHorizontalLine(tieTop, 0.0f, (float) gridWidth());
    }

    void applyArp(const juce::MouseEvent& e, bool down) {
        if (grid_.arp(e.x, e.y, gridWidth(), getHeight(), down)) repaint();
    }

    void stampMenu() {
        juce::PopupMenu m;
        m.addItem(1, tr("pattern-step-grid.roll-111-offbeat-16ths", "Roll -111 (offbeat 16ths)"));
        m.addItem(2, tr("pattern-step-grid.offbeat-1-8ths", "Offbeat --1- (8ths)"));
        m.addItem(3, tr("pattern-step-grid.full-1111", "Full 1111"));
        m.addItem(5, tr("pattern-step-grid.random", "Random"));
        m.addItem(4, tr("pattern-step-grid.clear", "Clear"));
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
                        [this](int r) {
            if (r == 0) return;
            using Stamp = grids::StepGridModel::Stamp;
            const auto kind = r == 1 ? Stamp::Roll : r == 2 ? Stamp::Offbeat : r == 3 ? Stamp::Full
                            : r == 4 ? Stamp::Clear : Stamp::Random;
            JuceDice dice(juce::Random::getSystemRandom());
            grid_.stamp(kind, dice);
            repaint();
        });
    }

    void apply(const juce::MouseEvent& e, bool down) {
        if (mode_ == Mode::Bassline) applyBassline(e, down);
        else                         applyArp(e, down);
    }

    juce::Rectangle<int> columnRect(int col) const {
        const int n = std::max(1, grid_.stepCount());
        const float cw = gridWidth() / (float) n;
        return juce::Rectangle<float>(col * cw, 0.0f, cw, (float) getHeight()).getSmallestIntegerContainer();
    }
    void paintPlayhead(juce::Graphics& g) {
        if (playhead_ < 0 || playhead_ >= grid_.stepCount()) return;
        const auto col = columnRect(playhead_).toFloat();
        const auto fam = familyColour();
        g.setColour(fam.withAlpha(alpha::mist));
        g.fillRect(col);
        g.setColour(fam.brighter(0.5f).withAlpha(alpha::nearOpaque));
        g.fillRect(col.withHeight(2.0f));
    }
    void pollPlayhead() {
        const int cur = grid_.playheadStep();
        if (cur == playhead_) return;
        if (playhead_ >= 0) repaint(columnRect(playhead_));
        playhead_ = cur;
        if (playhead_ >= 0) repaint(columnRect(playhead_));
    }
    void pollBank() {
        if (mode_ != Mode::Bassline) return;
        if (grid_.followBank()) repaint();
    }

    BrickHost& host_;
    std::string name_;
    Mode mode_;
    grids::StepGridModel grid_;
    StepChanceView chance_;
    int tickerId_ = 0;
    int playhead_ = -1;
    bool dropHot_ = false;
    JuceFilePicker picker_;
};

}
