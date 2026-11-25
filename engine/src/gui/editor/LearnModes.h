// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "gui/editor/AutomateMenu.h"
#include "gui/editor/ParamLearners.h"
#include "gui/host/BrickHost.h"

namespace hum {

inline void cancelLearningOn(const BrickHost& host) {
    if (MidiLearner::instance().armedOn(host)) MidiLearner::instance().cancel();
    if (OscLearner::instance().armedOn(host)) OscLearner::instance().cancel();
    if (FollowPicker::instance().armedOn(host)) FollowPicker::instance().cancel();
}

}
