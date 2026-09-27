// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/tracks/SongView.h"

#include <algorithm>
#include <cstdlib>

#include "core/graph/PodModel.h"
#include "core/packs/Roles.h"
#include "core/timeline/RowOrder.h"

namespace hum {

namespace {
constexpr int kRowDragSlop = 4;
constexpr int kRowDragScrollEdge = 14;
constexpr int kRowDragScrollStep = 12;
}

std::string SongView::trackPlaying(const std::string& instrument) const {
    for (const auto& c : host().model().midiConnections) {
        if (c.dst != instrument || arrangeable_.count(c.src) == 0) continue;
        if (const auto* cm = host().model().byName(c.src);
            cm != nullptr && classHasRole(cm->classRaw, role::kMidiTrack))
            return c.src;
    }
    return {};
}

std::vector<std::string> SongView::arrangedRows(const std::vector<std::string>& natural) const {
    std::vector<roworder::Row> rows;
    for (const auto& name : natural) {
        roworder::Row r;
        r.name = name;
        if (const auto* cm = host().model().byName(name)) r.index = cm->timelineRow;
        r.pod = pods::childPodOf(name, "");
        if (arrangeable_.count(name) == 0) r.above = trackPlaying(name);
        rows.push_back(std::move(r));
    }
    return roworder::arranged(rows);
}

bool SongView::storeRowOrder(const std::vector<std::string>& order) {
    bool changed = false;
    for (int i = 0; i < (int) order.size(); ++i)
        if (auto* cm = host().model().byName(order[(size_t) i]); cm != nullptr && cm->timelineRow != i) {
            cm->timelineRow = i;
            changed = true;
        }
    if (changed) host().markDirty();
    return changed;
}

void SongView::placeRowBelow(const std::string& node, const std::string& anchor) {
    if (node.empty() || inBox()) return;
    rebuild();
    storeRowOrder(roworder::placedBelow(rows_, node, anchor));
    rebuild();
}

std::string SongView::newTrackAnchor() const {
    if (selClipRow_ < 0 || selClipRow_ >= (int) rows_.size()) return {};
    return rows_[(size_t) selClipRow_];
}

std::vector<roworder::Extent> SongView::rowExtents() const {
    std::vector<roworder::Extent> out(rows_.size());
    for (const auto& s : slots_) {
        if (s.track < 0 || s.track >= (int) out.size()) continue;
        auto& e = out[(size_t) s.track];
        if (!e.shown()) e = {s.y, s.y + s.h};
        else e = {std::min(e.top, s.y), std::max(e.bottom, s.y + s.h)};
    }
    return out;
}

void SongView::armRowDrag(int row, juce::Point<int> p) {
    rowDrag_ = {};
    if (inBox() || row < 0 || row >= (int) rows_.size()) return;
    rowDrag_.armed = true;
    rowDrag_.node = rows_[(size_t) row];
    rowDrag_.downY = p.y;
}

std::set<std::string> SongView::rowsRidingDrag() const {
    if (selTracks_.count(rowDrag_.node) != 0 && selTracks_.size() > 1) return selTracks_;
    return {rowDrag_.node};
}

void SongView::dragRows(juce::Point<int> p) {
    if (!rowDrag_.live && std::abs(p.y - rowDrag_.downY) < kRowDragSlop) return;
    rowDrag_.live = true;
    if (p.y < headerH() + kRowDragScrollEdge) applyVScroll(view_.vScroll - kRowDragScrollStep);
    else if (p.y > getBottom() - kRowDragScrollEdge) applyVScroll(view_.vScroll + kRowDragScrollStep);
    rowDrag_.before = roworder::dropBefore(rowExtents(), p.y);
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    repaintAll();
}

bool SongView::endRowDrag() {
    const auto drag = rowDrag_;
    const auto riding = rowsRidingDrag();
    rowDrag_ = {};
    if (!drag.armed) return false;
    setMouseCursor(juce::MouseCursor::NormalCursor);
    if (!drag.live) return false;
    if (roworder::wouldMove(rows_, riding, drag.before)) {
        host().pushUndo();
        storeRowOrder(roworder::moved(rows_, riding, drag.before));
        rebuild();
        for (int r = 0; r < (int) rows_.size(); ++r)
            if (rows_[(size_t) r] == drag.node) selClipRow_ = r;
        ctx_.patchChanged();
    }
    repaintAll();
    return true;
}

bool SongView::cancelRowDrag() {
    if (!rowDrag_.live) return false;
    rowDrag_ = {};
    setMouseCursor(juce::MouseCursor::NormalCursor);
    repaintAll();
    return true;
}

void SongView::paintRowDrag(juce::Graphics& g) {
    if (!rowDrag_.live) return;
    const auto extents = rowExtents();
    const auto riding = rowsRidingDrag();
    g.setColour(Palette::background.withAlpha(alpha::mid));
    for (int r = 0; r < (int) rows_.size(); ++r)
        if (riding.count(rows_[(size_t) r]) != 0 && extents[(size_t) r].shown())
            g.fillRect(0, extents[(size_t) r].top, getWidth(),
                       extents[(size_t) r].bottom - extents[(size_t) r].top);
    if (!roworder::wouldMove(rows_, riding, rowDrag_.before)) return;
    int y = headerH();
    for (int r = 0; r < (int) extents.size(); ++r) {
        if (!extents[(size_t) r].shown()) continue;
        if (r < rowDrag_.before) y = extents[(size_t) r].bottom;
        else { y = extents[(size_t) r].top; break; }
    }
    g.setColour(Palette::accent);
    g.fillRect(0, y - 1, getWidth(), 3);
}

int SongView::rowOfForTest(const std::string& node) const {
    return roworder::positionOf(rows_, node);
}

}
