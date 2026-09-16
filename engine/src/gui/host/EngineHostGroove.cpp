// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include <cmath>
#include <string>

#include "gui/host/EngineHost.h"
#include "hum/Swing.h"

namespace hum {

void EngineHost::applyGroove() {
    if (graph_ == nullptr) return;
    const juce::ScopedLock sl(lock_);
    graph_->transport().setGroove(swing::grooveFor(model_.groove, model_.grooveUnit));
}

void EngineHost::restoreUnautomatedGroove(bool amountLane, bool gridLane) {
    if (graph_ == nullptr) return;
    const auto base = swing::grooveFor(model_.groove, model_.grooveUnit);
    auto live = graph_->transport().groove();
    if (!amountLane) live.amount = base.amount;
    if (!gridLane) live.gridTicks = base.gridTicks;
    graph_->transport().setGroove(live);
}

int EngineHost::liveGrooveGrid() const {
    return swing::gridIndexOf(graph_ ? graph_->transport().groove().gridTicks
                                     : stepTicksFor(model_.grooveUnit));
}

void EngineHost::performGroove(double amount) {
    if (capturing_ || playing_) {
        setParam(clockNodeName(), kGrooveParam, amount);
        return;
    }
    setGroove(amount, model_.grooveUnit);
}

void EngineHost::performGrooveGrid(int index) {
    if (capturing_ || playing_) {
        setParam(clockNodeName(), kGrooveGridParam, (double) index);
        return;
    }
    setGroove(model_.groove, swing::gridUnitAt(index));
}

bool EngineHost::setClockGroove(const std::string& organism, const std::string& param,
                                double value) {
    const auto* cm = model_.byName(organism);
    if (cm == nullptr || !isClockPseudo(cm->displayClass) || !isGrooveParam(param)) return false;
    const double was = prePassValue(organism, param);
    const bool amount = param == kGrooveParam;
    const double v = amount ? juce::jlimit(0.0, 1.0, value)
                            : (double) swing::gridIndexClamped(std::lround(value));
    if (amount) model_.groove = v;
    else model_.grooveUnit = swing::gridUnitAt((long) v);
    applyGroove();
    if (liveControl_) ++liveControlGen_;
    else markDirty();
    noteTouch(organism, param);
    capturePoint(organism, param, v, v, false, was);
    if (playing_ && !capturing_ && !derivedControl_)
        perfRing_.push_back({positionBeats(), organism, param, v, v, false});
    return true;
}

}
