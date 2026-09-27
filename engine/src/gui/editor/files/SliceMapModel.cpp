// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/files/SliceMapModel.h"

#include <algorithm>
#include <cstdlib>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace hum::files {

bool SliceMapModel::hasAudio() const {
    auto* src = source();
    return src != nullptr && !src->slicePeaks().empty()
           && *std::max_element(src->slicePeaks().begin(), src->slicePeaks().end()) > 0.0f;
}

int SliceMapModel::playing() const {
    auto* src = source();
    return src != nullptr ? src->playingSlice() : -1;
}

int SliceMapModel::origin(int slice) const {
    auto* w = bench();
    return w != nullptr ? w->sliceOrigin(slice) : -1;
}

bool SliceMapModel::originShown(int slice) const {
    auto* w = bench();
    const int o = origin(slice);
    return w != nullptr && o >= 0 && o < w->originCount();
}

std::string SliceMapModel::originName(int originIndex) const {
    auto* w = bench();
    return w != nullptr ? w->originName(originIndex) : std::string();
}

int SliceMapModel::sliceAt(float fraction) const {
    int s = -1;
    for (int i = 0; i < (int) starts_.size(); ++i)
        if (starts_[(size_t) i] <= fraction) s = i;
    return s;
}

bool SliceMapModel::showsRuler() const {
    auto* w = bench();
    return w != nullptr && !w->startParam().empty();
}

float SliceMapModel::start() const {
    auto* w = bench();
    return w != nullptr ? (float) host_.liveParamValue(organism_, w->startParam()) : 0.0f;
}

bool SliceMapModel::setStart(double fraction) {
    auto* w = bench();
    if (w == nullptr) return false;
    host_.setParam(organism_, w->startParam(), std::clamp(fraction, 0.0, 0.999));
    return true;
}

void SliceMapModel::press(int slice, int x, int y) {
    sel_ = slice;
    dragPitch0_ = edits_[slice].pitch;
    dragY0_ = y;
    dragX0_ = x;
    axis_ = Axis::Undecided;
    nudged_ = 0;
    auto* w = bench();
    nudgeFrom_ = w != nullptr ? w->pinValue(slice) : std::string();
}

bool SliceMapModel::drag(int x, int y, bool bySource) {
    if (sel_ < 0) return false;
    if (axis_ == Axis::Undecided) {
        const int dx = std::abs(x - dragX0_), dy = std::abs(y - dragY0_);
        if (std::max(dx, dy) < kAxisPx) return false;
        axis_ = dx > dy && !nudgeFrom_.empty() ? Axis::Across : Axis::Up;
    }
    if (axis_ == Axis::Across) return nudgeTo((x - dragX0_) / kNudgePx, bySource);
    const int p = std::clamp(dragPitch0_ + (dragY0_ - y) / 6, -24, 24);
    auto& ed = edits_[sel_];
    if (ed.pitch == p) return false;
    ed.pitch = p;
    pushEdits();
    return true;
}

bool SliceMapModel::togglePin(int s, bool one) {
    auto* w = bench();
    if (w == nullptr) return false;
    if (!one) pins_.clear();
    else if (pins_.erase(s) == 0) {
        const std::string value = w->pinValue(s);
        if (value.empty()) return false;
        pins_[s] = value;
    }
    host_.setParamText(organism_, w->pinParam(), encodeSlicePins(pins_));
    return true;
}

bool SliceMapModel::rollOne(int s) {
    auto* w = bench();
    if (w == nullptr) return false;
    const std::string value = w->pinAlternative(s);
    if (value.empty()) return false;
    pins_[s] = value;
    host_.setParamText(organism_, w->pinParam(), encodeSlicePins(pins_));
    return true;
}

void SliceMapModel::pull() {
    edits_ = parseSliceEdits(host_.liveParamText(organism_, editsParam_));
    if (auto* w = bench()) pins_ = parseSlicePins(host_.liveParamText(organism_, w->pinParam()));
}

bool SliceMapModel::poll() {
    auto* src = source();
    if (src == nullptr) return false;
    const unsigned gen = src->sliceGeneration();
    const int nowPlaying = src->playingSlice();
    auto fresh = src->sliceStarts();
    if (gen == lastGen_ && nowPlaying == lastPlaying_ && fresh == starts_) return false;
    lastGen_ = gen;
    lastPlaying_ = nowPlaying;
    starts_ = std::move(fresh);
    return true;
}

bool SliceMapModel::nudgeTo(int steps, bool bySource) {
    auto* w = bench();
    if (w == nullptr || steps == nudged_) return false;
    nudged_ = steps;
    const std::string value = w->pinNudged(nudgeFrom_, bySource ? steps : 0, bySource ? 0 : steps);
    if (value.empty()) return false;
    pins_[sel_] = value;
    host_.setParamText(organism_, w->pinParam(), encodeSlicePins(pins_));
    w->auditionSlice(sel_);
    return true;
}

void SliceMapModel::pushEdits() {
    for (auto it = edits_.begin(); it != edits_.end();)
        it = it->second.isDefault() ? edits_.erase(it) : std::next(it);
    host_.setParamText(organism_, editsParam_, encodeSliceEdits(edits_));
}

}
