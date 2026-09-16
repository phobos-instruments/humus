// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"
#include <limits>
#include "gui/properties/PresetActions.h"
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

void EngineHost::fireTransport(const std::string& action, bool high) {
    bool& was = transportHigh_[action];
    const bool edge = high && !was;
    was = high;
    if (!edge) return;
    if (action == kPlayAction) play();
    else if (action == kStopAction) stop();
    else if (action == kPlayFromStartAction) playFromStart();
    else if (action == kGoToStartAction) goToStart();
    else if (action == kGoToEndAction) goToEnd();
    else if (action == kCaptureAction) record().captureToggle();
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
        std::vector<std::string> targets;
        for (const auto& c : model_.organisms)
            if (nodeSupportsRandom(*this, c.name)) targets.push_back(c.name);
        for (const auto& n : targets) rollLive(n);
        return;
    }
    rollLive(organism);
}

void EngineHost::rollNode(const std::string& name) { randomizeNode(*this, name); }

}
