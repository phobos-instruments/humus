// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/bricks/PolledBrick.h"
#include "gui/editor/ParamRanges.h"
#include "gui/editor/inputs/PitchField.h"
#include "gui/editor/juce/PanelScroller.h"
#include "gui/host/BrickHost.h"
#include "gui/pianoroll/PianoNotePicker.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

class PitchFieldBrick : public PolledBrick {
public:
    PitchFieldBrick(BrickHost& host, std::string cn, std::string param)
        : PolledBrick(host, cn, 4), param_(std::move(param)) {}

    void reloadValues() override { repaint(); }
    void refreshAutomatedValues() override { repaint(); }
    int preferredContentWidth() const override { return 50; }
    int preferredContentHeight(int) const override { return 22; }
    std::function<void(juce::Point<int>)> onPopup;

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat();
        g.setColour(hover_ ? Palette::panelLight.brighter(0.18f) : Palette::panelLight);
        g.fillRoundedRectangle(r, 4.0f);
        g.setColour(Palette::border);
        g.drawRoundedRectangle(r.reduced(0.5f), 4.0f, 1.0f);
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(11.5f));
        g.drawText(juce::String(pitchfield::label(hz())), getLocalBounds(), juce::Justification::centred);
    }

    void mouseEnter(const juce::MouseEvent&) override { hover_ = true; repaint(); }
    void mouseExit(const juce::MouseEvent&) override { hover_ = false; repaint(); }

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu()) {
            if (onPopup) onPopup(e.getScreenPosition());
            return;
        }
        const auto range = paramRange(host_, name_, param_);
        auto picker = std::make_unique<PianoNotePicker>(
            pitchfield::nearestNote(hz()), pitchfield::lowestNote(range.first), pitchfield::highestNote(range.second),
            [this](int note) {
                host_.editParam(name_, param_, midiToHz((double) note));
                repaint();
            },
            &host_.midi());
        juce::CallOutBox::launchAsynchronously(std::move(picker), localAreaToGlobal(getLocalBounds()), nullptr);
    }

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override {
        if (PanelScroller::takesWheel(*this, e, w)) return;
        const auto range = paramRange(host_, name_, param_);
        host_.editParam(name_, param_, pitchfield::stepped(hz(), w.deltaY > 0 ? 1 : -1, range.first, range.second));
        repaint();
    }

private:
    double hz() const { return host_.liveParamValue(name_, param_); }

    void poll() override {
        const double now = hz();
        if (now == last_) return;
        last_ = now;
        repaint();
    }

    std::string param_;
    double last_ = -1.0;
    bool hover_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchFieldBrick)
};

}
