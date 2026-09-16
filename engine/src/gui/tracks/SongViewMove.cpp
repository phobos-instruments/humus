// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/tracks/SongView.h"

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

    sel_.clear();
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
    sel_.clear();
    for (const auto& m : moveBase_) sel_.insert({timeline::ItemRef::Kind::Clip, m.row, m.id});
    moveBase_.clear();
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
