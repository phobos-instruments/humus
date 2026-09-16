// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/params/ParamSchema.h"
#include "gui/host/BrickHost.h"
#include "io/PatchDocument.h"

namespace hum {

inline bool paramDefault(BrickHost& host, const std::string& organism,
                         const std::string& param, double& def, double& defMax) {
    if (auto* cm = host.model().byName(organism))
        for (const auto& d : schemaFor(cm->classRaw))
            if (d.name == param) { def = d.def; defMax = d.isRange ? d.defMax : d.def; return true; }
    return false;
}

inline void enableDoubleClickReset(juce::Slider& s, BrickHost& host,
                                   const std::string& organism, const std::string& param) {
    double def = 0.0, defMax = 0.0;
    if (paramDefault(host, organism, param, def, defMax))
        s.setDoubleClickReturnValue(true, def);
}

}
