// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include <algorithm>
#include <cmath>
#include <iterator>
#include <map>
#include <set>
#include <utility>

#include "core/timeline/BlockLane.h"
#include "gui/tracks/SongView.h"

namespace hum {

using timeline::ItemRef;

namespace {

constexpr double kStartTolerance = 0.5 / Pattern::kTicksPerBeat;

const AutomationLane* laneNamed(const OrganismModel* cm, const std::string& param) {
    if (cm == nullptr) return nullptr;
    for (const auto& l : cm->automation)
        if (l.propertyName == param) return &l;
    return nullptr;
}

}

int SongView::faceplateOf(const std::string& node, const std::string& param) const {
    if (const auto* cm = host().model().byName(node))
        for (const auto& p : cm->properties)
            if (p.name == param) return (int) std::lround(p.value);
    return (int) std::lround(laneRange(node, param).first);
}

bool SongView::samePoints(const std::vector<AutomationBreakpoint>& a, const std::vector<AutomationBreakpoint>& b) {
    return std::equal(a.begin(), a.end(), b.begin(), b.end(), [](const auto& x, const auto& y) {
        return x.beat == y.beat && x.value == y.value && x.release == y.release;
    });
}

ItemRef SongView::blockRef(int row, const std::string& lane, double start) const {
    return {ItemRef::Kind::Block, row, (int) std::llround(start * Pattern::kTicksPerBeat), lane};
}

double SongView::blockStartOf(const ItemRef& r) { return (double) r.key / Pattern::kTicksPerBeat; }

std::vector<SongView::BlockRide> SongView::selectedBlockLanes() const {
    std::map<std::pair<int, std::string>, BlockRide> lanes;
    for (const auto& r : sel_) {
        if (r.kind != ItemRef::Kind::Block || r.row < 0 || r.row >= (int) rows_.size()) continue;
        const auto* lane = laneNamed(host().model().byName(rows_[(size_t) r.row]), r.lane);
        if (lane == nullptr
            || blocklane::blockStartingAt(blocklane::blocksOf(lane->points), blockStartOf(r), kStartTolerance) < 0)
            continue;
        auto& ride = lanes[{r.row, r.lane}];
        ride.row = r.row;
        ride.node = rows_[(size_t) r.row];
        ride.param = r.lane;
        ride.before = lane->points;
        ride.starts.push_back(blockStartOf(r));
    }
    std::vector<BlockRide> out;
    for (auto& [key, ride] : lanes) out.push_back(std::move(ride));
    return out;
}

std::vector<ItemRef> SongView::BlockItems::all(int row) const {
    std::vector<ItemRef> out;
    const auto& node = song_.rows_[(size_t) row];
    const auto* cm = song_.host().model().byName(node);
    if (cm == nullptr) return out;
    for (const auto& lane : cm->automation)
        if (song_.isBlockLane(node, lane.propertyName))
            for (const auto& b : blocklane::blocksOf(lane.points))
                out.push_back(song_.blockRef(row, lane.propertyName, b.start));
    return out;
}

juce::Rectangle<int> SongView::BlockItems::bounds(const ItemRef& r) const {
    if (r.row < 0 || r.row >= (int) song_.rows_.size()) return {};
    const auto* lane = laneNamed(song_.host().model().byName(song_.rows_[(size_t) r.row]), r.lane);
    if (lane == nullptr) return {};
    for (const auto& sl : song_.slots_) {
        if (sl.kind != trackslayout::Kind::AutoLane || sl.track != r.row || sl.param != r.lane) continue;
        const auto blocks = blocklane::blocksOf(lane->points);
        const int at = blocklane::blockStartingAt(blocks, blockStartOf(r), kStartTolerance);
        if (at < 0) return {};
        const auto& b = blocks[(size_t) at];
        const int x0 = std::max(kStripW, (int) song_.beatToX(b.start));
        const int x1 = b.open() ? song_.getWidth() : (int) song_.beatToX(b.end);
        return {x0, sl.y + 3, std::max(1, x1 - x0), sl.h - 6};
    }
    return {};
}

bool SongView::BlockItems::alive(const ItemRef& r) const {
    if (r.row < 0 || r.row >= (int) song_.rows_.size()) return false;
    const auto* lane = laneNamed(song_.host().model().byName(song_.rows_[(size_t) r.row]), r.lane);
    return lane != nullptr
           && blocklane::blockStartingAt(blocklane::blocksOf(lane->points), blockStartOf(r), kStartTolerance) >= 0;
}

void SongView::BlockItems::remove(const std::vector<ItemRef>& refs) {
    std::set<ItemRef> chosen(refs.begin(), refs.end());
    for (const auto& ride : song_.selectedBlockLanes()) {
        auto points = ride.before;
        auto starts = ride.starts;
        std::sort(starts.rbegin(), starts.rend());
        for (const double s : starts) {
            if (chosen.count(song_.blockRef(ride.row, ride.param, s)) == 0) continue;
            if (const int at = blocklane::blockStartingAt(blocklane::blocksOf(points), s, kStartTolerance); at >= 0)
                points = blocklane::withoutBlock(points, at, song_.faceplateOf(ride.node, ride.param));
        }
        song_.host().automation().setPoints(ride.node, ride.param, points);
    }
    song_.pruneBlockSelection();
}

void SongView::BlockItems::duplicateAfter(const std::vector<ItemRef>&) {
    const auto lanes = song_.selectedBlockLanes();
    if (lanes.empty()) return;
    song_.host().pushUndo();
    for (const auto& ride : lanes) {
        const auto chosen = blocklane::blocksStartingAt(ride.before, ride.starts, kStartTolerance);
        double first = 1e18, last = -1.0;
        std::vector<double> finite;
        for (const auto& b : chosen)
            if (!b.open()) {
                first = std::min(first, b.start);
                last = std::max(last, b.end);
                finite.push_back(b.start);
            }
        if (finite.empty()) continue;
        const double span = last - first;
        const int fallback = song_.faceplateOf(ride.node, ride.param);
        song_.host().automation().setPoints(
            ride.node, ride.param, blocklane::withBlocksMoved(ride.before, finite, span, true, fallback, kStartTolerance));
        for (const double s : ride.starts) song_.sel_.erase(song_.blockRef(ride.row, ride.param, s));
        for (const double s : finite) song_.sel_.insert(song_.blockRef(ride.row, ride.param, s + span));
    }
}

int SongView::BlockItems::merge(const std::vector<ItemRef>&) {
    int joined = 0;
    for (const auto& ride : song_.selectedBlockLanes()) {
        const auto next = blocklane::withBlocksJoined(ride.before, ride.starts, kStartTolerance);
        joined += (int) (ride.before.size() - next.size());
        song_.host().automation().setPoints(ride.node, ride.param, next);
    }
    return joined;
}

void SongView::rideSelectedBlocks(double deltaBeats, bool copy) {
    if (blockRides_.empty()) {
        blockRides_ = selectedBlockLanes();
        if (blockRides_.empty()) return;
        if (drag_ == Drag::BoxMove && !groupMove_) host().pushUndo();
    }
    blockRideDelta_ = deltaBeats;
    blockRideCopy_ = copy;
    for (const auto& ride : blockRides_) {
        const int fallback = faceplateOf(ride.node, ride.param);
        host().automation().setPoints(ride.node, ride.param,
                                      blocklane::withBlocksMoved(ride.before, ride.starts, deltaBeats, copy, fallback,
                                                                 kStartTolerance));
    }
}

void SongView::endBlockRide() {
    if (std::abs(blockRideDelta_) > 1e-9)
        for (const auto& ride : blockRides_) {
            const double floor = *std::min_element(ride.starts.begin(), ride.starts.end());
            const double delta = std::max(blockRideDelta_, -floor);
            for (const double s : ride.starts) sel_.erase(blockRef(ride.row, ride.param, s));
            for (const double s : ride.starts) sel_.insert(blockRef(ride.row, ride.param, s + delta));
        }
    blockRides_.clear();
    blockRideDelta_ = 0.0;
    blockRideCopy_ = false;
}

bool SongView::setSelectedBlocksBank(const std::string& label, int value) {
    bool any = false;
    for (const auto& ride : selectedBlockLanes()) {
        const auto& labels = blockLabels(ride.node, ride.param);
        int v = value;
        if (!label.empty()) {
            const auto it = std::find_if(labels.begin(), labels.end(), [&](const std::string& l) {
                return juce::String(l).equalsIgnoreCase(juce::String(label));
            });
            if (it == labels.end()) continue;
            v = (int) (it - labels.begin());
        }
        if (!any) host().pushUndo();
        any = true;
        const int fallback = faceplateOf(ride.node, ride.param);
        auto points = ride.before;
        for (const double s : ride.starts)
            if (const int at = blocklane::blockStartingAt(blocklane::blocksOf(points), s, kStartTolerance); at >= 0)
                points = blocklane::withBlockValue(points, at, v, fallback);
        host().automation().setPoints(ride.node, ride.param, points);
    }
    if (any) repaintAll();
    return any;
}

bool SongView::bankKey(const juce::KeyPress& k) {
    const auto mods = k.getModifiers();
    if (mods.isCommandDown() || mods.isCtrlDown() || mods.isAltDown()) return false;
    const auto c = k.getTextCharacter();
    if (c <= ' ' || selectedOf(ItemRef::Kind::Block).empty()) return false;
    return setSelectedBlocksBank(juce::String::charToString(c).toStdString(), 0);
}

void SongView::pruneBlockSelection() {
    for (auto it = sel_.begin(); it != sel_.end();)
        it = it->kind == ItemRef::Kind::Block && !blockItems_.alive(*it) ? sel_.erase(it) : std::next(it);
}

void SongView::pickBlock(int slot, double start, const juce::ModifierKeys& mods) {
    const auto& sl = slots_[(size_t) slot];
    const auto ref = blockRef(sl.track, sl.param, start);
    if (mods.isAltDown()) {
        if (!sel_.insert(ref).second) sel_.erase(ref);
        return;
    }
    if (mods.isShiftDown()) {
        std::vector<double> mine;
        for (const auto& r : sel_)
            if (r.kind == ItemRef::Kind::Block && r.row == sl.track && r.lane == sl.param) mine.push_back(blockStartOf(r));
        sel_.insert(ref);
        if (mine.empty()) return;
        const double lo = std::min(start, *std::min_element(mine.begin(), mine.end()));
        const double hi = std::max(start, *std::max_element(mine.begin(), mine.end()));
        if (const auto* lane = laneOfSlot(sl))
            for (const auto& b : blocklane::blocksOf(lane->points))
                if (b.start >= lo - kStartTolerance && b.start <= hi + kStartTolerance)
                    sel_.insert(blockRef(sl.track, sl.param, b.start));
        return;
    }
    if (selected(ref)) return;
    clearClipSel();
    sel_.clear();
    sel_.insert(ref);
}

}
