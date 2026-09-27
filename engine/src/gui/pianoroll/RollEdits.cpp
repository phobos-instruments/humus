// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/pianoroll/RollModel.h"

#include <algorithm>
#include <climits>
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum::roll {

std::vector<NoteEvent> RollModel::clipboard_;

int Geometry::clampTick(int tick) const { return noteedit::clampTo(tick, 0, duration - 1); }

int Geometry::snapTick(int tick) const {
    const int s = step();
    return noteedit::clampTo((tick / s) * s, 0, duration - 1);
}

int Geometry::rows() const { return std::max(1, (gridBottom - gridTop) / rowH); }

RectF Geometry::noteBox(const NoteEvent& e) const {
    const float x = tickToX(e.tick);
    return {x, pitchToY(e.pitch), std::max(3.0f, tickToX(e.tick + e.lengthTicks) - x - 1.0f), (float) (rowH - 1)};
}

float Geometry::ccY(int value, int laneCC) const {
    const float span = (float) ccValueMax(laneCC < 0 ? 0 : laneCC);
    return (float) (gridBottom + velH - 3) - (float) (velH - 6) * (float) value / span;
}

int RollModel::noteAt(const Geometry& g, int tick, int pitch, noteedit::Grab& grab, int x) const {
    const auto n = notes();
    grab = noteedit::Grab::Miss;
    for (int i = (int) n.size() - 1; i >= 0; --i) {
        const auto& e = n[(size_t) i];
        if (e.pitch != pitch) continue;
        if (tick < e.tick || tick >= e.tick + e.lengthTicks) continue;
        const auto b = g.noteBox(e);
        grab = noteedit::grabAt(b, (float) x, b.y + b.h * 0.5f);
        return i;
    }
    return -1;
}

int RollModel::ccPointAt(const Geometry& g, int x, int y) const {
    constexpr float kPointGrab = 5.0f;
    if (laneCC < 0) return -1;
    const auto ccs = clip_.ccs();
    int best = -1;
    float bestDistance = kPointGrab;
    for (int i = 0; i < (int) ccs.size(); ++i) {
        const auto& c = ccs[(size_t) i];
        if (c.controller != laneCC) continue;
        const float dx = g.tickToX(c.tick) - (float) x, dy = g.ccY(c.value, laneCC) - (float) y;
        const float d = std::sqrt(dx * dx + dy * dy);
        if (d <= bestDistance) {
            bestDistance = d;
            best = i;
        }
    }
    return best;
}

void RollModel::selectAll() {
    selection.clear();
    const int n = (int) notes().size();
    for (int i = 0; i < n; ++i) selection.insert(i);
}

void RollModel::marqueeSelect(const Geometry& g, const Rect& area, bool additive) {
    if (!additive) selection.clear();
    const auto n = notes();
    for (int i = 0; i < (int) n.size(); ++i) {
        const auto& e = n[(size_t) i];
        const Rect r{(int) g.tickToX(e.tick), (int) g.pitchToY(e.pitch),
                     std::max(3, (int) (g.tickToX(e.tick + e.lengthTicks) - g.tickToX(e.tick))), g.rowH - 1};
        if (area.intersects(r)) selection.insert(i);
    }
}

void RollModel::ccMarqueeSelect(const Geometry& g, const Rect& area, bool additive) {
    if (!additive) ccSelection.clear();
    const auto ccs = clip_.ccs();
    const Rect grown = area.expanded(2);
    for (int i = 0; i < (int) ccs.size(); ++i) {
        const auto& c = ccs[(size_t) i];
        if (c.controller != laneCC) continue;
        if (grown.contains((int) g.tickToX(c.tick), (int) g.ccY(c.value, laneCC))) ccSelection.insert(i);
    }
}

void RollModel::nudge(const Geometry& g, int dTicks, int dSemis, int dVel) {
    if (selection.empty() || (dTicks == 0 && dSemis == 0 && dVel == 0)) return;
    if (!nudgeOpen_) {
        clip_.pushUndo();
        nudgeOpen_ = true;
    }
    auto n = notes();
    const int span = g.duration;
    for (int i : selection) {
        if (i < 0 || i >= (int) n.size()) continue;
        auto& e = n[(size_t) i];
        e.tick = noteedit::clampTo(e.tick + dTicks, 0, std::max(0, span - 1));
        e.pitch = noteedit::clampTo(e.pitch + dSemis, 0, kMidiMax);
        e.velocity = noteedit::clampTo(e.velocity + dVel, 1, kMidiMax);
    }
    commit(g, n);
}

void RollModel::printGroove(const Geometry& g, const swing::Groove& groove) {
    if (selection.empty() || groove.amount <= 0.0) return;
    clip_.pushUndo();
    auto n = notes();
    const int span = g.duration;
    for (int i : selection) {
        if (i < 0 || i >= (int) n.size()) continue;
        auto& e = n[(size_t) i];
        e.tick = noteedit::clampTo(e.tick + (int) std::lround(swing::delayTicks(e.tick, groove)), 0, std::max(0, span - 1));
    }
    commit(g, n);
}

void RollModel::quantise(const Geometry& g, int gridTicks) {
    if (selection.empty() || gridTicks <= 0) return;
    auto n = notes();
    const int span = g.duration;
    bool moved = false;
    for (int i : selection) {
        if (i < 0 || i >= (int) n.size()) continue;
        auto& e = n[(size_t) i];
        const int now = (int) std::lround(e.tick / (double) gridTicks) * gridTicks;
        const int landed = noteedit::clampTo(now, 0, std::max(0, span - 1));
        moved = moved || landed != e.tick;
        e.tick = landed;
    }
    if (!moved) return;
    clip_.pushUndo();
    commit(g, n);
}

void RollModel::deleteSelection(const Geometry& g) {
    if (selection.empty()) return;
    clip_.pushUndo();
    auto n = notes();
    std::vector<NoteEvent> kept;
    for (int i = 0; i < (int) n.size(); ++i)
        if (!selection.count(i)) kept.push_back(n[(size_t) i]);
    selection.clear();
    commit(g, kept);
}

void RollModel::copySelection(const Geometry& g, bool cut) {
    const auto n = notes();
    if (selection.empty()) return;
    int minTick = INT_MAX;
    for (int i : selection)
        if (i < (int) n.size()) minTick = std::min(minTick, n[(size_t) i].tick);
    clipboard_.clear();
    for (int i : selection)
        if (i < (int) n.size()) {
            auto e = n[(size_t) i];
            e.tick -= minTick;
            clipboard_.push_back(e);
        }
    if (cut) deleteSelection(g);
}

void RollModel::paste(const Geometry& g, double playheadTick) {
    if (clipboard_.empty()) return;
    clip_.pushUndo();
    const int at = g.snapTick(playheadTick >= 0.0 ? (int) std::lround(playheadTick) : 0);
    auto n = notes();
    selection.clear();
    for (auto e : clipboard_) {
        e.tick += at;
        if (e.tick >= g.duration) continue;
        selection.insert((int) n.size());
        n.push_back(e);
    }
    commit(g, n);
}

void RollModel::duplicate(const Geometry& g) {
    const auto base = notes();
    if (selection.empty()) return;
    clip_.pushUndo();
    int minTick = INT_MAX, maxEnd = 0;
    for (int i : selection)
        if (i < (int) base.size()) {
            minTick = std::min(minTick, base[(size_t) i].tick);
            maxEnd = std::max(maxEnd, base[(size_t) i].tick + base[(size_t) i].lengthTicks);
        }
    const int s = g.snap;
    const int span = ((maxEnd - minTick + s - 1) / s) * s;
    auto n = base;
    std::set<int> fresh;
    for (int i : selection)
        if (i < (int) base.size()) {
            auto e = base[(size_t) i];
            e.tick += span;
            if (e.tick >= g.duration) continue;
            fresh.insert((int) n.size());
            n.push_back(e);
        }
    selection = std::move(fresh);
    commit(g, n);
}

void RollModel::split(const Geometry& g, int noteIndex, int atTick) {
    auto n = notes();
    if (noteIndex < 0 || noteIndex >= (int) n.size()) return;
    auto& e = n[(size_t) noteIndex];
    const int cut = g.snapTick(atTick);
    if (cut <= e.tick || cut >= e.tick + e.lengthTicks) return;
    clip_.pushUndo();
    NoteEvent right = e;
    right.tick = cut;
    right.lengthTicks = e.tick + e.lengthTicks - cut;
    e.lengthTicks = cut - e.tick;
    n.push_back(right);
    selection.clear();
    commit(g, n);
}

int RollModel::selectedNotesColour() const {
    const auto n = notes();
    std::vector<int> colours;
    for (int i : selection)
        if (i >= 0 && i < (int) n.size()) colours.push_back(n[(size_t) i].colour);
    return notecolour::shared(colours);
}

void RollModel::colourSelectedNotes(const Geometry& g, int colour, bool asUndoStep) {
    if (selection.empty()) return;
    if (asUndoStep) clip_.pushUndo();
    auto n = notes();
    for (int i : selection)
        if (i >= 0 && i < (int) n.size()) n[(size_t) i].colour = colour;
    commit(g, n);
}

int RollModel::selectedCCsColour() const {
    const auto ccs = clip_.ccs();
    std::vector<int> colours;
    for (int i : ccSelection)
        if (i >= 0 && i < (int) ccs.size()) colours.push_back(ccs[(size_t) i].colour);
    return notecolour::shared(colours);
}

void RollModel::colourSelectedCCs(int colour, bool asUndoStep) {
    if (ccSelection.empty()) return;
    if (asUndoStep) clip_.pushUndo();
    auto ccs = clip_.ccs();
    for (int i : ccSelection)
        if (i >= 0 && i < (int) ccs.size()) ccs[(size_t) i].colour = colour;
    clip_.setCCs(ccs);
}

void RollModel::deleteSelectedCCs() {
    if (ccSelection.empty()) return;
    clip_.pushUndo();
    const auto ccs = clip_.ccs();
    std::vector<CCEvent> kept;
    for (int i = 0; i < (int) ccs.size(); ++i)
        if (ccSelection.count(i) == 0) kept.push_back(ccs[(size_t) i]);
    ccSelection.clear();
    clip_.setCCs(kept);
}

void RollModel::clearLane() {
    if (laneCC < 0) return;
    clip_.pushUndo();
    auto ccs = clip_.ccs();
    ccs.erase(std::remove_if(ccs.begin(), ccs.end(), [this](const CCEvent& c) { return c.controller == laneCC; }),
              ccs.end());
    clip_.setCCs(ccs);
}

void RollModel::scrollPitch(int step, int rows) {
    topPitch = noteedit::clampTo(topPitch + step, std::max(0, rows - 1), kMidiMax);
    pitchScrolled = true;
}

void RollModel::fitPitch(int rows) {
    if (pitchScrolled) return;
    const auto n = notes();
    if (n.empty()) return;
    int lo = kMidiMax, hi = 0;
    for (const auto& e : n) {
        lo = std::min(lo, e.pitch);
        hi = std::max(hi, e.pitch);
    }
    const int want = hi + (rows - (hi - lo + 1)) / 2;
    topPitch = noteedit::clampTo(hi - lo + 1 <= rows ? want : hi + 1, std::max(0, rows - 1), kMidiMax);
}

void RollModel::followRecording(int rows) {
    const auto n = notes();
    if (n.empty()) return;
    const int p = n.back().pitch;
    if (p > topPitch) topPitch = noteedit::clampTo(p + 4, rows - 1, kMidiMax);
    else if (p < topPitch - rows + 1) topPitch = noteedit::clampTo(p + rows / 2, rows - 1, kMidiMax);
}

}
