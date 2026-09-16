// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <functional>
#include <string>
#include <vector>

#include "core/graph/ModControl.h"
#include "core/graph/ModRoutes.h"
#include "core/params/ParamSchema.h"
#include "io/PatchDocument.h"

namespace hum {

inline const ParamDesc* paramDescOf(const OrganismModel& cm, const std::string& param) {
    for (const auto& d : schemaFor(cm.classRaw))
        if (d.name == param) return &d;
    return nullptr;
}

inline void socketDefaultRange(const ParamDesc& d, double& lo, double& hi) {
    const bool unitFits = d.min <= 0.0 && d.max >= 1.0 && (d.max - d.min) > 1.5;
    lo = unitFits ? 0.0 : d.min;
    hi = unitFits ? 1.0 : d.max;
}

inline bool routeLivesInEngine(const OrganismModel& target, const std::string& param) {
    return !isHostSwitchTarget(param) && !isMetapadPseudo(target.displayClass)
           && !isClockPseudo(target.displayClass);
}

inline bool fillModRoute(ModRoute& r, const OrganismModel& target, const OrganismModel* source,
                         const std::string& sourceValue, double min, double max,
                         const ControlShape& shape,
                         const std::function<int(const std::string&)>& indexOf) {
    if (source == nullptr) return false;
    r.srcNode = indexOf(source->name);
    r.dstNode = indexOf(target.name);
    if (r.srcNode < 0 || r.dstNode < 0) return false;
    r.srcIsParam = isParamSource(sourceValue);
    r.srcValue = r.srcIsParam ? paramSourceName(sourceValue) : sourceValue;
    if (r.srcIsParam) {
        const auto* sd = paramDescOf(*source, r.srcValue);
        if (sd == nullptr || sd->max <= sd->min) return false;
        r.srcMin = sd->min;
        r.srcMax = sd->max;
    }
    r.min = min;
    r.max = max;
    r.shape = shape;
    if (const auto* d = paramDescOf(target, r.dstParam)) {
        const bool rangeUnset = std::abs(r.max - r.min) <= 1.0e-12;
        r.carry = d->carry && rangeUnset;
        r.dstLo = d->min;
        r.dstHi = d->max;
        if (rangeUnset && d->max > d->min) { r.min = d->min; r.max = d->max; }
        if (!r.shape.isSwitch)
            r.shape.logScale = !d->isBool && !d->isEnum && rangeIsLogarithmic(d->min, d->max);
    }
    return true;
}

inline std::vector<ModRoute> modRoutesFor(const std::vector<OrganismModel>& organisms,
                                          const std::function<int(const std::string&)>& indexOf) {
    std::vector<ModRoute> out;
    auto byName = [&](const std::string& n) -> const OrganismModel* {
        for (const auto& cm : organisms) if (cm.name == n) return &cm;
        return nullptr;
    };
    for (const auto& cm : organisms)
        for (const auto& s : cm.modSources) {
            if (!routeLivesInEngine(cm, s.propertyName)) continue;
            ModRoute r;
            r.dstParam = s.propertyName;
            ControlShape sh;
            sh.smoothing = s.smoothing;
            sh.curve = s.curve;
            sh.isSwitch = s.isSwitch;
            sh.inverted = s.inverted;
            sh.toggle = s.toggle;
            sh.threshold = s.threshold;
            if (fillModRoute(r, cm, byName(s.sourceOrganism), s.sourceValue, s.mapMin, s.mapMax, sh,
                             indexOf))
                out.push_back(std::move(r));
        }
    return out;
}

}
