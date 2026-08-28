#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/OrganismEditor.h"
#include "gui/EngineHost.h"
#include "gui/LookAndFeel.h"
#include "gui/MappingShapePanel.h"

namespace hum {

class ParameterControlView : public juce::Component, private juce::Timer {
public:
    explicit ParameterControlView(EngineHost& host)
        : host_(host), organisms_("organisms", &organismModel_),
          params_("params", &paramModel_) {
        organismModel_.owner = paramModel_.owner = this;

        auto initList = [](juce::ListBox& lb) {
            lb.setRowHeight(22);
            lb.setColour(juce::ListBox::backgroundColourId, Palette::panel);
        };
        initList(organisms_);
        initList(params_);
        addAndMakeVisible(organisms_);
        addAndMakeVisible(params_);

        sourcesTitle_.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
        addAndMakeVisible(sourcesTitle_);
        addAndMakeVisible(sourceRows_);

        mapping_.onChanged = [this](const ControlShape& sh) { applyShape(sh); };
        addAndMakeVisible(mapping_);

        addCcLabel_.setText("Add CC", juce::dontSendNotification);
        addCcLabel_.setFont(juce::FontOptions(12.0f));
        addAndMakeVisible(addCcLabel_);
        addCcEdit_.setInputRestrictions(3, "0123456789");
        addCcEdit_.setJustification(juce::Justification::centred);
        addAndMakeVisible(addCcEdit_);
        addBtn_.setButtonText("Add");
        addBtn_.onClick = [this] { addManualCc(); };
        addAndMakeVisible(addBtn_);
        captureBtn_.setButtonText("Capture next controller...");
        captureBtn_.onClick = [this] { capture(); };
        addAndMakeVisible(captureBtn_);

        addModLabel_.setText("From", juce::dontSendNotification);
        addModLabel_.setFont(juce::FontOptions(12.0f));
        addAndMakeVisible(addModLabel_);
        addAndMakeVisible(modSourceBox_);
        addModBtn_.setButtonText("Add");
        addModBtn_.onClick = [this] { addModRoute(); };
        addAndMakeVisible(addModBtn_);

        hint_.setFont(juce::FontOptions(11.5f));
        hint_.setColour(juce::Label::textColourId, Palette::textDim);
        hint_.setJustificationType(juce::Justification::topLeft);
        addAndMakeVisible(hint_);

        setSize(720, 560);
        rebuildOrganisms();
        startTimerHz(2);
    }

    void selectParam(const std::string& organism, const std::string& param) {
        rebuildOrganisms();
        for (size_t i = 0; i < names_.size(); ++i)
            if (names_[i] == organism) {
                organisms_.selectRow((int) i);
                organismSelected();
                for (size_t j = 0; j < paramNames_.size(); ++j)
                    if (paramNames_[j] == param) params_.selectRow((int) j);
                rebuildSources();
                return;
            }
    }

    void resized() override {
        auto area = getLocalBounds().reduced(10);
        organisms_.setBounds(area.removeFromLeft(180));
        area.removeFromLeft(8);
        params_.setBounds(area.removeFromLeft(180));
        area.removeFromLeft(12);

        sourcesTitle_.setBounds(area.removeFromTop(22));
        auto foot = area.removeFromBottom(84);
        mapping_.setBounds(area.removeFromBottom(168));
        sourceRows_.setBounds(area.reduced(0, 4));

        auto addRowArea = foot.removeFromTop(26);
        addCcLabel_.setBounds(addRowArea.removeFromLeft(52));
        addCcEdit_.setBounds(addRowArea.removeFromLeft(48).reduced(0, 2));
        addRowArea.removeFromLeft(6);
        addBtn_.setBounds(addRowArea.removeFromLeft(56).reduced(0, 2));
        addRowArea.removeFromLeft(12);
        captureBtn_.setBounds(addRowArea.removeFromLeft(190).reduced(0, 2));
        auto modRowArea = foot.removeFromTop(26);
        addModLabel_.setBounds(modRowArea.removeFromLeft(52));
        modSourceBox_.setBounds(modRowArea.removeFromLeft(190).reduced(0, 2));
        modRowArea.removeFromLeft(6);
        addModBtn_.setBounds(modRowArea.removeFromLeft(56).reduced(0, 2));
        foot.removeFromTop(4);
        hint_.setBounds(foot);
        layoutSourceRows();
    }

    void paint(juce::Graphics& g) override { g.fillAll(Palette::background); }

private:
    static constexpr int kRowH = 28;

    struct OrganismListModel : juce::ListBoxModel {
        ParameterControlView* owner = nullptr;
        int getNumRows() override { return (int) owner->names_.size(); }
        void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool sel) override {
            if (row >= (int) owner->names_.size()) return;
            const auto& n = owner->names_[(size_t) row];
            owner->paintRow(g, w, h, sel,
                            targetOwnerLabel(owner->host_, n).toStdString(), false);
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

    void paintRow(juce::Graphics& g, int w, int h, bool sel, const std::string& text,
                  bool mapped) {
        if (sel) g.fillAll(Palette::accent.withAlpha(0.25f));
        g.setColour(mapped ? Palette::accent : Palette::text);
        g.setFont(juce::FontOptions(12.5f));
        g.drawText((mapped ? juce::String::fromUTF8("\xe2\x97\x8f ") : juce::String("   "))
                       + juce::String(text),
                   6, 0, w - 10, h, juce::Justification::centredLeft);
    }

    std::string selectedOrganism() const {
        const int r = organisms_.getSelectedRow();
        return r >= 0 && r < (int) names_.size() ? names_[(size_t) r] : std::string();
    }
    std::string selectedParam() const {
        const int r = params_.getSelectedRow();
        return r >= 0 && r < (int) paramNames_.size() ? paramNames_[(size_t) r] : std::string();
    }

    bool isMapped(const std::string& c, const std::string& p) const {
        for (const auto& e : host_.midi().map().entries())
            if (e.organism == c && e.param == p) return true;
        for (const auto& e : host_.osc().map().entries())
            if (e.organism == c && e.param == p) return true;
        for (const auto& e : host_.mod().map().entries())
            if (e.organism == c && e.param == p) return true;
        return false;
    }

    void rebuildOrganisms() {
        const auto prev = selectedOrganism();
        names_.clear();
        for (const auto& cm : host_.model().organisms)
            if (!controlTargets(host_, cm.name).empty()) names_.push_back(cm.name);
        organisms_.updateContent();
        int sel = names_.empty() ? -1 : 0;
        for (size_t i = 0; i < names_.size(); ++i)
            if (names_[i] == prev) sel = (int) i;
        if (sel >= 0) organisms_.selectRow(sel);
        organismSelected();
    }

    void organismSelected() {
        paramNames_ = controlTargets(host_, selectedOrganism());
        params_.updateContent();
        params_.repaint();
        rebuildSources();
    }

    struct SourceRow {
        std::unique_ptr<juce::TextButton> kind;
        std::unique_ptr<juce::TextEditor> cc, min, max;
        std::unique_ptr<juce::Label> address;
        std::unique_ptr<juce::TextButton> remove;
        bool isOsc = false;
        bool isMod = false;
        int ccNum = -1;
        std::string oscAddress;
        std::string modSource, modValue;
        ControlShape shape;
    };

    void rebuildSources() {
        rows_.clear();
        sourceRows_.removeAllChildren();
        const auto c = selectedOrganism();
        const auto p = selectedParam();
        const juce::String sfx = p.empty() ? juce::String() : juce::String(unitSuffix(rowUnit()));
        sourcesTitle_.setText(p.empty() ? "Sources"
                                        : "Sources " + juce::String("- ")
                                              + targetOwnerLabel(host_, c) + " / " + juce::String(p)
                                              + (sfx.isEmpty() ? "" : "  (" + sfx + ")"),
                              juce::dontSendNotification);
        const bool enable = !p.empty();
        addCcEdit_.setEnabled(enable);
        addBtn_.setEnabled(enable);
        captureBtn_.setEnabled(enable && MidiLearner::instance().armed() == false);

        if (enable) {
            for (const auto& e : host_.midi().map().entries())
                if (e.organism == c && e.param == p) addCcRow(e.cc, e.min, e.max, e.shape);
            for (const auto& e : host_.osc().map().entries())
                if (e.organism == c && e.param == p) addOscRow(e.address, e.shape);
            for (const auto& e : host_.mod().map().entries())
                if (e.organism == c && e.param == p)
                    addModRow(e.source, e.value, e.min, e.max, e.shape);
        }
        rebuildModSourceBox(enable);
        selectSource(rows_.empty() ? -1 : 0);
        hint_.setText(rows_.empty()
                          ? "No control sources for this parameter yet. Type a CC number and "
                            "Add, or Capture and move a hardware control (Quick-Map)."
                          : "Edit CC / range in place (Return applies). "
                            + juce::String(juce::CharPointer_UTF8("\xe2\x97\x8f"))
                            + " in the list marks mapped parameters.",
                      juce::dontSendNotification);
        mapSignature_ = signature();
        layoutSourceRows();
    }

    Unit rowUnit() const { return paramUnit(host_, selectedOrganism(), selectedParam()); }
    std::pair<double, double> rowSpan() const {
        return paramRange(host_, selectedOrganism(), selectedParam());
    }

    juce::String numText(double v) const {
        const auto [lo, hi] = rowSpan();
        return unitPlain(rowUnit(), v, lo, hi);
    }
    double numValue(const juce::String& text) const {
        const auto [lo, hi] = rowSpan();
        return unitPlainParse(rowUnit(), text, lo, hi);
    }

    static juce::String rangeChars() {
        return juce::String::fromUTF8("0123456789.,-#ABCDEFGLR\xe2\x88\x9e");
    }

    std::unique_ptr<juce::TextEditor> numberCell(const juce::String& value,
                                                 const juce::String& allowed) {
        auto ed = std::make_unique<juce::TextEditor>();
        ed->setInputRestrictions(12, allowed);
        ed->setJustification(juce::Justification::centred);
        ed->setText(value, juce::dontSendNotification);
        sourceRows_.addAndMakeVisible(*ed);
        return ed;
    }

    std::unique_ptr<juce::TextButton> kindButton(const juce::String& text) {
        auto b = std::make_unique<juce::TextButton>(text);
        const int idx = (int) rows_.size();
        b->setClickingTogglesState(false);
        b->onClick = [this, idx] { selectSource(idx); };
        sourceRows_.addAndMakeVisible(*b);
        return b;
    }

    void addCcRow(int cc, double min, double max, const ControlShape& shape) {
        SourceRow r;
        r.kind = kindButton(isNoteSource(cc) ? "MIDI Note" : "MIDI CC");
        r.ccNum = cc;
        r.shape = shape;
        r.cc = numberCell(juce::String(cc), "0123456789");
        r.min = numberCell(numText(min), rangeChars());
        r.max = numberCell(numText(max), rangeChars());
        auto apply = [this, cc, shape, ccEd = r.cc.get(), minEd = r.min.get(),
                      maxEd = r.max.get()] {
            const auto c = selectedOrganism();
            const auto p = selectedParam();
            if (c.empty() || p.empty()) return;
            const int newCc = juce::jlimit(0, 127, ccEd->getText().getIntValue());
            host_.midi().clearCC(cc, c, p);
            host_.midi().mapCC(newCc, c, p, numValue(minEd->getText()),
                               numValue(maxEd->getText()), false);
            if (!shape.isDefault()) host_.midi().setShape(newCc, c, p, shape);
            rebuildSources();
            params_.repaint();
        };
        for (auto* ed : {r.cc.get(), r.min.get(), r.max.get()}) {
            ed->onReturnKey = apply;
            ed->onFocusLost = apply;
        }
        r.remove = std::make_unique<juce::TextButton>("Remove");
        r.remove->onClick = [this, cc] {
            host_.midi().clearCC(cc, selectedOrganism(), selectedParam());
            rebuildSources();
            params_.repaint();
        };
        sourceRows_.addAndMakeVisible(*r.remove);
        rows_.push_back(std::move(r));
    }

    void addModRow(const std::string& source, const std::string& value, double min,
                   double max, const ControlShape& shape) {
        SourceRow r;
        r.kind = kindButton(isParamSource(value) ? "Follow" : "Mod");
        r.isMod = true;
        r.modSource = source;
        r.modValue = value;
        r.shape = shape;
        r.address = std::make_unique<juce::Label>();
        r.address->setText(juce::String(source) + " / "
                               + juce::String(paramSourceName(value)),
                           juce::dontSendNotification);
        r.address->setFont(juce::FontOptions(12.0f));
        sourceRows_.addAndMakeVisible(*r.address);
        r.min = numberCell(numText(min), rangeChars());
        r.max = numberCell(numText(max), rangeChars());
        auto apply = [this, source, value, minEd = r.min.get(), maxEd = r.max.get()] {
            const auto c = selectedOrganism();
            const auto p = selectedParam();
            if (c.empty() || p.empty()) return;
            host_.mod().mapRoute(source, value, c, p, numValue(minEd->getText()),
                                 numValue(maxEd->getText()));
            rebuildSources();
            params_.repaint();
        };
        for (auto* ed : {r.min.get(), r.max.get()}) {
            ed->onReturnKey = apply;
            ed->onFocusLost = apply;
        }
        r.remove = std::make_unique<juce::TextButton>("Remove");
        r.remove->onClick = [this, source, value] {
            host_.mod().clearRoute(source, value, selectedOrganism(), selectedParam());
            rebuildSources();
            params_.repaint();
        };
        sourceRows_.addAndMakeVisible(*r.remove);
        rows_.push_back(std::move(r));
    }

    void addOscRow(const std::string& address, const ControlShape& shape) {
        SourceRow r;
        r.kind = kindButton("OSC");
        r.isOsc = true;
        r.oscAddress = address;
        r.shape = shape;
        r.address = std::make_unique<juce::Label>();
        r.address->setText(juce::String(address), juce::dontSendNotification);
        r.address->setFont(juce::FontOptions(12.0f));
        sourceRows_.addAndMakeVisible(*r.address);
        r.remove = std::make_unique<juce::TextButton>("Remove");
        r.remove->onClick = [this, address] {
            host_.osc().clearAddress(address, selectedOrganism(), selectedParam());
            rebuildSources();
            params_.repaint();
        };
        sourceRows_.addAndMakeVisible(*r.remove);
        rows_.push_back(std::move(r));
    }

    void selectSource(int idx) {
        selectedSource_ = idx >= 0 && idx < (int) rows_.size() ? idx : -1;
        for (int i = 0; i < (int) rows_.size(); ++i)
            rows_[(size_t) i].kind->setToggleState(i == selectedSource_,
                                                   juce::dontSendNotification);
        const bool has = selectedSource_ >= 0;
        mapping_.show(has ? rows_[(size_t) selectedSource_].shape : ControlShape{}, has);
    }

    void applyShape(const ControlShape& sh) {
        if (selectedSource_ < 0 || selectedSource_ >= (int) rows_.size()) return;
        auto& r = rows_[(size_t) selectedSource_];
        ControlShape next = sh;
        next.isSwitch = r.shape.isSwitch;
        next.logScale = r.shape.logScale;
        r.shape = next;
        const auto c = selectedOrganism();
        const auto p = selectedParam();
        if (r.isMod) host_.mod().setShape(r.modSource, r.modValue, c, p, next);
        else if (r.isOsc) host_.osc().setShape(r.oscAddress, c, p, next);
        else host_.midi().setShape(r.ccNum, c, p, next);
    }

    void layoutSourceRows() {
        int y = 0;
        for (auto& r : rows_) {
            auto b = juce::Rectangle<int>(0, y, sourceRows_.getWidth(), kRowH).reduced(0, 3);
            r.kind->setBounds(b.removeFromLeft(60));
            if (r.isMod) {
                r.address->setBounds(b.removeFromLeft(140));
                b.removeFromLeft(8);
                r.min->setBounds(b.removeFromLeft(74));
                b.removeFromLeft(4);
                r.max->setBounds(b.removeFromLeft(74));
            } else if (r.cc) {
                r.cc->setBounds(b.removeFromLeft(44));
                b.removeFromLeft(8);
                r.min->setBounds(b.removeFromLeft(74));
                b.removeFromLeft(4);
                r.max->setBounds(b.removeFromLeft(74));
            } else if (r.address) {
                r.address->setBounds(b.removeFromLeft(184));
            }
            b.removeFromLeft(10);
            r.remove->setBounds(b.removeFromLeft(64));
            y += kRowH;
        }
    }

    void addManualCc() {
        const auto c = selectedOrganism();
        const auto p = selectedParam();
        if (c.empty() || p.empty() || addCcEdit_.getText().isEmpty()) return;
        const int cc = juce::jlimit(0, 127, addCcEdit_.getText().getIntValue());
        const auto range = paramRange(host_, c, p);
        host_.midi().mapCC(cc, c, p, range.first, range.second, false);
        addCcEdit_.clear();
        rebuildSources();
        params_.repaint();
    }

    void capture() {
        const auto c = selectedOrganism();
        const auto p = selectedParam();
        if (c.empty() || p.empty()) return;
        const auto range = paramRange(host_, c, p);
        MidiLearner::instance().arm(host_, c, p, range.first, range.second);
    }

    void rebuildModSourceBox(bool enable) {
        const auto prev = modSourceBox_.getSelectedId();
        modSourceBox_.clear(juce::dontSendNotification);
        modChoices_ = host_.mod().availableSources();
        std::string open;
        for (size_t i = 0; i < modChoices_.size(); ++i) {
            const auto& [from, value] = modChoices_[i];
            if (from != open) {
                open = from;
                modSourceBox_.addSectionHeading(juce::String(from));
            }
            modSourceBox_.addItem(juce::String(paramSourceName(value)), (int) i + 1);
        }
        if (prev > 0 && prev <= (int) modChoices_.size())
            modSourceBox_.setSelectedId(prev, juce::dontSendNotification);
        else if (!modChoices_.empty())
            modSourceBox_.setSelectedId(1, juce::dontSendNotification);
        modSourceBox_.setTextWhenNoChoicesAvailable("Nothing in the patch to follow");
        modSourceBox_.setEnabled(enable && !modChoices_.empty());
        addModBtn_.setEnabled(enable && !modChoices_.empty());
    }

    void addModRoute() {
        const auto c = selectedOrganism();
        const auto p = selectedParam();
        const int id = modSourceBox_.getSelectedId();
        if (c.empty() || p.empty() || id <= 0 || id > (int) modChoices_.size()) return;
        const auto& [source, value] = modChoices_[(size_t) id - 1];
        const auto range = paramRange(host_, c, p);
        host_.mod().mapRoute(source, value, c, p, range.first, range.second);
        rebuildSources();
        params_.repaint();
    }

    juce::String signature() const {
        juce::String s;
        s << (int) host_.model().organisms.size() << "|"
          << (int) host_.midi().map().entries().size() << "|"
          << (int) host_.osc().map().entries().size() << "|"
          << (int) host_.mod().map().entries().size();
        return s;
    }
    void timerCallback() override {
        if (!isShowing()) return;
        if (signature() != mapSignature_) {
            rebuildOrganisms();
            params_.repaint();
        }
        captureBtn_.setEnabled(!selectedParam().empty() && !MidiLearner::instance().armed());
    }

    EngineHost& host_;
    OrganismListModel organismModel_;
    ParamListModel paramModel_;
    juce::ListBox organisms_, params_;
    std::vector<std::string> names_, paramNames_;

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
