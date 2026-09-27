// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/midi/MidiFormat.h"
#include "gui/editor/inputs/NumberInputs.h"
#include "gui/bricks/PolledBrick.h"
#include "gui/editor/juce/PanelScroller.h"
#include "gui/host/BrickHost.h"
#include "gui/style/LookAndFeel.h"
#include "gui/pianoroll/PianoNotePicker.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class NoteFieldBrick : public PolledBrick {
public:
    static constexpr int kPitchClasses = 12;
    static constexpr int kMiddleOctave = 60;

    NoteFieldBrick(BrickHost& host, std::string cn, std::string param, bool pitchClass = false)
        : PolledBrick(host, cn, 4), param_(std::move(param)), pitchClass_(pitchClass),
          note_(host, cn, param_, 0, pitchClass ? kPitchClasses - 1 : kMidiMax) {}

    static int storedNote(int picked, bool pitchClass) {
        return pitchClass ? ((picked % kPitchClasses) + kPitchClasses) % kPitchClasses : picked;
    }

    void reloadValues() override { repaint(); }
    void refreshAutomatedValues() override { repaint(); }
    int preferredContentWidth() const override { return 90; }
    int preferredContentHeight(int) const override { return 22; }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat();
        g.setColour(hover_ ? Palette::panelLight.brighter(0.18f) : Palette::panelLight);
        g.fillRoundedRectangle(r, 4.0f);
        g.setColour(Palette::border);
        g.drawRoundedRectangle(r.reduced(0.5f), 4.0f, 1.0f);
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(13.0f));
        g.drawText(shownName(), getLocalBounds(), juce::Justification::centred);
    }

    void mouseEnter(const juce::MouseEvent&) override { hover_ = true; repaint(); }
    void mouseExit(const juce::MouseEvent&) override { hover_ = false; repaint(); }
    std::function<void(juce::Point<int>)> onPopup;

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu()) {
            if (onPopup) onPopup(e.getScreenPosition());
            return;
        }
        if (pitchClass_)
            pickPitchClass();
        else
            showNotePicker(host_, name_, param_, localAreaToGlobal(getLocalBounds()), note_.lowest(), note_.highest(),
                           [this] { repaint(); });
    }
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override {
        if (PanelScroller::takesWheel(*this, e, w)) return;
        if (note_.step(w.deltaY > 0 ? 1 : -1)) repaint();
    }

private:
    juce::String shownName() const {
        const int n = note_.note();
        if (pitchClass_) return pitchClassName(storedNote(n, true));
        return midiNoteName(juce::jlimit(0, kMidiMax, n));
    }

    void pickPitchClass() {
        auto picker = std::make_unique<PianoNotePicker>(
            kMiddleOctave + storedNote(note_.note(), true), 0, kMidiMax,
            [this](int n) {
                host_.editParam(name_, param_, (double) storedNote(n, true));
                repaint();
            },
            &host_.midi());
        juce::CallOutBox::launchAsynchronously(std::move(picker), localAreaToGlobal(getLocalBounds()), nullptr);
    }

    void poll() override {
        if (note_.poll()) repaint();
    }

    std::string param_;
    bool pitchClass_ = false;
    input::NoteInput note_;
    bool hover_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoteFieldBrick)
};

}
