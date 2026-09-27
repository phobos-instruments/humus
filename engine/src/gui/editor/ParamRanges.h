// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <utility>
#include <vector>

#include "core/params/ParamSchema.h"
#include "core/params/RangeEnd.h"
#include "gui/host/ModelHost.h"
#include "hum/Swing.h"
#include "io/PatchDocument.h"

namespace hum {

inline std::pair<double, double> paramRange(ModelHost& host, const std::string& organism,
                                            const std::string& param) {
    if (auto* cm = host.model().byName(organism)) {
        if (isClockPseudo(cm->displayClass) && param == kTempoParam)
            return {kTempoMin, kTempoMax};
        if (isClockPseudo(cm->displayClass) && isMeterParam(param))
            return {kMeterLaneMin, kMeterLaneMax};
        if (isClockPseudo(cm->displayClass) && param == kGrooveParam) return {0.0, 1.0};
        if (isClockPseudo(cm->displayClass) && param == kGrooveGridParam)
            return {0.0, (double) (swing::kGridChoices - 1)};
        if (isMetapadPseudo(cm->displayClass) && param == kMetaTemperatureParam)
            return {0.0, kMetaTemperatureMax};
        const auto base = rangeBaseOf(param);
        for (const auto& d : schemaFor(cm->classRaw))
            if (d.name == base) return {d.min, d.max};
    }
    return {0.0, 1.0};
}

inline bool paramIsRange(ModelHost& host, const std::string& organism,
                         const std::string& param) {
    if (auto* cm = host.model().byName(organism))
        for (const auto& d : schemaFor(cm->classRaw))
            if (d.name == param) return d.isRange;
    return false;
}

inline std::pair<double, double> endBounds(ModelHost& host, const std::string& organism,
                                           const std::string& param, RangeEnd end) {
    const auto whole = paramRange(host, organism, param);
    if (end != RangeEnd::Spread) return whole;
    return {0.0, whole.second - whole.first};
}

inline std::vector<std::string> controlNames(ModelHost& host, const std::string& organism,
                                             const std::string& param) {
    const auto base = rangeBaseOf(param);
    if (!paramIsRange(host, organism, base)) return {param};
    return {base, rangeEndParam(base, RangeEnd::Low), rangeEndParam(base, RangeEnd::High),
            rangeEndParam(base, RangeEnd::Spread)};
}

inline bool paramIsControlled(ModelHost& host, const std::string& organism,
                              const std::string& param) {
    if (host.isExternallyControlled(organism, param)) return true;
    if (!paramIsRange(host, organism, rangeBaseOf(param))) return false;
    for (const auto end : {RangeEnd::Low, RangeEnd::High, RangeEnd::Spread})
        if (host.isExternallyControlled(organism, rangeEndParam(rangeBaseOf(param), end)))
            return true;
    return false;
}

inline bool paramDefault(ModelHost& host, const std::string& organism,
                         const std::string& param, double& def, double& defMax) {
    if (auto* cm = host.model().byName(organism))
        for (const auto& d : schemaFor(cm->classRaw))
            if (d.name == param) { def = d.def; defMax = d.isRange ? d.defMax : d.def; return true; }
    return false;
}

}
