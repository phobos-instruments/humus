// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/tracks/SongView.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "core/graph/PodModel.h"
#include "core/params/ParamSchema.h"
#include "core/timeline/Automation.h"

namespace hum {

void SongView::traceRows(const char* what) const {
    static const bool on = std::getenv("HUMUS_VIEW_DEBUG") != nullptr;
    if (!on) return;
    std::fprintf(stderr, "[song] %-12s rows=%d v=%d bottom=%d box=%d\n", what, (int) rows_.size(), view_.vScroll,
                 getBottom(), (int) inBox());
}

void SongView::rebuild() {
    traceRows("rebuild");
    rows_.clear();
    arrangeable_.clear();
    for (const auto& n : host().arrangeableNodes()) {
        rows_.push_back(n);
        arrangeable_.insert(n);
    }
    if (const auto pinned = ctx_.pinnedRow(); !pinned.empty() && arrangeable_.count(pinned) == 0
        && host().model().byName(pinned) != nullptr) {
        rows_.push_back(pinned);
        arrangeable_.insert(pinned);
    }
    for (const auto& cm : host().model().organisms) {
        if (arrangeable_.count(cm.name)) continue;
        bool hasBox = !cm.automation.empty();
        for (const auto& b : host().automation().boxes())
            if (b.organism == cm.name) { hasBox = true; break; }
        if (!hasBox) continue;
        rows_.push_back(cm.name);
        if (autoOnlyRows_.insert(cm.name).second) expanded_.insert(cm.name);
    }
    if (inBox()) {
        const bool alive = std::find(rows_.begin(), rows_.end(), boxNode_) != rows_.end();
        if (!alive) leaveBox();
        else {
            rows_ = {boxNode_};
            expanded_.insert(boxNode_);
        }
    }
    {
        std::vector<std::string> soloable;
        for (const auto& n : rows_) if (arrangeable_.count(n) != 0) soloable.push_back(n);
        host().setSoloable(std::move(soloable));
    }
    rebuildSlots();
    const int contentH = trackslayout::totalHeight(slots_, headerH() - view_.vScroll) + view_.vScroll;
    if (const int maxV = std::max(0, contentH - getBottom() + ZoomBar::kGut); view_.vScroll > maxV) {
        view_.vScroll = maxV;
        rebuildSlots();
    }
    repaintAll();
}

void SongView::rebuildSlots() {
    if (inBox() && !rows_.empty()) {
        const int n = std::max(1, (int) lanesOf(0).size());
        const int avail = getBottom() - headerH() - kBoxRowH;
        boxLaneH_ = juce::jlimit(kLaneH * 2, 160, avail / n);
    }
    slots_ = trackslayout::build(
        (int) rows_.size(),
        [this](int t) { return expanded_.count(rows_[(size_t) t]) != 0; },
        [this](int t) { return lanesOf(t); },
        headerH() - view_.vScroll, inBox() ? 0 : rowH_,
        inBox() ? boxLaneH_ : kLaneH,
        [this](int t) { return podOfRow(t); },
        [this](const std::string& pod) { return collapsedPods_.count(pod) != 0; },
        kPodH,
        [this](int t) { return wantsBoxRow(t); },
        kBoxRowH);
}

bool SongView::wantsBoxRow(int row) const {
    if (row < 0 || row >= (int) rows_.size()) return false;
    const auto& node = rows_[(size_t) row];
    if (inBox()) return true;
    if (arrangeable_.count(node) == 0) return false;
    if (const auto* cm = host().model().byName(node); cm != nullptr && !cm->automation.empty())
        return true;
    for (const auto& b : host().automation().boxes())
        if (b.organism == node) return true;
    return false;
}

int SongView::boxRowSlot(int row) const {
    for (int i = 0; i < (int) slots_.size(); ++i)
        if (slots_[(size_t) i].kind == trackslayout::Kind::BoxRow && slots_[(size_t) i].track == row)
            return i;
    return -1;
}

std::string SongView::podOfRow(int t) const {
    if (t < 0 || t >= (int) rows_.size()) return {};
    return pods::childPodOf(rows_[(size_t) t], "");
}

void SongView::setRowHeight(int h) {
    rowH_ = h;
    rebuildSlots();
}

void SongView::resized() {
    if (inBox()) rebuildSlots();
}

void SongView::applyVScroll(int v) {
    const int contentH = trackslayout::totalHeight(slots_, headerH() - view_.vScroll) + view_.vScroll;
    const int maxV = std::max(0, contentH - getBottom() + ZoomBar::kGut);
    v = juce::jlimit(0, maxV, v);
    if (v == view_.vScroll) return;
    view_.vScroll = v;
    rebuildSlots();
    repaintAll();
}

std::string SongView::selectedNoteNode() const {
    if (selClipRow_ < 0 || selClipRow_ >= (int) rows_.size()) return {};
    const auto& node = rows_[(size_t) selClipRow_];
    return host().nodeRecordsAudio(node) ? std::string() : node;
}

bool SongView::nodeHasMuteParam(const std::string& node) const {
    auto& h = const_cast<TracksHost&>(host());
    const auto* cm = host().model().byName(node);
    if (cm == nullptr || h.liveOrganism(node) == nullptr) return false;
    for (const auto& d : schemaFor(cm->classRaw))
        if (d.name == "Mute") return true;
    return false;
}

bool SongView::nodeMuted(const std::string& node) const {
    if (nodeHasMuteParam(node)) {
        if (const auto* cm = host().model().byName(node))
            for (const auto& pr : cm->properties)
                if (pr.name == "Mute") return pr.value >= 0.5;
        return false;
    }
    return host().trackMuted(node);
}

void SongView::setNodeMuted(const std::string& node, bool muted) {
    if (nodeHasMuteParam(node)) host().setParam(node, "Mute", muted ? 1.0 : 0.0);
    else host().setTrackMuted(node, muted);
}

void SongView::enterBox(const std::string& node) {
    if (node.empty()) return;
    if (!inBox()) { boxPpb_ = view_.ppb; boxScroll_ = view_.scrollBeats; }
    boxNode_ = node;
    boxWasExpanded_ = expanded_.count(node) != 0;
    view_.sel.active = false;
    clearClipSel();
    clearPointSelection();
    selBox_ = -1;
    double s = 1e18, e = -1e18;
    for (const auto& b : host().automation().boxes())
        if (b.organism == node) { s = std::min(s, b.startBeat); e = std::max(e, b.endBeat); }
    if (const auto* cm = host().model().byName(node))
        for (const auto& l : cm->automation)
            for (const auto& pt : l.points) { s = std::min(s, pt.beat); e = std::max(e, pt.beat); }
    if (e > s) {
        const double w = std::max(60, getWidth() - kStripW) * 0.9;
        view_.ppb = juce::jlimit(2.0, 600.0, w / std::max(1.0, e - s));
        view_.scrollBeats = std::max(0.0, s - (w / 0.9 - w) * 0.5 / view_.ppb);
    }
    rebuild();
}

void SongView::leaveBox() {
    if (!inBox()) return;
    if (!boxWasExpanded_) expanded_.erase(boxNode_);
    boxNode_.clear();
    view_.ppb = boxPpb_;
    view_.scrollBeats = boxScroll_;
    lineSlot_ = -1;
    rebuild();
}

void SongView::clearSelections() {
    clearClipSel();
    clearSelection();
    clearPointSelection();
}

std::vector<std::string> SongView::liveTargets() const {
    std::vector<std::string> t;
    for (const auto& n : rows_)
        if (!host().nodeRecordsAudio(n) && host().midi().isRecordTarget(n)) t.push_back(n);
    if (t.empty())
        if (const auto sel = selectedNoteNode(); !sel.empty()) t.push_back(sel);
    return t;
}

std::vector<std::string> SongView::noteRows() const {
    std::vector<std::string> out;
    for (const auto& n : rows_)
        if (arrangeable_.count(n) != 0 && !host().nodeRecordsAudio(n)) out.push_back(n);
    return out;
}

void SongView::expandRow(const std::string& node) {
    expanded_.insert(node);
    rebuildSlots();
    repaintAll();
}

void SongView::expandAll() {
    for (auto& n : rows_) expanded_.insert(n);
    rebuildSlots();
    repaintAll();
}

void SongView::replaceSpan(const std::string& node, const std::string& param, double from,
                             double to, std::vector<AutomationBreakpoint> add, bool openFrom) {
    const auto* cm = host().model().byName(node);
    if (cm == nullptr) return;
    std::vector<AutomationBreakpoint> pts;
    for (const auto& l : cm->automation)
        if (l.propertyName == param) { pts = l.points; break; }
    pts.erase(std::remove_if(pts.begin(), pts.end(), [&](const AutomationBreakpoint& b) {
                  return (openFrom ? b.beat > from : b.beat >= from) && b.beat <= to;
              }), pts.end());
    pts.insert(pts.end(), add.begin(), add.end());
    std::stable_sort(pts.begin(), pts.end(),
                     [](const AutomationBreakpoint& a, const AutomationBreakpoint& b) {
                         return a.beat < b.beat;
                     });
    host().automation().setPoints(node, param, pts);
}

void SongView::commitLine(const std::string& node, const std::string& param,
                            double b0, double v0, double b1, double v1) {
    if (b1 < b0) { std::swap(b0, b1); std::swap(v0, v1); }
    AutomationBreakpoint a, b;
    a.beat = b0; a.value = a.valueMax = v0;
    b.beat = b1; b.value = b.valueMax = v1;
    if (b1 - b0 < 1e-6) replaceSpan(node, param, b0, b0, {a});
    else replaceSpan(node, param, b0, b1, {a, b});
}

void SongView::pencilStep(const std::string& node, const std::string& param, double beat,
                            double v) {
    AutomationBreakpoint p;
    p.beat = beat; p.value = p.valueMax = v;
    if (pencilLast_ < 0.0 || pencilLast_ == beat) replaceSpan(node, param, beat, beat, {p});
    else if (beat > pencilLast_) replaceSpan(node, param, pencilLast_, beat, {p}, true);
    else {
        replaceSpan(node, param, beat, pencilLast_, {p});
        AutomationBreakpoint keep;
        keep.beat = pencilLast_;
        keep.value = keep.valueMax = lastPencilVal_;
        replaceSpan(node, param, pencilLast_, pencilLast_, {keep});
    }
    lastPencilVal_ = v;
    pencilLast_ = beat;
}

}
