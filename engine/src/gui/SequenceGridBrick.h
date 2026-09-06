#pragma once
#include <cmath>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/MidiFormat.h"
#include "core/Randomize.h"
#include "gui/EngineHost.h"
#include "gui/FineDrag.h"
#include "gui/LookAndFeel.h"
#include "gui/PianoNotePicker.h"
#include "hum/Pattern.h"
#include "hum/PatternMatrix.h"
#include "gui/Localisation.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class SequenceGridBrick : public juce::Component, private juce::Timer {
public:
    static constexpr int kRows = 8, kSteps = 16;

    SequenceGridBrick(EngineHost& host, std::string organism)
        : host_(host), cn_(std::move(organism)) {
        host_.patterns().ensureBanks(cn_, kRows, kPatternBanks);
        for (int r = 0; r < kRows; ++r) {
            auto& k = vel_[(size_t) r];
            k.setSliderStyle(juce::Slider::RotaryVerticalDrag);
            k.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
            k.setRange(0.0, 1.0, 0.0);
            applyFineCrawl(k);
            k.setValue(host_.liveParamValue(cn_, param("Vel_", r)),
                       juce::dontSendNotification);
            k.onValueChange = [this, r] {
                host_.editParam(cn_, param("Vel_", r), vel_[(size_t) r].getValue());
            };
            addAndMakeVisible(k);
        }
        startTimerHz(30);
    }

    void paint(juce::Graphics& g) override {
        for (int r = 0; r < kRows; ++r) {
            const auto row = rowBounds(r);
            const bool en = host_.liveParamValue(cn_, param("Enable_", r)) >= 0.5;
            const auto led = ledRect(r).toFloat();
            g.setColour(en ? juce::Colour(0xff7ec44a) : Palette::panelLight);
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
            g.drawText(targetName(r), plate.reduced(7.0f, 0.0f).toNearestInt(),
                       juce::Justification::centredLeft);
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(9.5f));
            g.drawText(midiNoteName((int) host_.liveParamValue(cn_, param("Note_", r))),
                       plate.reduced(5.0f, 0.0f).toNearestInt(),
                       juce::Justification::centredRight);
            const auto ticks = rowTriggers(r);
            for (int s = 0; s < kSteps; ++s) {
                const auto cell = cellRect(r, s).toFloat();
                const bool on = std::find(ticks.begin(), ticks.end(), s * kStepTicks)
                                != ticks.end();
                const bool darkGroup = ((s / 4) & 1) != 0;
                juce::Colour c = on ? Palette::accent
                                    : darkGroup ? Palette::panelLight.darker(0.25f)
                                                : Palette::panelLight;
                if (playStep_ == s && host_.isPlaying()) c = c.brighter(on ? 0.35f : 0.12f);
                if (!en) c = c.withAlpha(0.45f);
                g.setColour(c);
                g.fillRoundedRectangle(cell, 3.0f);
                g.setColour(Palette::border.withAlpha(0.7f));
                g.drawRoundedRectangle(cell, 3.0f, 1.0f);
            }
            juce::ignoreUnused(row);
        }
    }

    void resized() override {
        for (int r = 0; r < kRows; ++r) vel_[(size_t) r].setBounds(knobRect(r));
    }

    void mouseDown(const juce::MouseEvent& e) override {
        for (int r = 0; r < kRows; ++r) {
            if (ledRect(r).contains(e.getPosition())) {
                const bool en = host_.liveParamValue(cn_, param("Enable_", r)) >= 0.5;
                host_.editParam(cn_, param("Enable_", r), en ? 0.0 : 1.0);
                repaint();
                return;
            }
            if (plateRect(r).contains(e.getPosition())) {
                auto anchor = localAreaToGlobal(plateRect(r));
                showNotePicker(host_, cn_, param("Note_", r), anchor, 0, kMidiMax,
                               [this] { repaint(); });
                return;
            }
            const int s = stepAt(r, e.getPosition());
            if (s >= 0) {
                if (e.mods.isRightButtonDown()) { bankMenu(); return; }
                host_.pushUndo();
                const auto ticks = rowTriggers(r);
                painting_ = true;
                paintOn_ = std::find(ticks.begin(), ticks.end(), s * kStepTicks)
                           == ticks.end();
                applyCell(r, s);
                return;
            }
        }
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        if (!painting_) return;
        for (int r = 0; r < kRows; ++r)
            if (const int s = stepAt(r, e.getPosition()); s >= 0) applyCell(r, s);
    }
    void mouseUp(const juce::MouseEvent&) override { painting_ = false; }

    void mouseWheelMove(const juce::MouseEvent& e,
                        const juce::MouseWheelDetails& wheel) override {
        for (int r = 0; r < kRows; ++r)
            if (plateRect(r).contains(e.getPosition())) {
                const int note = (int) host_.liveParamValue(cn_, param("Note_", r));
                const int next = juce::jlimit(0, kMidiMax, note + (wheel.deltaY > 0 ? 1 : -1));
                if (next != note) host_.editParam(cn_, param("Note_", r), (double) next);
                repaint();
                return;
            }
    }

private:
    static constexpr int kStepTicks = Pattern::kTicksPerBeat / 4;
    static constexpr int kRowH = 28, kRowGap = 3;
    static constexpr int kLedW = 14, kKnobW = 26, kPlateW = 116, kHeadGap = 6;

    static std::string param(const char* prefix, int row) {
        return prefix + std::to_string(row + 1);
    }

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

    std::vector<int> rowTriggers(int r) const {
        const auto lanes = host_.patterns().triggerLanes(cn_);
        return r < (int) lanes.size() ? lanes[(size_t) r]->triggers : std::vector<int>{};
    }

    static juce::String bankLetter(int bank) { return juce::String::charToString((juce::juce_wchar) ('A' + bank)); }

    void bankMenu() {
        const int cur = host_.patterns().bank(cn_);
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
            host_.pushUndo();
            EngineHost::PatternSyncBatch batch(host_);
            auto& rng = juce::Random::getSystemRandom();
            for (int row = 0; row < kRows; ++row) {
                std::vector<int> ticks;
                if (r == 1) {
                    const auto cells = randomTriggerRow(kSteps, 0.10 + rng.nextDouble() * 0.35, rng);
                    for (int s = 0; s < kSteps; ++s) if (cells[(size_t) s]) ticks.push_back(s * kStepTicks);
                } else if (r >= 10) {
                    ticks = rowTriggers(row);
                }
                host_.patterns().setLaneTriggers(cn_, r >= 10 ? r - 10 : cur, row, ticks);
            }
            repaint();
        });
    }

    void applyCell(int r, int s) {
        const int tick = s * kStepTicks;
        const auto ticks = rowTriggers(r);
        const bool has = std::find(ticks.begin(), ticks.end(), tick) != ticks.end();
        if (paintOn_ && !has) host_.patterns().addTrigger(cn_, r, tick);
        else if (!paintOn_ && has) host_.patterns().removeTrigger(cn_, r, tick);
        else return;
        repaint();
    }

    std::string targetName(int r) const {
        std::string name;
        for (const auto& c : host_.model().midiConnections)
            if (c.src == cn_ && c.srcOutlet == r) {
                if (name.empty()) name = c.dst;
                else name += " +";
            }
        return name.empty() ? "(row " + std::to_string(r + 1) + ")" : name;
    }

    void timerCallback() override {
        int step = -1;
        if (host_.isPlaying()) {
            const auto* cm = host_.model().byName(cn_);
            const int dur = cm && cm->pattern.present ? cm->pattern.duration : 0;
            if (dur > 0) {
                const double tick =
                    std::fmod(host_.positionBeats() * Pattern::kTicksPerBeat, (double) dur);
                step = (int) (tick / kStepTicks) % kSteps;
            }
        }
        const int bank = host_.patterns().bank(cn_);
        if (step != playStep_ || bank != bank_) {
            playStep_ = step;
            bank_ = bank;
            repaint();
        }
    }

    EngineHost& host_;
    std::string cn_;
    juce::Slider vel_[kRows];
    bool painting_ = false;
    bool paintOn_ = true;
    int playStep_ = -1;
    int bank_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequenceGridBrick)
};

}
