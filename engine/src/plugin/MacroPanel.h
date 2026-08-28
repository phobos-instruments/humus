#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "core/ParamSchema.h"
#include "plugin/FxLook.h"
#include "plugin/HumusProcessor.h"

namespace hum {

class MacroPanel : public juce::Component {
public:
    explicit MacroPanel(HumusProcessor& p) : proc_(p) {
        header_.setText("DAW automation: map a Macro, automate it from the host",
                        juce::dontSendNotification);
        header_.setFont(juce::FontOptions(11.0f));
        header_.setColour(juce::Label::textColourId, fxlook::dim());
        addAndMakeVisible(header_);
        for (int i = 0; i < HumusProcessor::kNumMacros; ++i) {
            label_[i].setText("M" + juce::String(i + 1), juce::dontSendNotification);
            label_[i].setColour(juce::Label::textColourId, fxlook::accent());
            label_[i].setFont(juce::FontOptions(11.0f));
            addAndMakeVisible(label_[i]);

            slider_[i].setSliderStyle(juce::Slider::LinearHorizontal);
            slider_[i].setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
            slider_[i].setRange(0.0, 1.0, 0.0);
            fxlook::styleSlider(slider_[i]);
            slider_[i].onValueChange = [this, i] {
                proc_.setMacroValue(i, (float) slider_[i].getValue());
            };
            addAndMakeVisible(slider_[i]);

            organismBox_[i].setTextWhenNothingSelected("(none)");
            organismBox_[i].onChange = [this, i] { onOrganismPicked(i); };
            addAndMakeVisible(organismBox_[i]);

            paramBox_[i].setTextWhenNothingSelected("(param)");
            paramBox_[i].onChange = [this, i] { onParamPicked(i); };
            addAndMakeVisible(paramBox_[i]);
        }
    }

    void refresh() {
        const auto& model = proc_.model();
        for (int i = 0; i < HumusProcessor::kNumMacros; ++i) {
            const auto map = proc_.macroMapping(i);
            organismBox_[i].clear(juce::dontSendNotification);
            int sel = 0;
            for (int n = 0; n < (int) model.organisms.size(); ++n) {
                const auto& nm = model.organisms[(size_t) n].name;
                organismBox_[i].addItem(nm, n + 1);
                if (nm == map.first) sel = n + 1;
            }
            organismBox_[i].setSelectedId(sel, juce::dontSendNotification);
            populateParams(i);
        }
    }

    void pullValues() {
        for (int i = 0; i < HumusProcessor::kNumMacros; ++i)
            if (!slider_[i].isMouseButtonDown())
                slider_[i].setValue(proc_.macroValue(i), juce::dontSendNotification);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(8, 4);
        header_.setBounds(r.removeFromTop(16));
        for (int i = 0; i < HumusProcessor::kNumMacros; ++i) {
            auto row = r.removeFromTop(21).reduced(0, 1);
            label_[i].setBounds(row.removeFromLeft(26));
            slider_[i].setBounds(row.removeFromLeft(90));
            row.removeFromLeft(4);
            const int half = (row.getWidth() - 4) / 2;
            organismBox_[i].setBounds(row.removeFromLeft(half));
            row.removeFromLeft(4);
            paramBox_[i].setBounds(row);
        }
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(fxlook::panel());
        g.setColour(fxlook::line());
        g.drawRect(getLocalBounds());
    }

private:
    void populateParams(int i) {
        paramBox_[i].clear(juce::dontSendNotification);
        const auto map = proc_.macroMapping(i);
        const int cid = organismBox_[i].getSelectedId();
        const auto& model = proc_.model();
        if (cid < 1 || cid > (int) model.organisms.size()) return;
        const auto& cm = model.organisms[(size_t) (cid - 1)];
        int n = 1, sel = 0;
        for (const auto& d : schemaFor(cm.displayClass)) {
            paramBox_[i].addItem(d.name, n);
            if (d.name == map.second) sel = n;
            ++n;
        }
        paramBox_[i].setSelectedId(sel, juce::dontSendNotification);
    }

    void onOrganismPicked(int i) {
        const int cid = organismBox_[i].getSelectedId();
        const auto& model = proc_.model();
        std::string c;
        if (cid >= 1 && cid <= (int) model.organisms.size())
            c = model.organisms[(size_t) (cid - 1)].name;
        proc_.setMacroMapping(i, c, "");
        populateParams(i);
    }

    void onParamPicked(int i) {
        const int cid = organismBox_[i].getSelectedId();
        const auto& model = proc_.model();
        if (cid < 1 || cid > (int) model.organisms.size()) return;
        const auto& cm = model.organisms[(size_t) (cid - 1)];
        proc_.setMacroMapping(i, cm.name, paramBox_[i].getText().toStdString());
    }

    HumusProcessor& proc_;
    juce::Label header_;
    juce::Label label_[HumusProcessor::kNumMacros];
    juce::Slider slider_[HumusProcessor::kNumMacros];
    juce::ComboBox organismBox_[HumusProcessor::kNumMacros];
    juce::ComboBox paramBox_[HumusProcessor::kNumMacros];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroPanel)
};

}
