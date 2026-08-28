#include "gui/TracksPane.h"

#include <algorithm>
#include <map>

#include "core/PerfBox.h"
#include "gui/EngineHost.h"

namespace hum {

using timeline::ItemRef;

std::vector<ItemRef> TracksPane::ClipItems::all(int row) const {
    std::vector<ItemRef> out;
    for (const auto& ci : pane_.host_.clips().list(pane_.rows_[(size_t) row]))
        out.push_back({ItemRef::Kind::Clip, row, ci.id});
    return out;
}

juce::Rectangle<int> TracksPane::ClipItems::bounds(const ItemRef& r) const {
    for (const auto& ci : pane_.host_.clips().list(pane_.rows_[(size_t) r.row]))
        if (ci.id == r.key) return pane_.clipBounds(r.row, ci);
    return {};
}

bool TracksPane::ClipItems::alive(const ItemRef& r) const {
    return r.row >= 0 && r.row < (int) pane_.rows_.size()
           && pane_.clipIndexOfId(pane_.rows_[(size_t) r.row], r.key) >= 0;
}

void TracksPane::ClipItems::remove(const std::vector<ItemRef>& refs) {
    for (const auto& r : refs)
        if (const int at = pane_.clipIndexOfId(pane_.rows_[(size_t) r.row], r.key); at >= 0)
            pane_.host_.clips().remove(pane_.rows_[(size_t) r.row], at);
}

void TracksPane::ClipItems::duplicateAfter(const std::vector<ItemRef>&) {
    pane_.duplicateSelectedClips();
}

int TracksPane::ClipItems::merge(const std::vector<timeline::ItemRef>& refs) {
    auto& host_ = pane_.host_;
    auto& rows_ = pane_.rows_;
    std::vector<std::pair<int, int>> sel;
    for (const auto& r : refs)
        if (const int at = pane_.clipIndexOfId(rows_[(size_t) r.row], r.key); at >= 0) sel.push_back({r.row, at});
    int joined = 0;
    if (sel.size() >= 2) {
        for (int r = 0; r < (int) rows_.size(); ++r) {
            const auto& node = rows_[(size_t) r];
            std::vector<int> ids;
            {
                const auto cs = host_.clips().list(node);
                for (const auto& [row, ord] : sel)
                    if (row == r && ord < (int) cs.size()) ids.push_back(cs[(size_t) ord].id);
            }
            if (ids.size() < 2) continue;
            auto ordinalOf = [&](int id) { return pane_.clipIndexOfId(node, id); };
            std::sort(ids.begin(), ids.end(), [&](int x, int y) {
                const auto cs = host_.clips().list(node);
                return cs[(size_t) ordinalOf(x)].startTick < cs[(size_t) ordinalOf(y)].startTick;
            });
            std::vector<int> left;
            int head = ids[0];
            for (size_t k = 1; k < ids.size(); ++k) {
                const int a = ordinalOf(head), b = ordinalOf(ids[k]);
                if (a >= 0 && b >= 0 && host_.clips().join(node, a, b) >= 0) { ++joined; continue; }
                left.push_back(head);
                head = ids[k];
            }
            left.push_back(head);
            if (left.size() >= 2) {
                std::vector<int> ords;
                for (int id : left) if (ordinalOf(id) >= 0) ords.push_back(ordinalOf(id));
                if (host_.clips().mergeAudio(node, ords) >= 0) joined += (int) ords.size() - 1;
            }
        }
    }
    return joined;
}

std::vector<ItemRef> TracksPane::BoxItems::all(int row) const {
    std::vector<ItemRef> out;
    const auto& boxes = pane_.host_.automation().boxes();
    for (int i = 0; i < (int) boxes.size(); ++i)
        if (boxes[(size_t) i].organism == pane_.rows_[(size_t) row]) out.push_back({ItemRef::Kind::Box, row, i});
    return out;
}

juce::Rectangle<int> TracksPane::BoxItems::bounds(const ItemRef& r) const {
    const auto& boxes = pane_.host_.automation().boxes();
    if (r.key < 0 || r.key >= (int) boxes.size()) return {};
    return pane_.boxBounds(r.row, boxes[(size_t) r.key]);
}

bool TracksPane::BoxItems::alive(const ItemRef& r) const {
    return r.key >= 0 && r.key < (int) pane_.host_.automation().boxes().size();
}

void TracksPane::BoxItems::remove(const std::vector<ItemRef>& refs) {
    std::vector<int> idx;
    for (const auto& r : refs) idx.push_back(r.key);
    std::sort(idx.rbegin(), idx.rend());
    for (int i : idx)
        if (i < (int) pane_.host_.automation().boxes().size()) pane_.host_.automation().deleteBox(i);
    pane_.selBox_ = -1;
}

void TracksPane::BoxItems::duplicateAfter(const std::vector<ItemRef>& refs) {
    std::vector<int> idx;
    for (const auto& r : refs) idx.push_back(r.key);
    std::sort(idx.rbegin(), idx.rend());
    for (int i : idx) {
        const auto& boxes = pane_.host_.automation().boxes();
        if (i < (int) boxes.size()) pane_.host_.automation().duplicateBox(i, boxes[(size_t) i].endBeat);
    }
}

int TracksPane::BoxItems::merge(const std::vector<ItemRef>& refs) {
    if (refs.size() < 2) return 0;
    std::map<std::string, std::pair<double, double>> span;
    const auto& boxes = pane_.host_.automation().boxes();
    for (const auto& r : refs) {
        if (r.key >= (int) boxes.size()) continue;
        const auto& b = boxes[(size_t) r.key];
        auto it = span.find(b.organism);
        if (it == span.end()) span[b.organism] = {b.startBeat, b.endBeat};
        else { it->second.first = std::min(it->second.first, b.startBeat);
               it->second.second = std::max(it->second.second, b.endBeat); }
    }
    for (const auto& [org, se] : span)
        hum::perfbox::addSpan(pane_.host_.model().perfBoxes, org, se.first, se.second);
    pane_.host_.markDirty();
    pane_.selBox_ = -1;
    return (int) span.size();
}

ItemRef TracksPane::clipRef(int row, int clipOrdinal) const {
    const auto cs = host_.clips().list(rows_[(size_t) row]);
    return {ItemRef::Kind::Clip, row, clipOrdinal < (int) cs.size() ? cs[(size_t) clipOrdinal].id : -1};
}

bool TracksPane::boxSelected(int box) const {
    for (const auto& r : sel_) if (r.kind == ItemRef::Kind::Box && r.key == box) return true;
    return false;
}

int TracksPane::selectedClipsN() const {
    return (int) std::count_if(sel_.begin(), sel_.end(), [](const ItemRef& r) { return r.kind == ItemRef::Kind::Clip; });
}

int TracksPane::selectedBoxesN() const {
    return (int) std::count_if(sel_.begin(), sel_.end(), [](const ItemRef& r) { return r.kind == ItemRef::Kind::Box; });
}

void TracksPane::toggleSelected(const ItemRef& r) {
    if (!sel_.insert(r).second) sel_.erase(r);
}

std::vector<ItemRef> TracksPane::selectedOf(ItemRef::Kind k) const {
    std::vector<ItemRef> out;
    for (const auto& r : sel_) if (r.kind == k) out.push_back(r);
    return out;
}

void TracksPane::marqueeSelect(juce::Rectangle<int> area) {
    sel_.clear();
    for (auto* kind : kinds())
        for (int r = 0; r < (int) rows_.size(); ++r)
            for (const auto& ref : kind->all(r))
                if (area.intersects(kind->bounds(ref))) sel_.insert(ref);
}

bool TracksPane::deleteSelection() {
    if (sel_.empty()) return false;
    host_.beginTransaction();
    host_.pushUndo();
    for (auto* kind : kinds()) {
        const auto refs = selectedOf(kind->kind());
        if (!refs.empty()) kind->remove(refs);
    }
    host_.endTransaction();
    sel_.clear();
    clearClipSel();
    rebuild();
    return true;
}

bool TracksPane::duplicateSelection() {
    if (sel_.empty()) return false;
    for (auto* kind : kinds()) {
        const auto refs = selectedOf(kind->kind());
        if (!refs.empty()) kind->duplicateAfter(refs);
    }
    rebuild();
    return true;
}

int TracksPane::mergeSelection() {
    int seams = 0;
    host_.pushUndo();
    for (auto* kind : kinds()) {
        const auto refs = selectedOf(kind->kind());
        if (refs.size() >= 2) seams += kind->merge(refs);
    }
    sel_.clear();
    clearClipSel();
    rebuild();
    return seams;
}

void TracksPane::selectAll() {
    sel_.clear();
    for (auto* kind : kinds())
        for (int r = 0; r < (int) rows_.size(); ++r)
            for (const auto& ref : kind->all(r)) sel_.insert(ref);
    syncTimeSelection();
    repaint();
}

}
