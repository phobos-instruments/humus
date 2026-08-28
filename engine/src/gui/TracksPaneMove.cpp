#include "gui/TracksPane.h"

#include <algorithm>
#include <climits>
#include <cmath>

#include "gui/EngineHost.h"

namespace hum {

int TracksPane::beginClipMove(int row, int clip, bool duplicate) {
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
            const auto clips = host_.clips().list(node);
            if (c < 0 || c >= (int) clips.size()) continue;
            const int copy = host_.clips().duplicate(node, c, clips[(size_t) c].startTick);
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
        const auto clips = host_.clips().list(rows_[(size_t) r]);
        if (c < 0 || c >= (int) clips.size()) continue;
        const auto& ci = clips[(size_t) c];
        moveBase_.push_back({r, r, ci.id, ci.startTick});
        if (r == row && c == grab) moveGrabId_ = ci.id;
    }
    return moveBase_.empty() ? -1 : grab;
}

bool TracksPane::rowShiftFits(int deltaRows) {
    for (const auto& m : moveBase_) {
        const int want = m.row + deltaRows;
        if (want < 0 || want >= (int) rows_.size()) return false;
        if (want == m.curRow) continue;
        const auto clips = host_.clips().list(rows_[(size_t) m.curRow]);
        const int at = clipIndexOfId(rows_[(size_t) m.curRow], m.id);
        if (at < 0 || at >= (int) clips.size()) return false;
        if (!host_.clips().accepts(rows_[(size_t) want], clips[(size_t) at].isAudio)) return false;
    }
    return true;
}

void TracksPane::moveSelection(int deltaTicks, int deltaRows) {
    if (moveBase_.empty()) return;
    int floorTick = INT_MAX;
    for (const auto& m : moveBase_) floorTick = std::min(floorTick, m.startTick);
    deltaTicks = std::max(deltaTicks, -floorTick);
    if (deltaRows != 0 && !rowShiftFits(deltaRows)) deltaRows = 0;

    host_.beginTransaction();
    for (auto& m : moveBase_) {
        int at = clipIndexOfId(rows_[(size_t) m.curRow], m.id);
        if (at < 0) continue;
        if (const int want = m.row + deltaRows; want != m.curRow) {
            const int nc = host_.clips().moveToNode(rows_[(size_t) m.curRow], at,
                                                    rows_[(size_t) want]);
            if (nc < 0) continue;
            m.curRow = want;
            at = nc;
        }
        host_.clips().move(rows_[(size_t) m.curRow], at, m.startTick + deltaTicks);
    }
    host_.endTransaction();

    sel_.clear();
    for (const auto& m : moveBase_) sel_.insert({timeline::ItemRef::Kind::Clip, m.curRow, m.id});
    for (const auto& m : moveBase_)
        if (m.id == moveGrabId_) {
            dragRow_ = m.curRow;
            dragClip_ = clipIndexOfId(rows_[(size_t) m.curRow], m.id);
        }
    syncTimeSelection();
}

void TracksPane::restoreClipMove() {
    host_.beginTransaction();
    for (auto& m : moveBase_) {
        int at = clipIndexOfId(rows_[(size_t) m.curRow], m.id);
        if (at < 0) continue;
        if (m.curRow != m.row) {
            const int back = host_.clips().moveToNode(rows_[(size_t) m.curRow], at,
                                                      rows_[(size_t) m.row]);
            if (back < 0) continue;
            m.curRow = m.row;
            at = back;
        }
        host_.clips().move(rows_[(size_t) m.row], at, m.startTick);
    }
    host_.endTransaction();
    sel_.clear();
    for (const auto& m : moveBase_) sel_.insert({timeline::ItemRef::Kind::Clip, m.row, m.id});
    moveBase_.clear();
}

void TracksPane::syncTimeSelection() {
    if (mode_ != Mode::Song) return;
    const auto list = selectedClipList();
    if (list.empty()) {
        if (drag_ != Drag::TimeSelect) hasSel_ = false;
        return;
    }
    int first = INT_MAX, last = INT_MIN;
    for (const auto& [row, clip] : list)
        for (const auto& ci : host_.clips().list(rows_[(size_t) row]))
            if (ci.index == clip) {
                first = std::min(first, ci.startTick);
                last = std::max(last, ci.startTick + ci.lengthTicks);
            }
    if (first == INT_MAX) { hasSel_ = false; return; }
    const double tpb = Pattern::kTicksPerBeat;
    double from = first / tpb, to = last / tpb;
    if (snapChoice_ >= 0.0) {
        const double grid = snapChoice_ > 0.0
                                ? snapChoice_
                                : juce::jmax(1.0, (double) host_.automation().timeSigNumerator());
        from = std::floor(from / grid + 1e-6) * grid;
        to = std::ceil(to / grid - 1e-6) * grid;
    }
    selFrom_ = std::max(0.0, from);
    selTo_ = std::max(selFrom_ + 1e-9, to);
    hasSel_ = true;
}

bool TracksPane::edgeScroll(juce::Point<int> p) {
    constexpr int kMargin = 30, kStep = 14;
    const int left = kStripW, right = getWidth() - kZoomGut;
    double reach = 0.0;
    if (p.x > right - kMargin)     reach = (p.x - (right - kMargin)) / (double) kMargin;
    else if (p.x < left + kMargin) reach = (p.x - (left + kMargin)) / (double) kMargin;

    int rows = 0;
    if (mode_ == Mode::Song || mode_ == Mode::Box) {
        if (p.y > fieldBottom() - kMargin) rows = 1;
        else if (p.y < headerH() + kMargin && vScroll_ > 0) rows = -1;
    }
    if (std::abs(reach) < 1e-9 && rows == 0) {
        juce::Desktop::getInstance().beginDragAutoRepeat(0);
        return false;
    }
    const double was = scrollBeats_;
    scrollBeats_ = std::max(0.0, scrollBeats_
                                     + juce::jlimit(-1.0, 1.0, reach) * kStep / ppb_);
    if (rows != 0) applyVScroll(vScroll_ + rows * 12);
    juce::Desktop::getInstance().beginDragAutoRepeat(16);
    if (scrollBeats_ != was) repaint();
    return true;
}

int TracksPane::selectionSpanTicks() const {
    if (!hasSel_ || selTo_ <= selFrom_) return 0;
    return (int) std::llround((selTo_ - selFrom_) * Pattern::kTicksPerBeat);
}

}  // namespace hum
