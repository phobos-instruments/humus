// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/properties/ParameterControlView.h"

#include <cmath>

namespace hum {

std::unique_ptr<DragNumberEditor> ParameterControlView::numberCell(const juce::String& value, const juce::String& allowed) {
    auto ed = std::make_unique<DragNumberEditor>();
    const auto [lo, hi] = rowSpan();
    ed->setRange(lo, hi, false);
    ed->parse = [this](const juce::String& t) { return numValue(t); };
    ed->format = [this](double v) { return numText(v); };
    ed->setInputRestrictions(12, allowed);
    ed->setJustification(juce::Justification::centred);
    ed->setText(value, juce::dontSendNotification);
    sourceRows_.addAndMakeVisible(*ed);
    return ed;
}

std::unique_ptr<juce::TextButton> ParameterControlView::kindButton(const juce::String& text) {
    auto b = std::make_unique<juce::TextButton>(text);
    const int idx = (int) rows_.size();
    b->setClickingTogglesState(false);
    b->onClick = [this, idx] { selectSource(idx); };
    sourceRows_.addAndMakeVisible(*b);
    return b;
}

void ParameterControlView::addCcRow(const MidiSource& src, double min, double max, const ControlShape& shape) {
    SourceRow r;
    r.kind = kindButton(isNoteSource(src.cc) ? tr("parameter-control.midi-note", "MIDI Note") : tr("parameter-control.midi-cc", "MIDI CC"));
    r.source = src;
    r.shape = shape;
    if (src.isChord()) {
        juce::String chord;
        for (int h : src.held) chord << juce::String(midiSourceLabel(h)) << " + ";
        r.held = std::make_unique<juce::Label>();
        r.held->setText(chord, juce::dontSendNotification);
        r.held->setFont(juce::FontOptions(12.0f));
        r.held->setJustificationType(juce::Justification::centredRight);
        sourceRows_.addAndMakeVisible(*r.held);
    }
    r.cc = numberCell(juce::String(isNoteSource(src.cc) ? noteOfSource(src.cc) : src.cc),
                      "0123456789");
    r.cc->setRange(0.0, kMidiMaxD, true);
    r.cc->parse = [](const juce::String& t) { return (double) t.getIntValue(); };
    r.cc->format = [](double v) { return juce::String((int) std::lround(v)); };
    r.min = numberCell(numText(min), rangeChars());
    r.max = numberCell(numText(max), rangeChars());
    auto apply = [this, src, shape, ccEd = r.cc.get(), minEd = r.min.get(),
                  maxEd = r.max.get()] {
        const auto c = selectedOrganism();
        const auto p = selectedParam();
        if (c.empty() || p.empty()) return;
        const int number = juce::jlimit(0, kMidiMax, ccEd->getText().getIntValue());
        const double mn = numValue(minEd->getText());
        const double mx = numValue(maxEd->getText());
        later([this, src, shape, c, p, number, mn, mx] {
            const MidiSource next(isNoteSource(src.cc) ? sourceForNote(number) : number, src.held);
            host_.midi().clearCC(src, c, p);
            host_.midi().mapCC(next, c, p, mn, mx, false);
            if (!shape.isDefault()) host_.midi().setShape(next, c, p, shape);
            rebuildSources();
            params_.repaint();
        });
    };
    for (auto* ed : {r.cc.get(), r.min.get(), r.max.get()}) {
        ed->onReturnKey = apply;
        ed->onFocusLost = apply;
    }
    r.remove = std::make_unique<juce::TextButton>("Remove");
    r.remove->onClick = [this, src] {
        later([this, src] {
            host_.midi().clearCC(src, selectedOrganism(), selectedParam());
            rebuildSources();
            params_.repaint();
        });
    };
    sourceRows_.addAndMakeVisible(*r.remove);
    rows_.push_back(std::move(r));
}

void ParameterControlView::addModRow(const std::string& source, const std::string& value, double min, double max, const ControlShape& shape) {
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
        const double mn = numValue(minEd->getText());
        const double mx = numValue(maxEd->getText());
        later([this, source, value, c, p, mn, mx] {
            host_.mod().mapRoute(source, value, c, p, mn, mx);
            rebuildSources();
            params_.repaint();
        });
    };
    for (auto* ed : {r.min.get(), r.max.get()}) {
        ed->onReturnKey = apply;
        ed->onFocusLost = apply;
    }
    r.remove = std::make_unique<juce::TextButton>("Remove");
    r.remove->onClick = [this, source, value] {
        later([this, source, value] {
            host_.mod().clearRoute(source, value, selectedOrganism(), selectedParam());
            rebuildSources();
            params_.repaint();
        });
    };
    sourceRows_.addAndMakeVisible(*r.remove);
    rows_.push_back(std::move(r));
}

void ParameterControlView::addOscRow(const std::string& address, const ControlShape& shape) {
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
        later([this, address] {
            host_.osc().clearAddress(address, selectedOrganism(), selectedParam());
            rebuildSources();
            params_.repaint();
        });
    };
    sourceRows_.addAndMakeVisible(*r.remove);
    rows_.push_back(std::move(r));
}

void ParameterControlView::selectSource(int idx) {
    selectedSource_ = idx >= 0 && idx < (int) rows_.size() ? idx : -1;
    for (int i = 0; i < (int) rows_.size(); ++i)
        rows_[(size_t) i].kind->setToggleState(i == selectedSource_,
                                               juce::dontSendNotification);
    const bool has = selectedSource_ >= 0;
    mapping_.show(has ? rows_[(size_t) selectedSource_].shape : ControlShape{}, has);
}

void ParameterControlView::applyShape(const ControlShape& sh) {
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
    else host_.midi().setShape(r.source, c, p, next);
}

void ParameterControlView::addManualCc() {
    const auto c = selectedOrganism();
    const auto p = selectedParam();
    if (c.empty() || p.empty() || addCcEdit_.getText().isEmpty()) return;
    const int cc = juce::jlimit(0, kMidiMax, addCcEdit_.getText().getIntValue());
    const auto range = paramRange(host_, c, p);
    host_.midi().mapCC(cc, c, p, range.first, range.second, false);
    addCcEdit_.clear();
    rebuildSources();
    params_.repaint();
}

void ParameterControlView::capture() {
    const auto c = selectedOrganism();
    const auto p = selectedParam();
    if (c.empty() || p.empty()) return;
    const auto range = paramRange(host_, c, p);
    MidiLearner::instance().arm(host_, c, p, range.first, range.second);
}

void ParameterControlView::rebuildModSourceBox(bool enable) {
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
    modSourceBox_.setTextWhenNoChoicesAvailable(tr("parameter-control.nothing-in-the-patch-to", "Nothing in the patch to follow"));
    modSourceBox_.setEnabled(enable && !modChoices_.empty());
    addModBtn_.setEnabled(enable && !modChoices_.empty());
}

void ParameterControlView::addModRoute() {
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

}
