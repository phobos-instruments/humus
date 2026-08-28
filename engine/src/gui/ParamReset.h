#pragma once
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/ParamSchema.h"
#include "gui/EngineHost.h"

namespace hum {

inline bool paramDefault(EngineHost& host, const std::string& organism,
                         const std::string& param, double& def, double& defMax) {
    if (auto* cm = host.model().byName(organism))
        for (const auto& d : schemaFor(cm->classRaw))
            if (d.name == param) { def = d.def; defMax = d.isRange ? d.defMax : d.def; return true; }
    return false;
}

inline void enableDoubleClickReset(juce::Slider& s, EngineHost& host,
                                   const std::string& organism, const std::string& param) {
    double def = 0.0, defMax = 0.0;
    if (paramDefault(host, organism, param, def, defMax))
        s.setDoubleClickReturnValue(true, def);
}

}
