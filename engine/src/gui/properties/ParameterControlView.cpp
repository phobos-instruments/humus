// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/properties/ParameterControlView.h"

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

    addCcLabel_.setText(tr("parameter-control.add-cc", "Add CC"), juce::dontSendNotification);
    addCcLabel_.setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(addCcLabel_);
    addCcEdit_.setInputRestrictions(3, "0123456789");
    addCcEdit_.setJustification(juce::Justification::centred);
    addAndMakeVisible(addCcEdit_);
    addBtn_.setButtonText(tr("parameter-control.add", "Add"));
    addBtn_.onClick = [this] { addManualCc(); };
    addAndMakeVisible(addBtn_);
    captureBtn_.setButtonText(tr("parameter-control.capture-next-controller", "Capture next controller..."));
    captureBtn_.onClick = [this] { capture(); };
    addAndMakeVisible(captureBtn_);

    addModLabel_.setText(tr("parameter-control.from", "From"), juce::dontSendNotification);
    addModLabel_.setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(addModLabel_);
    addAndMakeVisible(modSourceBox_);
    addModBtn_.setButtonText(tr("parameter-control.add", "Add"));
    addModBtn_.onClick = [this] { addModRoute(); };
    addAndMakeVisible(addModBtn_);

    hint_.setFont(juce::FontOptions(11.5f));
    hint_.setColour(juce::Label::textColourId, Palette::textDim);
    hint_.setJustificationType(juce::Justification::topLeft);
    addAndMakeVisible(hint_);

    setSize(kWindowW, 560);
    rebuildOrganisms();
    startTimerHz(2);
}

void ParameterControlView::selectParam(const std::string& organism, const std::string& param) {
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

void ParameterControlView::resized() {
    auto area = getLocalBounds().reduced(kMargin);
    organisms_.setBounds(area.removeFromLeft(kListW));
    area.removeFromLeft(kListGap);
    params_.setBounds(area.removeFromLeft(kListW));
    area.removeFromLeft(kPanelGap);

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

DragNumberEditor* ParameterControlView::sourceFieldForTest(int row, int which) {
    if (row < 0 || row >= (int) rows_.size()) return nullptr;
    auto& r = rows_[(size_t) row];
    return which == 0 ? r.cc.get() : which == 1 ? r.min.get() : r.max.get();
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
    return host_.isExternallyControlled(c, p);
}

void ParameterControlView::markControlledOrganisms() {
    controlledOrganisms_.clear();
    for (const auto& n : names_)
        for (const auto& p : controlTargets(host_, n))
            if (isMapped(n, p)) { controlledOrganisms_.insert(n); break; }
}

void ParameterControlView::rebuildOrganisms() {
    const auto prev = selectedOrganism();
    names_.clear();
    for (const auto& cm : host_.model().organisms)
        if (!controlTargets(host_, cm.name).empty()) names_.push_back(cm.name);
    markControlledOrganisms();
    organisms_.updateContent();
    organisms_.repaint();
    int sel = names_.empty() ? -1 : 0;
    for (size_t i = 0; i < names_.size(); ++i)
        if (names_[i] == prev) sel = (int) i;
    if (sel >= 0) organisms_.selectRow(sel);
    organismSelected();
}

void ParameterControlView::organismSelected() {
    paramNames_ = controlTargets(host_, selectedOrganism());
    params_.updateContent();
    params_.repaint();
    rebuildSources();
}

void ParameterControlView::later(std::function<void()> fn) {
    juce::MessageManager::callAsync(
        [safe = juce::Component::SafePointer<ParameterControlView>(this), fn] {
            if (safe != nullptr) fn();
        });
}

void ParameterControlView::rebuildSources() {
    rows_.clear();
    sourceRows_.removeAllChildren();
    const auto c = selectedOrganism();
    const auto p = selectedParam();
    const juce::String sfx = p.empty() ? juce::String() : juce::String(unitSuffix(rowUnit()));
    sourcesTitle_.setText(p.empty() ? tr("parameter-control.sources", "Sources")
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
            if (e.organism == c && e.param == p) addCcRow(e.source(), e.min, e.max, e.shape);
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
                        + " marks what is automated or mapped.",
                  juce::dontSendNotification);
    mapSignature_ = signature();
    layoutSourceRows();
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
    return unitPlainParse(rowUnit(), text, lo, hi);
}

juce::String ParameterControlView::rangeChars() {
    return juce::String::fromUTF8("0123456789.,-#ABCDEFGLR\xe2\x88\x9e");
}

void ParameterControlView::layoutSourceRows() {
    int y = 0;
    for (auto& r : rows_) {
        auto b = juce::Rectangle<int>(0, y, sourceRows_.getWidth(), kRowH).reduced(0, 3);
        r.kind->setBounds(b.removeFromLeft(kKindW));
        if (r.isMod) {
            r.address->setBounds(b.removeFromLeft(140));
            b.removeFromLeft(8);
            r.min->setBounds(b.removeFromLeft(kBoundW));
            b.removeFromLeft(4);
            r.max->setBounds(b.removeFromLeft(kBoundW));
        } else if (r.cc) {
            if (r.held) {
                r.held->setBounds(b.removeFromLeft(kHeldW));
                b.removeFromLeft(4);
            }
            r.cc->setBounds(b.removeFromLeft(kCcW));
            b.removeFromLeft(8);
            r.min->setBounds(b.removeFromLeft(kBoundW));
            b.removeFromLeft(4);
            r.max->setBounds(b.removeFromLeft(kBoundW));
        } else if (r.address) {
            r.address->setBounds(b.removeFromLeft(184));
        }
        b.removeFromLeft(10);
        r.remove->setBounds(b.removeFromLeft(kRemoveW));
        y += kRowH;
    }
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
    if (signature() != mapSignature_) {
        rebuildOrganisms();
        params_.repaint();
    }
    captureBtn_.setEnabled(!selectedParam().empty() && !MidiLearner::instance().armed());
}

}
