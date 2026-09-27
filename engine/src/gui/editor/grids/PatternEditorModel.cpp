// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/grids/PatternEditorModel.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>
#include <utility>

namespace hum::grids {

PatternEditorModel::PatternEditorModel(BrickHost& host, PatternEdits& patterns, std::string organism, int lanes)
    : host_(host), patterns_(patterns), organism_(std::move(organism)), lanes_(lanes) {
    patterns_.ensure(organism_, lanes_);
}

const Pattern* PatternEditorModel::pattern() const {
    if (auto* cm = host_.model().byName(organism_)) return &cm->pattern;
    return nullptr;
}

int PatternEditorModel::ticksOf(const std::string& res) {
    const auto slash = res.find('/');
    const int denom = slash == std::string::npos ? 0 : std::atoi(res.c_str() + slash + 1);
    return denom > 0 ? (4 * Pattern::kTicksPerBeat) / denom : Pattern::kTicksPerBeat / 4;
}

std::string PatternEditorModel::laneSnap(int lane) const {
    const auto* p = pattern();
    if (p) {
        const auto ch = p->triggerChannels();
        if (lane >= 0 && lane < (int) ch.size() && !ch[(size_t) lane]->snap.empty()) return ch[(size_t) lane]->snap;
    }
    return resolution();
}

int PatternEditorModel::snapTick(int lane, int tick) const {
    const int s = std::max(1, ticksOf(laneSnap(lane)));
    return ((tick + s / 2) / s) * s;
}

int PatternEditorModel::nearestTrigger(int lane, int tick, int tolTicks) const {
    const auto* p = pattern();
    if (!p) return -1;
    const auto chans = p->triggerChannels();
    if (lane < 0 || lane >= (int) chans.size()) return -1;
    int best = -1, bd = tolTicks + 1;
    for (int t : chans[(size_t) lane]->triggers) {
        const int d = std::abs(t - tick);
        if (d < bd) {
            bd = d;
            best = t;
        }
    }
    return best;
}

void PatternEditorModel::fitZoom(double available) {
    const auto* p = pattern();
    const double beats = (p && p->duration > 0) ? (double) p->duration / Pattern::kTicksPerBeat : 4.0;
    if (available > 0.0) ppb_ = std::clamp(available / std::max(1.0, beats), 18.0, 160.0);
}

void PatternEditorModel::press(int lane, float x, int gridX, bool free) {
    dragLane_ = -1;
    dragAdded_ = dragMoved_ = deleteOnUp_ = false;
    const int rawTick = std::max(0, xToTick(x, gridX));
    const int tolTicks = std::max(1, (int) std::lround(7.0 / pixelsPerTick()));
    const int hit = nearestTrigger(lane, rawTick, tolTicks);
    host_.pushUndo();
    dragLane_ = lane;
    if (hit >= 0) {
        dragTick_ = hit;
        return;
    }
    const int t = free ? rawTick : snapTick(lane, rawTick);
    patterns_.addTrigger(organism_, lane, t);
    dragTick_ = t;
    dragAdded_ = true;
}

void PatternEditorModel::cancel() {
    dragLane_ = -1;
    dragAdded_ = dragMoved_ = deleteOnUp_ = false;
}

bool PatternEditorModel::drag(float x, int y, int gridX, int laneTop, int laneH, bool free) {
    if (dragLane_ < 0) return false;
    deleteOnUp_ = y < laneTop - 10 || y > laneTop + laneH + 10;
    const int rawTick = std::max(0, xToTick(x, gridX));
    const int newTick = free ? rawTick : snapTick(dragLane_, rawTick);
    if (!deleteOnUp_ && newTick != dragTick_) {
        patterns_.moveTrigger(organism_, dragLane_, dragTick_, newTick);
        dragTick_ = newTick;
        dragMoved_ = true;
    }
    return true;
}

void PatternEditorModel::release() {
    if (dragLane_ >= 0 && (deleteOnUp_ || (!dragAdded_ && !dragMoved_)))
        patterns_.removeTrigger(organism_, dragLane_, dragTick_);
    cancel();
}

PatternEditorModel::Playhead PatternEditorModel::followPlayhead() {
    if (!host_.isPlaying()) {
        if (!showPlayhead_) return Playhead::Unchanged;
        showPlayhead_ = false;
        return Playhead::Hidden;
    }
    double tick = host_.positionBeats() * Pattern::kTicksPerBeat;
    if (const auto* p = pattern(); p && p->duration > 0) tick = std::fmod(tick, (double) p->duration);
    showPlayhead_ = true;
    if (std::abs(tick - playTick_) <= 0.5) return Playhead::Unchanged;
    playTick_ = tick;
    return Playhead::Moved;
}

}
