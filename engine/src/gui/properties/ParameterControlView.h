// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/AutomateMenu.h"
#include "gui/style/Colours.h"
#include "gui/editor/DragNumberEditor.h"
#include "gui/editor/OrganismEditor.h"
#include "gui/host/PropertiesHost.h"
#include "gui/style/LookAndFeel.h"
#include "gui/properties/MappingShapePanel.h"
#include "gui/common/Localisation.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class ParameterControlView : public juce::Component, private juce::Timer {
public:
    explicit ParameterControlView(PropertiesHost& host);

    void selectParam(const std::string& organism, const std::string& param);

    void resized() override;

    void paint(juce::Graphics& g) override { g.fillAll(Palette::background); }

private:
    static constexpr int kRowH = 28;
    static constexpr int kMargin = 10, kListW = 180, kListGap = 8, kPanelGap = 12;
    static constexpr int kKindW = 60, kHeldW = 110, kCcW = 44, kBoundW = 74, kRemoveW = 64;
    static constexpr int kSourceRowW =
        kKindW + kHeldW + 4 + kCcW + 8 + kBoundW + 4 + kBoundW + 10 + kRemoveW;
    static constexpr int kWindowW =
        kMargin + kListW + kListGap + kListW + kPanelGap + kSourceRowW + kMargin;

public:
    static constexpr int preferredWidthForTest() { return kWindowW; }
    static constexpr int sourceRowWidthForTest() { return kSourceRowW; }
    int sourceRowsWidthForTest() const { return sourceRows_.getWidth(); }
    bool organismMarkedForTest(const std::string& n) const { return controlledOrganisms_.count(n) > 0; }
    DragNumberEditor* sourceFieldForTest(int row, int which);

private:

    struct OrganismListModel : juce::ListBoxModel {
        ParameterControlView* owner = nullptr;
        int getNumRows() override { return (int) owner->names_.size(); }
        void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool sel) override {
            if (row >= (int) owner->names_.size()) return;
            const auto& n = owner->names_[(size_t) row];
            owner->paintRow(g, w, h, sel, targetOwnerLabel(owner->host_, n).toStdString(),
                            owner->controlledOrganisms_.count(n) > 0);
        }
        void selectedRowsChanged(int) override { owner->organismSelected(); }
    };
    struct ParamListModel : juce::ListBoxModel {
        ParameterControlView* owner = nullptr;
        int getNumRows() override { return (int) owner->paramNames_.size(); }
        void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool sel) override {
            if (row >= (int) owner->paramNames_.size()) return;
            const auto& p = owner->paramNames_[(size_t) row];
            owner->paintRow(g, w, h, sel, p, owner->isMapped(owner->selectedOrganism(), p));
        }
        void selectedRowsChanged(int) override { owner->rebuildSources(); }
    };

    void paintRow(juce::Graphics& g, int w, int h, bool sel, const std::string& text, bool mapped);

    std::string selectedOrganism() const;
    std::string selectedParam() const;

    bool isMapped(const std::string& c, const std::string& p) const;

    void markControlledOrganisms();

    void rebuildOrganisms();

    void organismSelected();

    struct SourceRow {
        std::unique_ptr<juce::TextButton> kind;
        std::unique_ptr<DragNumberEditor> cc, min, max;
        std::unique_ptr<juce::Label> address, held;
        std::unique_ptr<juce::TextButton> remove;
        bool isOsc = false;
        bool isMod = false;
        MidiSource source;
        std::string oscAddress;
        std::string modSource, modValue;
        ControlShape shape;
    };

    void later(std::function<void()> fn);

    void rebuildSources();

    Unit rowUnit() const { return paramUnit(host_, selectedOrganism(), selectedParam()); }
    std::pair<double, double> rowSpan() const;

    juce::String numText(double v) const;
    double numValue(const juce::String& text) const;

    static juce::String rangeChars();

    std::unique_ptr<DragNumberEditor> numberCell(const juce::String& value, const juce::String& allowed);

    std::unique_ptr<juce::TextButton> kindButton(const juce::String& text);

    void addCcRow(const MidiSource& src, double min, double max, const ControlShape& shape);

    void addModRow(const std::string& source, const std::string& value, double min, double max, const ControlShape& shape);

    void addOscRow(const std::string& address, const ControlShape& shape);

    void selectSource(int idx);

    void applyShape(const ControlShape& sh);

    void layoutSourceRows();

    void addManualCc();

    void capture();

    void rebuildModSourceBox(bool enable);

    void addModRoute();

    juce::String signature() const;
    void timerCallback() override;

    PropertiesHost& host_;
    OrganismListModel organismModel_;
    ParamListModel paramModel_;
    juce::ListBox organisms_, params_;
    std::vector<std::string> names_, paramNames_;
    std::set<std::string> controlledOrganisms_;

    juce::Label sourcesTitle_, addCcLabel_, addModLabel_, hint_;
    juce::Component sourceRows_;
    std::vector<SourceRow> rows_;
    MappingShapePanel mapping_;
    int selectedSource_ = -1;
    juce::TextEditor addCcEdit_;
    juce::TextButton addBtn_, captureBtn_, addModBtn_;
    juce::ComboBox modSourceBox_;
    std::vector<std::pair<std::string, std::string>> modChoices_;
    juce::String mapSignature_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterControlView)
};

}
