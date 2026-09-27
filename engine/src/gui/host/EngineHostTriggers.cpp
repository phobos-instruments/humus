// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"
#include <limits>
#include "gui/host/PresetRecall.h"
#include <algorithm>
#include <string>
#include <vector>
#include "core/library/BankLibrary.h"
#include "core/packs/ClassString.h"
#include "core/params/ParamSchema.h"
#include "core/graph/RtWord.h"
#include "core/packs/Roles.h"
#include "gui/host/NodeRoll.h"

namespace hum {

void EngineHost::firePresetStep(const std::string& organism, int dir, bool high) {
    bool& was = presetStepHigh_[organism + (dir > 0 ? "+" : "-")];
    const bool edge = high && !was;
    was = high;
    if (!edge) return;
    presets::recallBracketed(*this, organism,
                             [this, organism, dir] { presets_.recallAdjacent(organism, dir); });
    ++liveControlGen_;
    if (onNodeRolled) onNodeRolled(organism);
}

double EngineHost::markerBeat(const std::string& organism, int slot) const {
    (void) organism;
    return markerBeatIn(model_, slot);
}

void EngineHost::setMarkerBeat(const std::string& organism, int slot, double beat) {
    auto* cm = mutableByName(organism);
    if (cm == nullptr) return;
    const auto name = markerBeatParam(slot);
    for (auto& p : cm->properties)
        if (p.name == name) {
            p.value = p.rangeMin = p.rangeMax = beat;
            dirty_ = true;
            ++changeStamp_;
            return;
        }
    Parameter p;
    p.index = (int) cm->properties.size();
    p.name = name;
    p.type = "double";
    p.value = p.rangeMin = p.rangeMax = beat;
    p.userEdited = true;
    cm->properties.push_back(std::move(p));
    dirty_ = true;
    ++changeStamp_;
}

void EngineHost::fireMarker(const std::string& organism, int slot, bool high) {
    bool& was = markerHigh_[markerAction(slot)];
    const bool edge = high && !was;
    was = high;
    if (!edge) return;
    noteFired(organism, markerAction(slot));
    const double held = markerBeat(organism, slot);
    if (held < 0.0) setMarkerBeat(organism, slot, positionBeats());
    else setPositionBeats(held);
}

void EngineHost::fireMarkerStep(const std::string& organism, int dir, bool high) {
    bool& was = markerHigh_[dir > 0 ? kMarkerNextAction : kMarkerPrevAction];
    const bool edge = high && !was;
    was = high;
    if (!edge) return;
    noteFired(organism, dir > 0 ? kMarkerNextAction : kMarkerPrevAction);
    const double now = positionBeats();
    double best = -1.0;
    for (int slot = 1; slot <= kMarkers; ++slot) {
        const double at = markerBeat(organism, slot);
        if (at < 0.0) continue;
        if (dir > 0 ? (at > now + 1.0e-6 && (best < 0.0 || at < best))
                    : (at < now - 1.0e-6 && at > best))
            best = at;
    }
    if (best >= 0.0) setPositionBeats(best);
    else if (dir < 0) setPositionBeats(0.0);
}

void EngineHost::fireTransport(const std::string& action, bool high) {
    bool& was = transportHigh_[action];
    const bool edge = high && !was;
    was = high;
    if (!edge) return;
    noteFired(clockNodeName(), action);
    if (action == kPlayAction) play();
    else if (action == kStopAction) stop();
    else if (action == kPlayFromStartAction) playFromStart();
    else if (action == kGoToStartAction) goToStart();
    else if (action == kGoToEndAction) goToEnd();
    else if (action == kCaptureAction) record().captureToggle();
    else if (action == kPanicAction) panic();
    else if (action == kLoopToggleAction)
        automation().setLoop(automation().loopStartBeat(), automation().loopEndBeat(),
                             !automation().loopEnabled());
}

void EngineHost::fireRandom(const std::string& organism, bool high) {
    bool& was = randomHigh_[organism];
    const bool edge = high && !was;
    was = high;
    if (!edge) return;
    auto rollLive = [this](const std::string& name) {
        paramHistory_.commit(name, captureNodeState(name));
        randomizeNode(*this, name,false);
        paramHistory_.commit(name, captureNodeState(name));
        ++liveControlGen_;
        if (onNodeRolled) onNodeRolled(name);
    };
    const auto* cm = model_.byName(organism);
    if (cm == nullptr) return;
    LiveControlScope live(*this);
    if (isClockPseudo(cm->displayClass)) {
        for (const auto& n : rollscope::targets(rollCandidates(*this))) rollLive(n);
        return;
    }
    rollLive(organism);
}

void EngineHost::firePodRandom(const std::string& pod, bool high) {
    bool& was = randomHigh_[rollscope::podAction(pod)];
    const bool edge = high && !was;
    was = high;
    if (!edge) return;
    LiveControlScope live(*this);
    for (const auto& n : rollscope::targets(rollCandidates(*this), pod)) {
        paramHistory_.commit(n, captureNodeState(n));
        randomizeNode(*this, n, false);
        paramHistory_.commit(n, captureNodeState(n));
        ++liveControlGen_;
        if (onNodeRolled) onNodeRolled(n);
    }
}

void EngineHost::rollNode(const std::string& name) { randomizeNode(*this, name); }

}
