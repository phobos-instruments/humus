// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/tracks/SongView.h"
#include <iterator>

#include <algorithm>
#include <climits>
#include <cmath>

#include "gui/host/TracksHost.h"

namespace hum {

int SongView::beginClipMove(int row, int clip, bool duplicate) {
    moveBase_.clear();
    moveGrabRow_ = row;
    moveGrabId_ = 0;
    auto list = selectedClipList();
    if (list.empty()) return -1;

    int grab = clip;
    if (duplicate) {
        std::vector<std::pair<int, int>> made;
        for (const auto& [r, c] : list) {
            const auto& node = rows_[(size_t) r];
            const auto clips = host().clips().list(node);
            if (c < 0 || c >= (int) clips.size()) continue;
            const int copy = host().clips().duplicate(node, c, clips[(size_t) c].startTick);
            if (copy < 0) continue;
            if (r == row && c == clip) grab = copy;
            made.push_back({r, copy});
        }
        if (made.empty()) return -1;
        list = made;
        sel_.clear();
        for (const auto& [r, c] : list) sel_.insert(clipRef(r, c));
    }

    for (const auto& [r, c] : list) {
        const auto clips = host().clips().list(rows_[(size_t) r]);
        if (c < 0 || c >= (int) clips.size()) continue;
        const auto& ci = clips[(size_t) c];
        moveBase_.push_back({r, r, ci.id, ci.startTick});
        if (r == row && c == grab) moveGrabId_ = ci.id;
    }
    return moveBase_.empty() ? -1 : grab;
}

bool SongView::duplicateOnceMoved(juce::Point<int> p) {
    if (!duplicatePending_) return true;
    if (p.getDistanceFrom(dragDownAt_) < kDuplicateSlopPx) return false;
    duplicatePending_ = false;
    const int made = beginClipMove(dragRow_, dragClip_, true);
    if (made < 0) {
        drag_ = Drag::None;
        return false;
    }
    dragClip_ = made;
    dragDuplicated_ = true;
    selectClip(dragRow_, made);
    return true;
}

void SongView::pickWithAlt(int row, int clip) {
    const auto ref = clipRef(row, clip);
    if (selected(ref)) {
        altDrop_ = ref;
        return;
    }
    if (sel_.empty() && selClipRow_ >= 0 && selClip_ >= 0) sel_.insert(clipRef(selClipRow_, selClip_));
    sel_.insert(ref);
}

bool SongView::rowShiftFits(int deltaRows) {
    for (const auto& m : moveBase_) {
        const int want = m.row + deltaRows;
        if (want < 0 || want >= (int) rows_.size()) return false;
        if (want == m.curRow) continue;
        const auto clips = host().clips().list(rows_[(size_t) m.curRow]);
        const int at = clipIndexOfId(rows_[(size_t) m.curRow], m.id);
        if (at < 0 || at >= (int) clips.size()) return false;
        if (!host().clips().accepts(rows_[(size_t) want], clips[(size_t) at])) return false;
    }
    return true;
}

void SongView::moveSelection(int deltaTicks, int deltaRows) {
    if (moveBase_.empty()) return;
    int floorTick = INT_MAX;
    for (const auto& m : moveBase_) floorTick = std::min(floorTick, m.startTick);
    deltaTicks = std::max(deltaTicks, -floorTick);
    if (deltaRows != 0 && !rowShiftFits(deltaRows)) deltaRows = 0;

    host().beginTransaction();
    for (auto& m : moveBase_) {
        int at = clipIndexOfId(rows_[(size_t) m.curRow], m.id);
        if (at < 0) continue;
        if (const int want = m.row + deltaRows; want != m.curRow) {
            const int nc = host().clips().moveToNode(rows_[(size_t) m.curRow], at,
                                                    rows_[(size_t) want]);
            if (nc < 0) continue;
            m.curRow = want;
            at = nc;
        }
        host().clips().move(rows_[(size_t) m.curRow], at, m.startTick + deltaTicks);
    }
    host().endTransaction();

    keepOnlyClipsOutOfSelection();
    for (const auto& m : moveBase_) sel_.insert({timeline::ItemRef::Kind::Clip, m.curRow, m.id});
    for (const auto& m : moveBase_)
        if (m.id == moveGrabId_) {
            dragRow_ = m.curRow;
            dragClip_ = clipIndexOfId(rows_[(size_t) m.curRow], m.id);
        }
    syncTimeSelection();
}

void SongView::restoreClipMove() {
    host().beginTransaction();
    for (auto& m : moveBase_) {
        int at = clipIndexOfId(rows_[(size_t) m.curRow], m.id);
        if (at < 0) continue;
        if (m.curRow != m.row) {
            const int back = host().clips().moveToNode(rows_[(size_t) m.curRow], at,
                                                      rows_[(size_t) m.row]);
            if (back < 0) continue;
            m.curRow = m.row;
            at = back;
        }
        host().clips().move(rows_[(size_t) m.row], at, m.startTick);
    }
    host().endTransaction();
    keepOnlyClipsOutOfSelection();
    for (const auto& m : moveBase_) sel_.insert({timeline::ItemRef::Kind::Clip, m.row, m.id});
    moveBase_.clear();
}

void SongView::keepOnlyClipsOutOfSelection() {
    for (auto it = sel_.begin(); it != sel_.end();)
        it = it->kind == timeline::ItemRef::Kind::Clip ? sel_.erase(it) : std::next(it);
}

bool SongView::boxRidesDrag(int box) const {
    return box == dragBox_ || (groupMove_ && boxSelected(box));
}

double SongView::groupMoveFloorBeats() const {
    double floor = 1e18;
    const auto& boxes = host().automation().boxes();
    for (int i = 0; i < (int) boxes.size(); ++i)
        if (boxRidesDrag(i)) floor = std::min(floor, boxes[(size_t) i].startBeat);
    for (const auto& m : moveBase_)
        floor = std::min(floor, (double) m.startTick / Pattern::kTicksPerBeat);
    return floor > 1e17 ? 0.0 : floor;
}

void SongView::beginGroupMove() {
    const bool mixed = selectedBoxesN() > 0 && selectedClipsN() + selectedBoxesN() > 1;
    if (!mixed || groupMove_) return;
    groupMove_ = true;
    host().beginUndoGroup();
    if (!moveBase_.empty()) return;
    host().pushUndo();
    moveGrabId_ = -1;
    for (const auto& [r, c] : selectedClipList()) {
        const auto clips = host().clips().list(rows_[(size_t) r]);
        if (c >= 0 && c < (int) clips.size())
            moveBase_.push_back({r, r, clips[(size_t) c].id, clips[(size_t) c].startTick});
    }
}

void SongView::endGroupMove() {
    const auto& boxes = host().automation().boxes();
    std::vector<int> riding;
    for (int i = 0; i < (int) boxes.size(); ++i)
        if (boxRidesDrag(i)) riding.push_back(i);
    if (std::abs(boxDragDelta_) > 1e-9 && !riding.empty()) {
        std::sort(riding.begin(), riding.end(), [&](int a, int b) {
            return boxDragDelta_ > 0.0 ? boxes[(size_t) a].startBeat > boxes[(size_t) b].startBeat
                                       : boxes[(size_t) a].startBeat < boxes[(size_t) b].startBeat;
        });
        for (const int i : riding) host().automation().moveBox(i, boxDragDelta_);
        rebuild();
    }
    if (groupMove_) host().endUndoGroup();
    groupMove_ = false;
}

void SongView::syncTimeSelection() {
    if (inBox()) return;
    const auto list = selectedClipList();
    if (list.empty()) {
        if (!view_.sel.dragging) view_.sel.active = false;
        return;
    }
    int first = INT_MAX, last = INT_MIN;
    for (const auto& [row, clip] : list)
        for (const auto& ci : host().clips().list(rows_[(size_t) row]))
            if (ci.index == clip) {
                first = std::min(first, ci.startTick);
                last = std::max(last, ci.startTick + ci.lengthTicks);
            }
    if (first == INT_MAX) { view_.sel.active = false; return; }
    const double tpb = Pattern::kTicksPerBeat;
    double from = first / tpb, to = last / tpb;
    if (view_.snapChoice > 0.0) {
        from = std::floor(from / view_.snapChoice + 1e-6) * view_.snapChoice;
        to = std::ceil(to / view_.snapChoice - 1e-6) * view_.snapChoice;
    } else if (view_.snapChoice == 0.0) {
        const MeterMap meters = host().automation().meterMap();
        from = meters.barStartBefore(from + 1e-6);
        to = std::abs(to - meters.barStartBefore(to)) < 1e-6 ? to : meters.nextBarStart(to - 1e-6);
    }
    view_.sel.from = std::max(0.0, from);
    view_.sel.to = std::max(view_.sel.from + 1e-9, to);
    view_.sel.active = true;
}

int SongView::selectionSpanTicks() const {
    if (!view_.sel.active || view_.sel.to <= view_.sel.from) return 0;
    return (int) std::llround((view_.sel.to - view_.sel.from) * Pattern::kTicksPerBeat);
}

}
