// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cstdlib>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "gui/editor/LiveControls.h"
#include "gui/host/ModelHost.h"
#include "hum/SliceEdits.h"
#include "hum/caps/Samples.h"

namespace hum::files {

class SliceMapModel {
public:
    static constexpr int kNudgePx = 14;
    static constexpr int kAxisPx = 4;

    SliceMapModel(ModelHost& host, std::string organism, std::string editsParam)
        : host_(host), organism_(std::move(organism)), editsParam_(std::move(editsParam)) {}

    SliceSource* source() const { return live::source<SliceSource>(host_, organism_); }
    SliceWorkbench* bench() const { return live::source<SliceWorkbench>(host_, organism_); }

    bool hasAudio() const;

    const std::vector<float>& starts() const { return starts_; }
    const std::map<int, SliceEdit>& edits() const { return edits_; }
    const std::map<int, std::string>& pins() const { return pins_; }
    int selected() const { return sel_; }
    int playing() const;
    int origin(int slice) const;
    bool originShown(int slice) const;
    std::string originName(int originIndex) const;
    int sliceAt(float fraction) const;
    bool showsRuler() const;
    float start() const;
    bool setStart(double fraction);
    void press(int slice, int x, int y);

    void select(int slice) { sel_ = slice; }

    bool drag(int x, int y, bool bySource);

    void audition(int slice) {
        if (auto* w = bench(); w != nullptr && slice >= 0) w->auditionSlice(slice);
    }

    void toggleReverse(int s) { edits_[s].reverse = !edits_[s].reverse; pushEdits(); }
    void toggleMute(int s) { edits_[s].gainPct = edits_[s].gainPct == 0 ? 100 : 0; pushEdits(); }
    void reset(int s) { edits_.erase(s); pushEdits(); }
    void clearAll() { edits_.clear(); pushEdits(); }
    bool reversed(int s) { return edits_[s].reverse; }
    bool muted(int s) { return edits_[s].gainPct == 0; }

    bool togglePin(int s, bool one);
    bool rollOne(int s);
    void pull();
    bool poll();

private:
    enum class Axis { Undecided, Up, Across };

    bool nudgeTo(int steps, bool bySource);
    void pushEdits();

    ModelHost& host_;
    std::string organism_, editsParam_;
    std::vector<float> starts_;
    std::map<int, SliceEdit> edits_;
    std::map<int, std::string> pins_;
    Axis axis_ = Axis::Undecided;
    int dragX0_ = 0, nudged_ = 0;
    std::string nudgeFrom_;
    int sel_ = -1, dragPitch0_ = 0, dragY0_ = 0;
    unsigned lastGen_ = ~0u;
    int lastPlaying_ = -2;
};

}
