// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/ControlApply.h"

#include <limits>

#include "core/params/ControlStep.h"
#include "core/params/ParamSchema.h"
#include "gui/host/ModelHost.h"
#include "io/PatchDocument.h"

namespace hum {

namespace {

const ParamDesc* describedParam(ModelHost& host, const std::string& organism,
                                const std::string& param) {
    const auto* cm = host.model().byName(organism);
    if (cm == nullptr) return nullptr;
    for (const auto& d : schemaFor(cm->classRaw))
        if (d.name == param) return &d;
    return nullptr;
}

}

double currentParamValue(ModelHost& host, const std::string& organism, const std::string& param) {
    if (host.model().byName(organism) == nullptr) return std::numeric_limits<double>::quiet_NaN();
    return host.liveParamValue(organism, param);
}

void applyControl(ModelHost& host, const ControlUpdate& u) {
    switch (u.kind) {
        case ControlActionKind::None: return;
        case ControlActionKind::Absolute:
            host.setParam(u.organism, u.param, u.value);
            return;
        case ControlActionKind::Delta: {
            const auto* d = describedParam(host, u.organism, u.param);
            const bool discrete = d != nullptr && (d->isBool || d->isEnum || d->isInt);
            host.setParam(u.organism, u.param,
                          steppedValue(host.liveParamValue(u.organism, u.param), u.value,
                                       u.rangeMin, u.rangeMax, u.logScale, discrete));
            return;
        }
        case ControlActionKind::Reset:
            if (const auto* d = describedParam(host, u.organism, u.param); d != nullptr && !d->isText)
                host.setParam(u.organism, u.param, d->def);
            return;
    }
}

}
