#pragma once
#include <string>

#include "core/ControlShape.h"
#include "core/ParamSchema.h"
#include "gui/EngineHost.h"
#include "io/PatchDocument.h"

namespace hum {

inline bool isActionTarget(EngineHost& host, const std::string& organism,
                           const std::string& param) {
    if (isHostSwitchTarget(param)) return true;
    if (const auto* cm = host.model().byName(organism))
        if (isMetapadPseudo(cm->displayClass)) return isMetapadAction(param);
    return false;
}

inline bool paramIsSwitch(EngineHost& host, const std::string& organism,
                          const std::string& param) {
    if (isActionTarget(host, organism, param)) return true;
    if (const auto* cm = host.model().byName(organism))
        for (const auto& d : schemaFor(cm->classRaw))
            if (d.name == param) return d.isBool;
    return false;
}

inline bool paramIsLog(EngineHost& host, const std::string& organism,
                       const std::string& param) {
    if (const auto* cm = host.model().byName(organism))
        for (const auto& d : schemaFor(cm->classRaw))
            if (d.name == param) return !d.isBool && !d.isEnum && rangeIsLogarithmic(d.min, d.max);
    return false;
}

}
