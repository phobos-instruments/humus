// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/tracks/SongView.h"

#include <algorithm>
#include <map>

#include "core/graph/PerfBox.h"
#include "gui/host/TracksHost.h"
#include "gui/common/Localisation.h"

namespace hum {

using timeline::ItemRef;

std::vector<ItemRef> SongView::ClipItems::all(int row) const {
    std::vector<ItemRef> out;
    for (const auto& ci : song_.host().clips().list(song_.rows_[(size_t) row]))
        out.push_back({ItemRef::Kind::Clip, row, ci.id});
    return out;
}

juce::Rectangle<int> SongView::ClipItems::bounds(const ItemRef& r) const {
    for (const auto& ci : song_.host().clips().list(song_.rows_[(size_t) r.row]))
        if (ci.id == r.key) return song_.clipBounds(r.row, ci);
    return {};
}

bool SongView::ClipItems::alive(const ItemRef& r) const {
    return r.row >= 0 && r.row < (int) song_.rows_.size()
           && song_.clipIndexOfId(song_.rows_[(size_t) r.row], r.key) >= 0;
}

void SongView::ClipItems::remove(const std::vector<ItemRef>& refs) {
    for (const auto& r : refs)
        if (const int at = song_.clipIndexOfId(song_.rows_[(size_t) r.row], r.key); at >= 0)
            song_.host().clips().remove(song_.rows_[(size_t) r.row], at);
}

void SongView::ClipItems::duplicateAfter(const std::vector<ItemRef>&) {
    song_.duplicateSelectedClips();
}

int SongView::ClipItems::merge(const std::vector<timeline::ItemRef>& refs) {
    auto& host = song_.host();
    auto& rows_ = song_.rows_;
    std::vector<std::pair<int, int>> sel;
    for (const auto& r : refs)
        if (const int at = song_.clipIndexOfId(rows_[(size_t) r.row], r.key); at >= 0) sel.push_back({r.row, at});
    int joined = 0;
    if (sel.size() >= 2) {
        for (int r = 0; r < (int) rows_.size(); ++r) {
            const auto& node = rows_[(size_t) r];
            std::vector<int> ids;
            {
                const auto cs = host.clips().list(node);
                for (const auto& [row, ord] : sel)
                    if (row == r && ord < (int) cs.size()) ids.push_back(cs[(size_t) ord].id);
            }
            if (ids.size() < 2) continue;
            auto ordinalOf = [&](int id) { return song_.clipIndexOfId(node, id); };
            std::sort(ids.begin(), ids.end(), [&](int x, int y) {
                const auto cs = host.clips().list(node);
                return cs[(size_t) ordinalOf(x)].startTick < cs[(size_t) ordinalOf(y)].startTick;
            });
            std::vector<int> left;
            int head = ids[0];
            for (size_t k = 1; k < ids.size(); ++k) {
                const int a = ordinalOf(head), b = ordinalOf(ids[k]);
                if (a >= 0 && b >= 0 && host.clips().join(node, a, b) >= 0) { ++joined; continue; }
                left.push_back(head);
                head = ids[k];
            }
            left.push_back(head);
            if (left.size() >= 2) {
                std::vector<int> ords;
                for (int id : left) if (ordinalOf(id) >= 0) ords.push_back(ordinalOf(id));
                const auto all = host.clips().list(node);
                const bool picture = std::all_of(ords.begin(), ords.end(), [&](int o) {
                    return o < (int) all.size() && (all[(size_t) o].isVideo || all[(size_t) o].isCompound);
                });
                const int made = picture ? host.clips().makeCompound(node, ords)
                                         : host.clips().mergeAudio(node, ords);
                if (made >= 0) joined += (int) ords.size() - 1;
            }
        }
    }
    return joined;
}

std::vector<ItemRef> SongView::BoxItems::all(int row) const {
    std::vector<ItemRef> out;
    const auto& boxes = song_.host().automation().boxes();
    for (int i = 0; i < (int) boxes.size(); ++i)
        if (boxes[(size_t) i].organism == song_.rows_[(size_t) row]) out.push_back({ItemRef::Kind::Box, row, i});
    return out;
}

juce::Rectangle<int> SongView::BoxItems::bounds(const ItemRef& r) const {
    const auto& boxes = song_.host().automation().boxes();
    if (r.key < 0 || r.key >= (int) boxes.size()) return {};
    return song_.boxBounds(r.row, boxes[(size_t) r.key]);
}

bool SongView::BoxItems::alive(const ItemRef& r) const {
    return r.key >= 0 && r.key < (int) song_.host().automation().boxes().size();
}

void SongView::BoxItems::remove(const std::vector<ItemRef>& refs) {
    std::vector<int> idx;
    for (const auto& r : refs) idx.push_back(r.key);
    std::sort(idx.rbegin(), idx.rend());
    for (int i : idx)
        if (i < (int) song_.host().automation().boxes().size()) song_.host().automation().deleteBox(i);
    song_.selBox_ = -1;
}

void SongView::BoxItems::duplicateAfter(const std::vector<ItemRef>& refs) {
    std::vector<int> idx;
    for (const auto& r : refs) idx.push_back(r.key);
    std::sort(idx.rbegin(), idx.rend());
    for (int i : idx) {
        const auto& boxes = song_.host().automation().boxes();
        if (i < (int) boxes.size()) song_.host().automation().duplicateBox(i, boxes[(size_t) i].endBeat);
    }
}

int SongView::BoxItems::merge(const std::vector<ItemRef>& refs) {
    if (refs.size() < 2) return 0;
    std::map<std::string, std::pair<double, double>> span;
    const auto& boxes = song_.host().automation().boxes();
    for (const auto& r : refs) {
        if (r.key >= (int) boxes.size()) continue;
        const auto& b = boxes[(size_t) r.key];
        auto it = span.find(b.organism);
        if (it == span.end()) span[b.organism] = {b.startBeat, b.endBeat};
        else { it->second.first = std::min(it->second.first, b.startBeat);
               it->second.second = std::max(it->second.second, b.endBeat); }
    }
    for (const auto& [org, se] : span)
        hum::perfbox::addSpan(song_.host().model().perfBoxes, org, se.first, se.second);
    song_.host().markDirty();
    song_.selBox_ = -1;
    return (int) span.size();
}

ItemRef SongView::clipRef(int row, int clipOrdinal) const {
    const auto cs = host().clips().list(rows_[(size_t) row]);
    return {ItemRef::Kind::Clip, row, clipOrdinal < (int) cs.size() ? cs[(size_t) clipOrdinal].id : -1};
}

bool SongView::boxSelected(int box) const {
    for (const auto& r : sel_) if (r.kind == ItemRef::Kind::Box && r.key == box) return true;
    return false;
}

int SongView::selectedClipsN() const {
    return (int) std::count_if(sel_.begin(), sel_.end(), [](const ItemRef& r) { return r.kind == ItemRef::Kind::Clip; });
}

int SongView::selectedBoxesN() const {
    return (int) std::count_if(sel_.begin(), sel_.end(), [](const ItemRef& r) { return r.kind == ItemRef::Kind::Box; });
}

void SongView::toggleSelected(const ItemRef& r) {
    if (!sel_.insert(r).second) sel_.erase(r);
}

std::vector<ItemRef> SongView::selectedOf(ItemRef::Kind k) const {
    std::vector<ItemRef> out;
    for (const auto& r : sel_) if (r.kind == k) out.push_back(r);
    return out;
}

void SongView::marqueeSelect(juce::Rectangle<int> area) {
    sel_.clear();
    for (auto* kind : kinds())
        for (int r = 0; r < (int) rows_.size(); ++r)
            for (const auto& ref : kind->all(r))
                if (area.intersects(kind->bounds(ref))) sel_.insert(ref);
}

bool SongView::deleteSelection() {
    if (sel_.empty()) return false;
    host().beginTransaction();
    host().pushUndo();
    for (auto* kind : kinds()) {
        const auto refs = selectedOf(kind->kind());
        if (!refs.empty()) kind->remove(refs);
    }
    host().endTransaction();
    sel_.clear();
    clearClipSel();
    rebuild();
    return true;
}

bool SongView::duplicateSelection() {
    if (sel_.empty()) return false;
    for (auto* kind : kinds()) {
        const auto refs = selectedOf(kind->kind());
        if (!refs.empty()) kind->duplicateAfter(refs);
    }
    rebuild();
    return true;
}

juce::String SongView::mergeRefusal() const {
    std::map<int, std::vector<int>> perRow;
    for (const auto& r : sel_)
        if (r.kind == timeline::ItemRef::Kind::Clip) perRow[r.row].push_back(r.key);
    int row = -1;
    for (const auto& [r, ids] : perRow)
        if (ids.size() >= 2 && row < 0) row = r;
    if (row < 0 || row >= (int) rows_.size())
        return tr("tracks-pane-menu.merge-one-row",
                  "Select two or more clips on one row, or two or more automation boxes.");

    const auto& node = rows_[(size_t) row];
    const auto all = host().clips().list(node);
    std::vector<ClipEditor::ClipInfo> chosen;
    for (const int id : perRow[row])
        if (const int at = clipIndexOfId(node, id); at >= 0 && at < (int) all.size())
            chosen.push_back(all[(size_t) at]);
    std::sort(chosen.begin(), chosen.end(),
              [](const auto& a, const auto& b) { return a.startTick < b.startTick; });
    if (chosen.size() < 2)
        return tr("tracks-pane-menu.merge-one-row",
                  "Select two or more clips on one row, or two or more automation boxes.");

    if (!std::any_of(chosen.begin(), chosen.end(),
                     [](const auto& c) { return c.isVideo || c.isCompound; }))
        return tr("tracks-pane-menu.merge-failed",
                  "Those clips could not be merged into one take.");
    return tr("tracks-pane-menu.merge-failed-video",
              "Those clips could not be made into a reel.");
}

int SongView::mergeSelection() {
    int seams = 0;
    host().pushUndo();
    for (auto* kind : kinds()) {
        const auto refs = selectedOf(kind->kind());
        if (refs.size() >= 2) seams += kind->merge(refs);
    }
    sel_.clear();
    clearClipSel();
    rebuild();
    return seams;
}

void SongView::selectAll() {
    sel_.clear();
    for (auto* kind : kinds())
        for (int r = 0; r < (int) rows_.size(); ++r)
            for (const auto& ref : kind->all(r)) sel_.insert(ref);
    syncTimeSelection();
    repaintAll();
}

}
