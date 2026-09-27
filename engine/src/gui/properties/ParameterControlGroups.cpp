// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/properties/ParameterControlView.h"

#include <algorithm>

namespace hum {

juce::String ParameterControlView::endName(RangeEnd end) {
    switch (end) {
        case RangeEnd::Low:    return tr("parameter-control.minimum", "Minimum");
        case RangeEnd::High:   return tr("parameter-control.maximum", "Maximum");
        case RangeEnd::Spread: return tr("parameter-control.spread", "Spread");
        case RangeEnd::Whole:
        default:               return tr("parameter-control.value", "Value");
    }
}

std::vector<RangeEnd> ParameterControlView::shownEnds() const {
    if (!selectedIsRange()) return {RangeEnd::Whole};
    switch (host_.rangeMode(selectedOrganism(), selectedParam())) {
        case RangeMode::MinMax: return {RangeEnd::Low, RangeEnd::High};
        case RangeMode::Single: return {RangeEnd::Whole};
        case RangeMode::ValueSpread:
        default:                return {RangeEnd::Whole, RangeEnd::Spread};
    }
}

bool ParameterControlView::hiddenMappings() const {
    if (!selectedIsRange()) return false;
    const auto shown = shownEnds();
    for (const auto end : {RangeEnd::Whole, RangeEnd::Spread, RangeEnd::Low, RangeEnd::High}) {
        if (std::find(shown.begin(), shown.end(), end) != shown.end()) continue;
        if (host_.isExternallyControlled(selectedOrganism(), aimedParam(end))) return true;
    }
    return false;
}

void ParameterControlView::learnInto(int group) {
    const auto ends = shownEnds();
    const auto c = selectedOrganism();
    if (group < 0 || group >= (int) ends.size() || c.empty() || selectedParam().empty()) return;
    const auto range = endBounds(host_, c, selectedParam(), ends[(size_t) group]);
    MidiLearner::instance().arm(host_, c, aimedParam(ends[(size_t) group]), range.first,
                                range.second, false);
    MidiLearner::instance().onCaptured = [this] { later([this] { rebuildSources(); }); };
    rebuildSources();
}

void ParameterControlView::showAddMenu(int group) {
    const auto ends = shownEnds();
    if (group < 0 || group >= (int) ends.size() || selectedParam().empty()) return;
    juce::PopupMenu menu;
    menu.addItem(1, tr("parameter-control.enter-a-cc", "Enter a CC number..."));
    menu.addSeparator();
    menu.addItem(2, tr("parameter-control.follow-a-control", "Follow a control..."));
    juce::PopupMenu list;
    std::string open;
    for (size_t i = 0; i < modChoices_.size(); ++i) {
        const auto& [from, value] = modChoices_[i];
        if (from != open) { open = from; list.addSectionHeader(juce::String(from)); }
        list.addItem(100 + (int) i, juce::String(paramSourceName(value)));
    }
    menu.addSubMenu(tr("parameter-control.follow-from-a-list", "Follow from a list"), list,
                    list.getNumItems() > 0);
    if (host_.osc().enabled()) {
        menu.addSeparator();
        menu.addItem(3, tr("parameter-control.osc-learn", "OSC Learn"));
    }
    const auto at = juce::Desktop::getMousePosition();
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({at.x, at.y, 1, 1}),
                       [this, group, at](int pick) {
        if (pick <= 0) return;
        const auto c = selectedOrganism();
        const auto ends = shownEnds();
        if (c.empty() || group >= (int) ends.size()) return;
        const auto target = aimedParam(ends[(size_t) group]);
        const auto span = endBounds(host_, c, selectedParam(), ends[(size_t) group]);
        if (pick == 1) {
            NumberPrompt::show({at.x, at.y, 1, 1},
                               tr("parameter-control.cc-number", "CC number"), "0123456789", 1,
                               [this, group](int cc) { addManualCc(group, cc); });
        } else if (pick == 2) {
            FollowPicker::instance().arm(host_, c, target, span.first, span.second,
                                         [this] { later([this] { rebuildSources(); }); });
        } else if (pick == 3) {
            OscLearner::instance().arm(host_, c, target, span.first, span.second);
        } else if (pick >= 100) {
            addModRoute(group, pick - 100);
        }
    });
}

int ParameterControlView::rowOfComponent(juce::Component* c) const {
    for (; c != nullptr && c != &sourceRows_; c = c->getParentComponent())
        for (int i = 0; i < (int) rows_.size(); ++i) {
            const auto& r = rows_[(size_t) i];
            for (const juce::Component* owned : {(juce::Component*) r.tile.get(), (juce::Component*) r.remove.get()})
                if (owned != nullptr && owned == c) return i;
        }
    return -1;
}

void ParameterControlView::showRowMenu(int row) {
    if (row < 0 || row >= (int) rows_.size()) return;
    selectSource(row);
    const auto was = rows_[(size_t) row].end;
    const bool waiting = rows_[(size_t) row].isWaiting;
    juce::PopupMenu menu;
    if (selectedIsRange() && !waiting) {
        for (int i = 0; i < 3; ++i)
            menu.addItem(i + 1, tr("parameter-control.move-to", "Move to ") + endName(kEnds[i]),
                         kEnds[i] != was, kEnds[i] == was);
        menu.addSeparator();
    }
    menu.addItem(10, tr("parameter-control.remove", "Remove"));
    menu.showMenuAsync(juce::PopupMenu::Options(), [this, row](int pick) {
        if (pick <= 0 || row >= (int) rows_.size()) return;
        if (pick == 10) removeRow(rows_[(size_t) row]);
        else            reaimRow(rows_[(size_t) row], kEnds[pick - 1]);
    });
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
    const auto organism = selectedOrganism();
    switchingOrganism_ = true;
    paramNames_ = controlTargets(host_, organism);
    params_.updateContent();
    int row = -1;
    if (const auto it = lastParam_.find(organism); it != lastParam_.end())
        for (size_t i = 0; i < paramNames_.size(); ++i)
            if (paramNames_[i] == it->second) row = (int) i;
    if (row >= 0) params_.selectRow(row);
    else params_.deselectAllRows();
    switchingOrganism_ = false;
    params_.repaint();
    rebuildSources();
}

void ParameterControlView::paramChosen() {
    if (switchingOrganism_) return;
    if (const auto p = selectedParam(); !p.empty()) lastParam_[selectedOrganism()] = p;
    rebuildSources();
}

void ParameterControlView::later(std::function<void()> fn) {
    juce::MessageManager::callAsync(
        [safe = juce::Component::SafePointer<ParameterControlView>(this), fn] {
            if (safe != nullptr) fn();
        });
}

void ParameterControlView::layoutSourceRows() {
    int y = 0;
    const auto ends = shownEnds();
    const bool named = ends.size() > 1;
    const int w = sourceRows_.getWidth();
    for (int g = 0; g < 3; ++g) {
        const bool live = g < (int) ends.size();
        groupHead_[g].setVisible(live && named);
        groupEmpty_[g].setVisible(false);
        groupLearn_[g].setVisible(live);
        groupAdd_[g].setVisible(live);
        if (!live) continue;
        auto head = juce::Rectangle<int>(0, y, w, kGroupH);
        groupAdd_[g].setBounds(head.removeFromRight(kAddW));
        head.removeFromRight(6);
        groupLearn_[g].setBounds(head.removeFromRight(kLearnW));
        if (named) {
            groupHead_[g].setText(endName(ends[(size_t) g]).toUpperCase(), juce::dontSendNotification);
            groupHead_[g].setBounds(head);
        }
        y += kGroupH + 6;
        int seen = 0;
        for (auto& r : rows_) {
            if (r.end != ends[(size_t) g]) continue;
            y = layoutOneRow(r, y, seen++ == 0, named, g);
        }
        if (seen == 0) {
            groupEmpty_[g].setVisible(true);
            groupEmpty_[g].setBounds(0, y, w, kLineH);
            y += kLineH;
        }
        y += kGroupGap;
    }
}

int ParameterControlView::layoutOneRow(SourceRow& r, int y, bool, bool, int) {
    auto line = juce::Rectangle<int>(0, y, sourceRows_.getWidth(), kLineH).reduced(0, 2);
    r.remove->setBounds(line.removeFromRight(kRemoveW));
    line.removeFromRight(4);
    r.tile->setBounds(line);
    return y + kLineH;
}

int ParameterControlView::sourceRowsHeight() const {
    int h = 0;
    const auto ends = shownEnds();
    for (const auto end : ends) {
        int lines = 0;
        for (const auto& r : rows_)
            if (r.end == end) ++lines;
        h += kGroupH + 6 + (lines == 0 ? 1 : lines) * kLineH + kGroupGap;
    }
    return h;
}


}
