// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>

#include "gui/common/Localisation.h"
#include "gui/editor/DawMacroMenu.h"
#include "gui/style/Colours.h"
#include "plugin/HumusProcessor.h"

namespace hum {

class DawMacroStrip : public juce::Component {
public:
    explicit DawMacroStrip(HumusProcessor& p) : proc_(p) {
        for (int i = 0; i < DawBridge::kControls; ++i) {
            auto& cell = cells_[(size_t) i];
            cell.knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            cell.knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            cell.attachment = std::make_unique<juce::SliderParameterAttachment>(p.macro(i), cell.knob);
            addAndMakeVisible(cell.knob);
            cell.name.setJustificationType(juce::Justification::centredLeft);
            cell.name.setFont(juce::FontOptions(11.0f));
            addAndMakeVisible(cell.name);
        }
        refresh();
    }

    void refresh() {
        for (int i = 0; i < DawBridge::kControls; ++i) {
            auto& cell = cells_[(size_t) i];
            const auto targets = proc_.host().dawControlTargets(i);
            const auto n = juce::String(i + 1);
            const bool mapped = !targets.empty();
            auto text = "M" + n;
            juce::StringArray names;
            for (const auto& t : targets) names.add(dawMacroTargetName(t));
            if (mapped) text << "  " << names[0];
            if (names.size() > 1) text << " +" << juce::String(names.size() - 1);
            if (cell.name.getText() != text) cell.name.setText(text, juce::dontSendNotification);
            cell.name.setColour(juce::Label::textColourId, mapped ? Palette::text : Palette::textDim);
            cell.knob.setTooltip(mapped ? tr("plugin-macros.drives-tip", "Macro M{n} drives {targets}; the DAW can automate it",
                                             {{"n", n}, {"targets", names.joinIntoString(", ")}})
                                        : tr("plugin-macros.empty-tip",
                                             "Right-click any control and choose Automate from the DAW to give it Macro M{n}",
                                             {{"n", n}}));
        }
    }

    void paint(juce::Graphics& g) override {
        g.setColour(Palette::panel);
        g.fillRect(getLocalBounds());
        g.setColour(Palette::border);
        g.fillRect(getLocalBounds().removeFromTop(1));
    }

    void resized() override {
        auto r = getLocalBounds().reduced(8, 4);
        const int w = r.getWidth() / DawBridge::kControls;
        for (auto& cell : cells_) {
            auto c = r.removeFromLeft(w).reduced(2, 0);
            cell.knob.setBounds(c.removeFromLeft(c.getHeight()));
            cell.name.setBounds(c);
        }
    }

private:
    struct Cell {
        juce::Slider knob;
        juce::Label name;
        std::unique_ptr<juce::SliderParameterAttachment> attachment;
    };

    HumusProcessor& proc_;
    std::array<Cell, DawBridge::kControls> cells_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DawMacroStrip)
};

}
