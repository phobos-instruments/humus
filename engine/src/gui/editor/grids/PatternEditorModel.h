// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>
#include <utility>

#include "gui/host/BrickHost.h"
#include "gui/host/PatternEdits.h"
#include "hum/Pattern.h"
#include "io/PatchDocument.h"

namespace hum::grids {

class PatternEditorModel {
public:
    static constexpr const char* kResolutions[] = {"1/4", "1/8", "1/16", "1/32"};

    PatternEditorModel(BrickHost& host, PatternEdits& patterns, std::string organism, int lanes);

    int lanes() const { return lanes_; }

    const Pattern* pattern() const;

    std::string resolution() const { return pattern() ? pattern()->matrixResolution : std::string("1/16"); }

    static int ticksOf(const std::string& res);

    int snapTicks() const { return ticksOf(resolution()); }

    std::string laneSnap(int lane) const;
    int snapTick(int lane, int tick) const;
    int nearestTrigger(int lane, int tick, int tolTicks) const;

    void setLaneSnap(int lane, const std::string& res) { patterns_.setChannelSnap(organism_, lane, res); }
    void setResolution(const std::string& res) { patterns_.setResolution(organism_, res); }
    void nudgeLane(int lane, int ticks) { patterns_.nudgeChannel(organism_, lane, ticks); }
    void reframe(int ticks) { patterns_.reframe(organism_, ticks); }

    double pixelsPerTick() const { return ppb_ / Pattern::kTicksPerBeat; }
    float tickToX(int tick, int gridX) const { return (float) (gridX + (tick - scrollTicks_) * pixelsPerTick()); }
    int xToTick(float x, int gridX) const { return (int) std::lround(scrollTicks_ + (x - gridX) / pixelsPerTick()); }

    void fitZoom(double available);

    int dragLane() const { return dragLane_; }
    int dragTick() const { return dragTick_; }
    bool deleting() const { return deleteOnUp_; }

    void press(int lane, float x, int gridX, bool free);
    void cancel();
    bool drag(float x, int y, int gridX, int laneTop, int laneH, bool free);
    void release();

    enum class Playhead { Unchanged, Moved, Hidden };

    Playhead followPlayhead();

    bool playheadShown() const { return showPlayhead_; }
    double playTick() const { return playTick_; }

private:
    BrickHost& host_;
    PatternEdits& patterns_;
    std::string organism_;
    int lanes_;
    double ppb_ = 96.0;
    double scrollTicks_ = 0.0;
    int dragLane_ = -1, dragTick_ = -1;
    bool dragAdded_ = false, dragMoved_ = false, deleteOnUp_ = false;
    double playTick_ = 0.0;
    bool showPlayhead_ = false;
};

}
