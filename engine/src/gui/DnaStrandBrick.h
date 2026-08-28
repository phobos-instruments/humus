#pragma once
#include <cmath>
#include <cstdint>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/EngineHost.h"
#include "gui/LookAndFeel.h"
#include "gui/PolledBrick.h"

namespace hum {

class DnaStrandBrick : public PolledBrick, public juce::SettableTooltipClient {
public:
    static constexpr int kSteps = 16;

    DnaStrandBrick(EngineHost& host, std::string cn) : PolledBrick(host, std::move(cn)) {
        setTooltip("Click a step to silence it; drag to sweep. Right-click for the whole strip");
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

    static bool stepRests(long idx, int seed, double rest) {
        std::uint32_t h = (std::uint32_t) (idx * 2246822519u)
                        ^ (std::uint32_t) (seed * 374761393u);
        h ^= h >> 16;
        h *= 0x85ebca6bu;
        h ^= h >> 13;
        h *= 0xc2b2ae35u;
        h ^= h >> 16;
        return (h & 0xFFFF) / 65536.0 < rest;
    }

    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        const float cellW = r.getWidth() / kSteps;
        const long abs = absoluteStep();
        const int cur = abs < 0 ? -1 : (int) (abs % kSteps);
        const long lap = abs < 0 ? 0 : abs - abs % kSteps;
        const int seed = (int) host_.liveParamValue(name_, "Seed");
        const double rest = host_.liveParamValue(name_, "Rest");
        const int mute = mask();
        for (int k = 0; k < kSteps; ++k) {
            auto cell = juce::Rectangle<float>(r.getX() + k * cellW, r.getY(),
                                               cellW - 2.0f, r.getHeight());
            const bool rests = abs >= 0 && stepRests(lap + k, seed, rest);
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
    static constexpr int kAll = (1 << kSteps) - 1;

    int cellAt(float x) const {
        return juce::jlimit(0, kSteps - 1, (int) (x * kSteps / juce::jmax(1, getWidth())));
    }
    int mask() const { return (int) host_.liveParamValue(name_, "Mute"); }
    void setMask(int m) {
        host_.editParam(name_, "Mute", (double) (m & kAll));
        repaint();
    }

    void stripMenu() {
        juce::PopupMenu m;
        m.addItem(1, "Play every step", mask() != 0);
        m.addItem(2, "Invert");
        m.addItem(3, "Silence the off-beats");
        m.showMenuAsync(juce::PopupMenu::Options(),
                        [safe = juce::Component::SafePointer<DnaStrandBrick>(this)](int r) {
                            if (safe == nullptr || r == 0) return;
                            if (r == 1) safe->setMask(0);
                            if (r == 2) safe->setMask(~safe->mask());
                            if (r == 3) safe->setMask(0xAAAA);
                        });
    }

    static constexpr double kStepsPerBeat[12] = {4.0, 2.0, 8.0, 3.0, 6.0, 4.0 / 3.0,
                                                 1.0, 8.0 / 3.0, 1.5, 0.5, 0.25, 16.0};

    long absoluteStep() {
        if (!host_.isPlaying()) return -1;
        const int rate = juce::jlimit(0, 11, (int) host_.liveParamValue(name_, "Rate"));
        return (long) std::floor(host_.positionBeats() * kStepsPerBeat[rate]);
    }

    void poll() override {
        struct Sig { long step; int seed; double rest; int mute; };
        const Sig s{absoluteStep(), (int) host_.liveParamValue(name_, "Seed"),
                    host_.liveParamValue(name_, "Rest"), mask()};
        if (s.step != lastStep_ || s.seed != lastSeed_ || s.rest != lastRest_
            || s.mute != lastMute_) {
            lastStep_ = s.step;
            lastSeed_ = s.seed;
            lastRest_ = s.rest;
            lastMute_ = s.mute;
            repaint();
        }
    }

    long lastStep_ = -2;
    int lastSeed_ = 0;
    double lastRest_ = -1.0;
    int lastMute_ = -1;
    bool silencing_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DnaStrandBrick)
};

}
