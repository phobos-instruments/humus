#pragma once
#include <memory>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/OrganismEditor.h"
#include "gui/EngineHost.h"
#include "gui/ParamSlider.h"

namespace hum {

class ParameterPanel : public OrganismEditor {
public:
    explicit ParameterPanel(EngineHost& host) : host_(host) {}

    void show(const std::string& organismName);
    int contentRows() const { return (int) rows_.size(); }
    void resized() override;
    void paint(juce::Graphics&) override;

    void reloadValues() override { show(organism_); }
    void refreshAutomatedValues() override;
    int preferredContentWidth() const override { return 280; }
    int preferredContentHeight(int width) const override;

private:
    static constexpr int kCellW = 84;
    int labelLines() const;
    int labelHeight() const;
    int cellHeight() const;

    void showParamMenu(const std::string& param, juce::Point<int> screenPos);

    struct Row {
        std::string param;
        std::unique_ptr<juce::Label> label;
        std::unique_ptr<ParamSlider> slider;
        std::unique_ptr<juce::ToggleButton> toggle;
        std::unique_ptr<juce::TextEditor> text;
    };

    EngineHost& host_;
    std::string organism_;
    std::vector<Row> rows_;
};

}
