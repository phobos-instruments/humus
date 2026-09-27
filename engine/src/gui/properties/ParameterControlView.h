// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/AutomateMenu.h"
#include "gui/style/Colours.h"
#include "gui/editor/DragNumberEditor.h"
#include "gui/editor/NumberPrompt.h"
#include "gui/editor/OrganismEditor.h"
#include "gui/host/PropertiesHost.h"
#include "gui/style/IconButton.h"
#include "gui/style/LookAndFeel.h"
#include "gui/properties/ControlKindButton.h"
#include "gui/properties/MappingInspector.h"
#include "gui/properties/SourceTile.h"
#include "gui/common/Localisation.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class ParameterControlView : public juce::Component, private juce::Timer {
public:
    explicit ParameterControlView(PropertiesHost& host);

    void selectParam(const std::string& organism, const std::string& param);

    void resized() override;

    void paint(juce::Graphics& g) override { g.fillAll(Palette::background); }
    void mouseDown(const juce::MouseEvent& e) override;

private:
    static constexpr int kRowH = 30;
    static constexpr int kHeadH = 16;
    static constexpr int kMargin = 12, kListW = 170, kListGap = 8, kPanelGap = 14;
    static constexpr int kRemoveW = 28, kIndentW = 8, kGroupW = 72, kLearnW = 64, kAddW = 28;
    static constexpr int kGroupH = 24, kLineH = 34, kGroupGap = 12;
    static constexpr int kSourceRowW = 330, kInspectorW = 500;
    static constexpr int kWindowW =
        kMargin + kListW + kListGap + kListW + kPanelGap + kSourceRowW + kPanelGap + kInspectorW + kMargin;

public:
    static constexpr int preferredWidthForTest() { return kWindowW; }
    static constexpr int sourceRowWidthForTest() { return kSourceRowW; }
    static constexpr int removeWidthWantedForTest() { return kRemoveW; }
    int sourceRowsWidthForTest() const { return sourceRows_.getWidth(); }
    bool organismMarkedForTest(const std::string& n) const { return controlledOrganisms_.count(n) > 0; }
    DragNumberEditor* sourceFieldForTest(int row, int which);
    std::string selectedParamForTest() const { return selectedParam(); }
    void selectOrganismForTest(const std::string& name) {
        for (size_t i = 0; i < names_.size(); ++i)
            if (names_[i] == name) organisms_.selectRow((int) i);
    }
    int removeWidthForTest() const {
        int narrowest = kRemoveW;
        for (const auto& r : rows_)
            if (r.remove) narrowest = std::min(narrowest, r.remove->getWidth());
        return narrowest;
    }
    int sourceRowCountForTest() const { return (int) rows_.size(); }
    juce::String rowKindTextForTest(int row) const {
        return row >= 0 && row < (int) rows_.size() ? rows_[(size_t) row].tile->modeForTest() : juce::String();
    }
    juce::String rowNameForTest(int row) const {
        return row >= 0 && row < (int) rows_.size() ? rows_[(size_t) row].tile->nameForTest() : juce::String();
    }
    juce::String rowMessageTextForTest(int row) {
        if (row < 0 || row >= (int) rows_.size()) return {};
        selectSource(row);
        return mapping_.message.isVisible() ? mapping_.message.getButtonText() : juce::String();
    }
    bool rowShowsHeldForTest(int row) const {
        return row >= 0 && row < (int) rows_.size() && !rows_[(size_t) row].source.held.empty();
    }
    void selectSourceForTest(int row) { selectSource(row); }
    MappingInspector& mappingForTest() { return mapping_; }
    void refreshLiveForTest() { updateLive(); }
    bool paramMarkedForTest(const std::string& param) const { return isMapped(selectedOrganism(), param); }
    std::string rowAimForTest(int row) const {
        if (row < 0 || row >= (int) rows_.size()) return {};
        if (!selectedIsRange()) return {};
        return endName(rows_[(size_t) row].end).toStdString();
    }
    void learnGroupForTest(int group) { learnInto(group); }
    void setModeForTest(RangeMode mode) { mode_.setSelectedId(idOfMode(mode), juce::sendNotificationSync); }
    bool hiddenMappingsForTest() const { return hiddenMappings(); }
    void refreshForTest() { rebuildSources(); }
    bool learnAimsAtForTest(const std::string& param) const;
    int groupHeadCountForTest() const;
    bool removeRowForTest(int row);
    void addCcForTest(int group, int cc) { addManualCc(group, cc); }
    juce::Rectangle<int> rowWordForTest(int row, int which);
    std::string rowUnitWordForTest(int row) {
        if (rowWordForTest(row, 2).isEmpty()) return {};
        return mapping_.unitWordText().toStdString();
    }

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
        void selectedRowsChanged(int) override { owner->paramChosen(); }
    };

    void paintRow(juce::Graphics& g, int w, int h, bool sel, const std::string& text, bool mapped);

    std::string selectedOrganism() const;
    std::string selectedParam() const;

    bool isMapped(const std::string& c, const std::string& p) const;

    void markControlledOrganisms();

    void rebuildOrganisms();

    void organismSelected();
    void paramChosen();

    struct SourceRow {
        std::unique_ptr<SourceTile> tile;
        std::unique_ptr<IconButton> remove;
        juce::String name;
        double min = 0.0, max = 1.0;
        bool bounded = false;
        bool isOsc = false;
        bool isMod = false;
        bool isWaiting = false;
        RangeEnd end = RangeEnd::Whole;
        MidiSource source;
        std::string oscAddress;
        std::string modSource, modValue;
        ControlShape shape;
    };

    bool selectedIsRange() const;
    static RangeMode modeOfId(int id);
    static int idOfMode(RangeMode mode);
    void normaliseEndBounds();
    std::string aimedParam(RangeEnd end) const;
    static juce::String endName(RangeEnd end);
    static constexpr RangeEnd kEnds[3] = {RangeEnd::Whole, RangeEnd::Low, RangeEnd::High};

    std::vector<RangeEnd> shownEnds() const;
    bool hiddenMappings() const;
    void learnInto(int group);
    void showAddMenu(int group);
    void reaimRow(const SourceRow& r, RangeEnd to);
    void removeRow(const SourceRow& r);
    void showRowMenu(int row);
    int rowOfComponent(juce::Component* c) const;
    void addWaitingRow(RangeEnd end);

    void later(std::function<void()> fn);

    void rebuildSources();
    juce::String deviceLabel(const MidiSource& src) const;
    void chooseDevice(const MidiSource& src, double min, double max, const ControlShape& shape,
                      RangeEnd end);
    void chooseMessage(const MidiSource& src, double min, double max, const ControlShape& shape,
                       RangeEnd end);
    void chooseChannel(const MidiSource& src, double min, double max, const ControlShape& shape,
                       RangeEnd end);
    void remapMidi(const MidiSource& from, const MidiSource& to, double min, double max,
                   const ControlShape& shape, RangeEnd end);
    static ControlFamily familyOf(const SourceRow& r);

    Unit rowUnit() const { return paramUnit(host_, selectedOrganism(), selectedParam()); }
    std::pair<double, double> rowSpan() const;

    juce::String numText(double v) const;
    double numValue(const juce::String& text) const;

    static juce::String rangeChars();

    bool sameAsMapped(DragNumberEditor* ed, double value) const;
    std::pair<int, int> focusedField() const;

    SourceRow& newRow(const juce::String& name, RangeEnd end, const ControlShape& shape, bool learning);
    static juce::String kindWord(const ControlShape& shape);
    static juce::String midiName(const MidiSource& src, const ControlShape& shape);
    InspectedSource inspected(const SourceRow& r) const;
    void wireInspector(const SourceRow& r);
    void updateLive();

    void addCcRow(const MidiSource& src, double min, double max, const ControlShape& shape,
                  RangeEnd end, bool bounded);

    void addModRow(const std::string& source, const std::string& value, double min, double max,
                   const ControlShape& shape, RangeEnd end, bool bounded);

    void addOscRow(const std::string& address, const ControlShape& shape, RangeEnd end);

    void selectSource(int idx);

    void applyShape(const ControlShape& sh);

    void layoutSourceRows();
    int layoutOneRow(SourceRow& r, int y, bool first, bool named, int group);
    int sourceRowsHeight() const;

    void addManualCc(int group, int cc);

    void rebuildModSourceBox();

    void addModRoute(int group, int choice);

    juce::String signature() const;
    void timerCallback() override;

    PropertiesHost& host_;
    OrganismListModel organismModel_;
    ParamListModel paramModel_;
    juce::ListBox organisms_, params_;
    std::vector<std::string> names_, paramNames_;
    std::set<std::string> controlledOrganisms_;

    juce::Label sourcesTitle_, hint_;
    std::string shownParam_;
    juce::Label totalRange_, modeLabel_;
    juce::ComboBox mode_;
    juce::Label groupHead_[3], groupEmpty_[3];
    juce::TextButton groupLearn_[3], groupAdd_[3];
    bool wasLearning_ = false;
    juce::Component sourceRows_;
    std::vector<SourceRow> rows_;
    MappingInspector mapping_;
    int selectedSource_ = -1;
    std::vector<std::pair<std::string, std::string>> modChoices_;
    juce::String mapSignature_;
    std::map<DragNumberEditor*, double> mappedValues_;
    std::map<std::string, std::string> lastParam_;
    bool switchingOrganism_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterControlView)
};

}
