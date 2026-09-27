// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "core/params/ParamSchema.h"
#include "gui/editor/Geometry.h"
#include "gui/editor/LiveControls.h"
#include "gui/host/BrickHost.h"
#include "hum/Chord.h"
#include "hum/caps/Midi.h"

namespace hum::input {

class StepStripModel {
public:
    static constexpr int kDefaultSteps = 16;

    StepStripModel(BrickHost& host, std::string organism, std::string muteParam)
        : host_(host), organism_(std::move(organism)), muteParam_(std::move(muteParam)) {}

    int steps() const;

    int mask() const { return (int) host_.liveParamValue(organism_, muteParam_); }
    bool silenced(int step) const { return ((mask() >> step) & 1) != 0; }

    int cellAt(float x, int width) const;
    void press(int step);
    bool sweep(int step);

    void playEvery() { setMask(0); }
    void invert() { setMask(~mask()); }
    void silenceOffBeats() { setMask(0xAAAA); }

    std::int64_t absoluteStep() const;
    bool rests(std::int64_t absolute) const;
    bool poll();

private:
    StepStrip* source() const { return live::source<StepStrip>(host_, organism_); }
    int allSteps() const { return (1 << steps()) - 1; }
    void setMask(int m) { host_.editParam(organism_, muteParam_, (double) (m & allSteps())); }

    int restsOfLap(std::int64_t absolute) const;

    BrickHost& host_;
    std::string organism_, muteParam_;
    std::int64_t lastStep_ = -2;
    int lastRests_ = -1;
    int lastMute_ = -1;
    bool silencing_ = false;
};

class IntervalRowsModel {
public:
    static constexpr int kKeys = 13;
    static constexpr int kLabelW = 18;

    struct Row {
        std::string param, label;
    };

    IntervalRowsModel(ModelHost& host, std::string organism, const std::string& className, const std::string& rowPrefix, std::string chordParam);

    const std::vector<Row>& rows() const { return rows_; }

    int overrideOf(int row) const;
    int effectiveOf(int row) const;
    std::pair<int, int> cellAt(float x, float y, const Rect& area) const;
    void press(int row, int key);
    bool poll();

private:
    ModelHost& host_;
    std::string organism_, chordParam_;
    std::vector<Row> rows_;
    std::vector<int> lastSig_;
};

}
