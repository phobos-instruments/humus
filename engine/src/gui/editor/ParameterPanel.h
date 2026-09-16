// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <memory>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/OrganismEditor.h"
#include "gui/host/EditorHost.h"
#include "gui/editor/ParamSlider.h"

namespace hum {

class ParameterPanel : public OrganismEditor {
public:
    explicit ParameterPanel(EditorHost& host) : host_(host) {}

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

    EditorHost& host_;
    std::string organism_;
    std::vector<Row> rows_;
};

}
