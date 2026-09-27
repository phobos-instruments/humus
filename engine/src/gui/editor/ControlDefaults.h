// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include "core/midi/ControlGuess.h"
#include "core/net/ControlShape.h"
#include "core/params/ParamSchema.h"
#include "gui/host/BrickHost.h"
#include "io/PatchDocument.h"
#include "io/PatchDocument.h"

namespace hum {

inline bool isActionTarget(BrickHost& host, const std::string& organism,
                           const std::string& param) {
    if (isHostSwitchTarget(param)) return true;
    if (const auto* cm = host.model().byName(organism))
        if (isMetapadPseudo(cm->displayClass)) return isMetapadAction(param);
    return false;
}

inline bool paramIsSwitch(BrickHost& host, const std::string& organism,
                          const std::string& param) {
    if (isActionTarget(host, organism, param)) return true;
    if (const auto* cm = host.model().byName(organism))
        for (const auto& d : schemaFor(cm->classRaw))
            if (d.name == param) return d.isBool;
    return false;
}

inline bool paramIsLog(BrickHost& host, const std::string& organism,
                       const std::string& param) {
    if (const auto* cm = host.model().byName(organism))
        for (const auto& d : schemaFor(cm->classRaw))
            if (d.name == param) return !d.isBool && !d.isEnum && rangeIsLogarithmic(d.min, d.max);
    return false;
}

inline bool paramIsMomentary(BrickHost& host, const std::string& organism,
                             const std::string& param) {
    if (isActionTarget(host, organism, param)) return true;
    if (const auto* cm = host.model().byName(organism))
        for (const auto& d : schemaFor(cm->classRaw))
            if (d.name == param) return d.isTrigger;
    return false;
}

inline ControlShape learnedShape(BrickHost& host, const std::string& organism,
                                 const std::string& param, const ControlGuess& g) {
    ControlShape d;
    d.type = g.type;
    d.encoder = g.encoder;
    const bool switchParam = paramIsSwitch(host, organism, param);
    if (switchParam && !d.isButton()) {
        d.type = ControlType::Button;
        d.button = ButtonMode::Hold;
    } else if (d.isButton()) {
        d.button = paramIsMomentary(host, organism, param) ? ButtonMode::Hold : ButtonMode::Toggle;
    }
    if (d.isFader()) d.logScale = paramIsLog(host, organism, param);
    return d;
}

}
