// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/inputs/StripInputs.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace hum::input {

int StepStripModel::steps() const {
    auto* strip = source();
    const int n = strip != nullptr ? strip->stripSteps() : kDefaultSteps;
    return n < 1 ? 1 : (30 < n ? 30 : n);
}

int StepStripModel::cellAt(float x, int width) const {
    const int n = steps();
    const int k = (int) (x * n / std::max(1, width));
    return k < 0 ? 0 : (n - 1 < k ? n - 1 : k);
}

void StepStripModel::press(int step) {
    silencing_ = !silenced(step);
    setMask(silencing_ ? (mask() | (1 << step)) : (mask() & ~(1 << step)));
}

bool StepStripModel::sweep(int step) {
    const int want = silencing_ ? (mask() | (1 << step)) : (mask() & ~(1 << step));
    if (want == mask()) return false;
    setMask(want);
    return true;
}

std::int64_t StepStripModel::absoluteStep() const {
    auto* strip = source();
    if (!host_.isPlaying() || strip == nullptr) return -1;
    return strip->stripStepAt(host_.positionBeats());
}

bool StepStripModel::rests(std::int64_t absolute) const {
    auto* strip = source();
    return absolute >= 0 && strip != nullptr && strip->stripStepRests(absolute);
}

bool StepStripModel::poll() {
    const auto step = absoluteStep();
    const int rested = restsOfLap(step);
    const int mute = mask();
    if (step == lastStep_ && rested == lastRests_ && mute == lastMute_) return false;
    lastStep_ = step;
    lastRests_ = rested;
    lastMute_ = mute;
    return true;
}

int StepStripModel::restsOfLap(std::int64_t absolute) const {
    auto* strip = source();
    if (absolute < 0 || strip == nullptr) return 0;
    const int n = steps();
    int out = 0;
    for (int k = 0; k < n; ++k)
        if (strip->stripStepRests(absolute - absolute % n + k)) out |= 1 << k;
    return out;
}

IntervalRowsModel::IntervalRowsModel(ModelHost& host, std::string organism, const std::string& className, const std::string& rowPrefix, std::string chordParam)
    : host_(host), organism_(std::move(organism)), chordParam_(std::move(chordParam)) {
    for (const auto& d : schemaFor(className))
        if (!rowPrefix.empty() && d.name.rfind(rowPrefix, 0) == 0)
            rows_.push_back({d.name, d.name.substr(rowPrefix.size())});
    lastSig_.assign(rows_.size() + 1, 0);
}

int IntervalRowsModel::overrideOf(int row) const {
    const int v = (int) host_.liveParamValue(organism_, rows_[(size_t) row].param);
    return v >= 0 && v < kKeys ? v : -1;
}

int IntervalRowsModel::effectiveOf(int row) const {
    const int ov = overrideOf(row);
    if (ov >= 0) return ov;
    const Chord chord = Chord::byId((int) host_.liveParamValue(organism_, chordParam_));
    return (int) std::lround(chord.cents(row) / 100.0);
}

std::pair<int, int> IntervalRowsModel::cellAt(float x, float y, const Rect& area) const {
    const float rowH = (float) area.h / (float) std::max<size_t>(1, rows_.size());
    const float keyW = (float) (area.w - kLabelW) / kKeys;
    const int row = (int) ((y - area.y) / rowH);
    const int key = (int) ((x - area.x - kLabelW) / keyW);
    if (row < 0 || row >= (int) rows_.size() || key < 0 || key >= kKeys || x < area.x + kLabelW) return {-1, -1};
    return {row, key};
}

void IntervalRowsModel::press(int row, int key) {
    const bool release = overrideOf(row) == key;
    host_.editParam(organism_, rows_[(size_t) row].param, release ? -1.0 : (double) key);
}

bool IntervalRowsModel::poll() {
    std::vector<int> sig(rows_.size() + 1, 0);
    for (size_t i = 0; i < rows_.size(); ++i) sig[i] = overrideOf((int) i);
    sig.back() = (int) host_.liveParamValue(organism_, chordParam_);
    if (sig == lastSig_) return false;
    lastSig_ = sig;
    return true;
}

}
