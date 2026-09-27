// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/properties/ParameterControlView.h"

#include <algorithm>

namespace hum {

ParameterControlView::ParameterControlView(PropertiesHost& host)
    : host_(host), organisms_("organisms", &organismModel_), params_("params", &paramModel_) {
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

    modeLabel_.setText(tr("parameter-control.range-mode", "Range Mode"), juce::dontSendNotification);
    modeLabel_.setFont(juce::FontOptions(11.5f));
    modeLabel_.setColour(juce::Label::textColourId, Palette::textDim);
    addChildComponent(modeLabel_);
    mode_.addItem(tr("parameter-control.value-spread", "Value / Spread"), 1);
    mode_.addItem(tr("parameter-control.min-max", "Minimum / Maximum"), 2);
    mode_.addItem(tr("parameter-control.single", "Single"), 3);
    mode_.onChange = [this] {
        const auto c = selectedOrganism(), p = selectedParam();
        if (c.empty() || p.empty()) return;
        host_.setRangeMode(c, p, modeOfId(mode_.getSelectedId()));
        rebuildSources();
    };
    addChildComponent(mode_);

    totalRange_.setFont(juce::FontOptions(11.5f));
    totalRange_.setColour(juce::Label::textColourId, Palette::textDim);
    addAndMakeVisible(totalRange_);

    for (int i = 0; i < 3; ++i) {
        groupHead_[i].setFont(juce::FontOptions(10.5f));
        groupHead_[i].setColour(juce::Label::textColourId, Palette::textDim);
        groupHead_[i].setInterceptsMouseClicks(false, false);
        groupEmpty_[i].setText(tr("parameter-control.nothing-yet", "nothing yet"),
                               juce::dontSendNotification);
        groupEmpty_[i].setFont(juce::FontOptions(12.0f));
        groupEmpty_[i].setColour(juce::Label::textColourId, Palette::textDim);
        groupEmpty_[i].setInterceptsMouseClicks(false, false);
        groupLearn_[i].setButtonText(tr("parameter-control.learn", "Learn"));
        groupLearn_[i].setTooltip(tr("parameter-control.learn-tip",
                                     "Move a control on your device to map it here"));
        groupLearn_[i].onClick = [this, i] { learnInto(i); };
        groupAdd_[i].setButtonText("+");
        groupAdd_[i].setTooltip(tr("parameter-control.add-tip",
                                   "Add a CC number, a control to follow, or an OSC address"));
        groupAdd_[i].onClick = [this, i] { showAddMenu(i); };
    }

    hint_.setFont(juce::FontOptions(11.5f));
    hint_.setColour(juce::Label::textColourId, Palette::textDim);
    hint_.setJustificationType(juce::Justification::topLeft);
    addAndMakeVisible(hint_);

    addMouseListener(this, true);

    setSize(kWindowW, 600);
    rebuildOrganisms();
    startTimerHz(15);
}

void ParameterControlView::selectParam(const std::string& organism, const std::string& param) {
    rebuildOrganisms();
    for (size_t i = 0; i < names_.size(); ++i)
        if (names_[i] == organism) {
            organisms_.selectRow((int) i);
            organismSelected();
            for (size_t j = 0; j < paramNames_.size(); ++j)
                if (paramNames_[j] == param) params_.selectRow((int) j);
            if (!param.empty()) lastParam_[organism] = param;
            rebuildSources();
            return;
        }
}

void ParameterControlView::mouseDown(const juce::MouseEvent& e) {
    if (e.mods.isPopupMenu())
        if (const int row = rowOfComponent(e.originalComponent); row >= 0) {
            showRowMenu(row);
            return;
        }
    if (e.originalComponent != nullptr
        && (dynamic_cast<juce::TextEditor*>(e.originalComponent) != nullptr
            || e.originalComponent->findParentComponentOfClass<juce::TextEditor>() != nullptr))
        return;
    if (auto* focused = juce::Component::getCurrentlyFocusedComponent())
        if (dynamic_cast<DragNumberEditor*>(focused) != nullptr) focused->giveAwayKeyboardFocus();
}

void ParameterControlView::resized() {
    auto area = getLocalBounds().reduced(kMargin);
    organisms_.setBounds(area.removeFromLeft(kListW));
    area.removeFromLeft(kListGap);
    params_.setBounds(area.removeFromLeft(kListW));
    area.removeFromLeft(kPanelGap);

    mapping_.setBounds(area.removeFromRight(kInspectorW));
    area.removeFromRight(kPanelGap);
    sourcesTitle_.setBounds(area.removeFromTop(20));
    totalRange_.setBounds(area.removeFromTop(16));
    area.removeFromTop(4);
    const bool ranged = selectedIsRange();
    modeLabel_.setVisible(ranged);
    mode_.setVisible(ranged);
    if (ranged) {
        auto modeRow = area.removeFromTop(26);
        modeLabel_.setBounds(modeRow.removeFromLeft(74));
        mode_.setBounds(modeRow.removeFromLeft(190).reduced(0, 2));
        area.removeFromTop(4);
    }

    area.removeFromTop(8);
    hint_.setBounds(area.removeFromBottom(46));

    sourceRows_.setBounds(area);
    layoutSourceRows();
}

void ParameterControlView::paintRow(juce::Graphics& g, int w, int h, bool sel, const std::string& text, bool mapped) {
    if (sel) g.fillAll(Palette::accent.withAlpha(alpha::scrim));
    g.setColour(mapped ? Palette::accent : Palette::text);
    g.setFont(juce::FontOptions(12.5f));
    g.drawText((mapped ? juce::String::fromUTF8("\xe2\x97\x8f ") : juce::String("   "))
                   + juce::String(text),
               6, 0, w - 10, h, juce::Justification::centredLeft);
}

std::string ParameterControlView::selectedOrganism() const {
    const int r = organisms_.getSelectedRow();
    return r >= 0 && r < (int) names_.size() ? names_[(size_t) r] : std::string();
}

std::string ParameterControlView::selectedParam() const {
    const int r = params_.getSelectedRow();
    return r >= 0 && r < (int) paramNames_.size() ? paramNames_[(size_t) r] : std::string();
}

bool ParameterControlView::isMapped(const std::string& c, const std::string& p) const {
    return paramIsControlled(host_, c, p);
}

void ParameterControlView::normaliseEndBounds() {
    const auto c = selectedOrganism(), p = selectedParam();
    if (c.empty() || p.empty()) return;
    for (const auto end : {RangeEnd::Whole, RangeEnd::Low, RangeEnd::High, RangeEnd::Spread}) {
        const auto target = aimedParam(end);
        const auto [lo, hi] = endBounds(host_, c, p, end);
        auto off = [lo = lo, hi = hi](double a, double b) {
            return std::abs(a - lo) > 1.0e-9 || std::abs(b - hi) > 1.0e-9;
        };
        std::vector<MidiSource> ccs;
        for (const auto& e : host_.midi().map().entries())
            if (e.organism == c && e.param == target && off(e.min, e.max)) ccs.push_back(e.source());
        for (const auto& src : ccs) host_.midi().mapCC(src, c, target, lo, hi, false);
        std::vector<std::pair<std::string, std::string>> routes;
        for (const auto& e : host_.mod().map().entries())
            if (e.organism == c && e.param == target && off(e.min, e.max))
                routes.push_back({e.source, e.value});
        for (const auto& r : routes) host_.mod().mapRoute(r.first, r.second, c, target, lo, hi);
        std::vector<std::string> addrs;
        for (const auto& e : host_.osc().map().entries())
            if (e.organism == c && e.param == target && off(e.min, e.max)) addrs.push_back(e.address);
        for (const auto& a : addrs) host_.osc().mapAddress(a, c, target, lo, hi, false);
    }
}

RangeMode ParameterControlView::modeOfId(int id) {
    return id == 2 ? RangeMode::MinMax : id == 3 ? RangeMode::Single : RangeMode::ValueSpread;
}

int ParameterControlView::idOfMode(RangeMode mode) {
    return mode == RangeMode::MinMax ? 2 : mode == RangeMode::Single ? 3 : 1;
}

bool ParameterControlView::selectedIsRange() const {
    const auto p = selectedParam();
    return !p.empty() && paramIsRange(host_, selectedOrganism(), p);
}

std::string ParameterControlView::aimedParam(RangeEnd end) const {
    return rangeEndParam(selectedParam(), end);
}

void ParameterControlView::rebuildSources() {
    const auto [focusRow, focusWhich] = focusedField();
    const int keep = selectedSource_;
    const auto keepParam = shownParam_;
    mappedValues_.clear();
    rows_.clear();
    sourceRows_.removeAllChildren();
    for (int i = 0; i < 3; ++i) {
        sourceRows_.addChildComponent(groupHead_[i]);
        sourceRows_.addChildComponent(groupEmpty_[i]);
        sourceRows_.addChildComponent(groupLearn_[i]);
        sourceRows_.addChildComponent(groupAdd_[i]);
    }
    const auto c = selectedOrganism();
    const auto p = selectedParam();
    const juce::String sfx = p.empty() ? juce::String() : juce::String(unitSuffix(rowUnit()));
    sourcesTitle_.setText(p.empty() ? tr("parameter-control.sources", "Sources")
                                    : "Sources " + juce::String("- ")
                                          + targetOwnerLabel(host_, c) + " / " + juce::String(p)
                                          + (sfx.isEmpty() ? "" : "  (" + sfx + ")"),
                          juce::dontSendNotification);
    const bool enable = !p.empty();
    const bool ranged = selectedIsRange();
    if (enable && ranged) normaliseEndBounds();

    if (enable) {
        for (const auto end : shownEnds()) {
            const auto target = aimedParam(end);
            for (const auto& e : host_.midi().map().entries())
                if (e.organism == c && e.param == target)
                    addCcRow(e.source(), e.min, e.max, e.shape, end, !ranged);
            for (const auto& e : host_.osc().map().entries())
                if (e.organism == c && e.param == target) addOscRow(e.address, e.shape, end);
            for (const auto& e : host_.mod().map().entries())
                if (e.organism == c && e.param == target)
                    addModRow(e.source, e.value, e.min, e.max, e.shape, end, !ranged);
            if (learnAimsAtForTest(target)) addWaitingRow(end);
        }
    }
    rebuildModSourceBox();
    if (selectedIsRange())
        mode_.setSelectedId(idOfMode(host_.rangeMode(c, p)), juce::dontSendNotification);
    const auto [lo, hi] = rowSpan();
    totalRange_.setText(enable ? tr("parameter-control.total-range", "Total range: ")
                                     + unitPlain(rowUnit(), lo, lo, hi) + " to "
                                     + unitPlain(rowUnit(), hi, lo, hi) + " "
                                     + juce::String(unitSuffix(rowUnit()))
                               : juce::String(),
                        juce::dontSendNotification);
    shownParam_ = c + "\t" + p;
    const bool sameParam = keepParam == shownParam_;
    selectSource(rows_.empty() ? -1 : sameParam ? juce::jlimit(0, (int) rows_.size() - 1, keep) : 0);
    hint_.setText(hiddenMappings()
                      ? tr("parameter-control.hint-hidden",
                           "This parameter also has sources in the other Range Mode. "
                           "Switch back to see them.")
                  : rows_.empty()
                      ? tr("parameter-control.hint-empty",
                           "Nothing controls this yet. Learn a controller, type a CC number, "
                           "or follow something already in the patch.")
                      : ranged
                      ? tr("parameter-control.hint-span",
                           "Each source sweeps the parameter's whole range. "
                           "Right-click one to move it to another slot.")
                      : tr("parameter-control.hint-pick",
                           "Pick a source to edit it on the right. "
                           "Right-click one for more."),
                  juce::dontSendNotification);
    mapSignature_ = signature();
    wasLearning_ = MidiLearner::instance().armed();
    resized();
    if (auto* ed = sourceFieldForTest(focusRow, focusWhich)) {
        ed->grabKeyboardFocus();
        ed->selectAll();
    }
}

std::pair<double, double> ParameterControlView::rowSpan() const {
    return paramRange(host_, selectedOrganism(), selectedParam());
}

juce::String ParameterControlView::numText(double v) const {
    const auto [lo, hi] = rowSpan();
    return unitPlain(rowUnit(), v, lo, hi);
}

double ParameterControlView::numValue(const juce::String& text) const {
    const auto [lo, hi] = rowSpan();
    const double v = unitPlainParse(rowUnit(), text, lo, hi);
    return hi > lo ? juce::jlimit(lo, hi, v) : v;
}

juce::String ParameterControlView::rangeChars() {
    return juce::String::fromUTF8("0123456789.,-#ABCDEFGLR\xe2\x88\x9e");
}

juce::String ParameterControlView::signature() const {
    juce::String s;
    s << (int) host_.model().organisms.size() << "|"
      << (int) host_.midi().map().entries().size() << "|"
      << (int) host_.osc().map().entries().size() << "|"
      << (int) host_.mod().map().entries().size();
    int lanes = 0;
    for (const auto& cm : host_.model().organisms) lanes += (int) cm.automation.size();
    s << "|" << lanes;
    return s;
}

void ParameterControlView::timerCallback() {
    if (!isShowing()) return;
    updateLive();
    if (signature() != mapSignature_) {
        rebuildOrganisms();
        params_.repaint();
    } else if (MidiLearner::instance().armed() != wasLearning_) {
        rebuildSources();
    }
}

}
