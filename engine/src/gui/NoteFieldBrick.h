#pragma once
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/ParamSchema.h"
#include "gui/PolledBrick.h"
#include "gui/EngineHost.h"
#include "gui/LookAndFeel.h"
#include "gui/PianoNotePicker.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class NoteFieldBrick : public PolledBrick {
public:
    NoteFieldBrick(EngineHost& host, std::string cn, std::string param)
        : PolledBrick(host, std::move(cn), 4), param_(std::move(param)) {
        if (const auto* cm = host_.model().byName(name_))
            for (const auto& d : schemaFor(cm->classRaw))
                if (d.name == param_) { lo_ = (int) d.min; hi_ = (int) d.max; break; }
        last_ = (int) host_.liveParamValue(name_, param_);
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
        g.drawText(PianoNotePicker::noteName((int) host_.liveParamValue(name_, param_)),
                   getLocalBounds(), juce::Justification::centred);
    }

    void mouseEnter(const juce::MouseEvent&) override { hover_ = true; repaint(); }
    void mouseExit(const juce::MouseEvent&) override { hover_ = false; repaint(); }
    std::function<void(juce::Point<int>)> onPopup;

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu()) {
            if (onPopup) onPopup(e.getScreenPosition());
            return;
        }
        showNotePicker(host_, name_, param_, localAreaToGlobal(getLocalBounds()), lo_, hi_,
                       [this] { repaint(); });
    }
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& w) override {
        const int n = (int) host_.liveParamValue(name_, param_);
        const int nx = juce::jlimit(lo_, hi_, n + (w.deltaY > 0 ? 1 : -1));
        if (nx != n) { host_.editParam(name_, param_, (double) nx); repaint(); }
    }

private:
    void poll() override {
        const int v = (int) host_.liveParamValue(name_, param_);
        if (v != last_) { last_ = v; repaint(); }
    }

    std::string param_;
    int lo_ = 0, hi_ = kMidiMax, last_ = 0;
    bool hover_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoteFieldBrick)
};

}
