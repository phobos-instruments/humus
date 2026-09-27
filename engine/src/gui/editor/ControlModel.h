// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include "core/params/ParamSchema.h"
#include "gui/editor/LayoutCondition.h"
#include "hum/caps/Layout.h"
#include "gui/host/ModelHost.h"
#include "hum/Registry.h"
#include "io/PatchDocument.h"

namespace hum::control {

inline double paramValue(ModelHost& host, const std::string& organism, const std::string& param) {
    if (const auto* cm = host.model().byName(organism)) {
        for (const auto& pr : cm->properties)
            if (pr.name == param) {
                for (const auto& d : schemaFor(cm->classRaw))
                    if (d.name == param && d.isText) return pr.text.empty() ? 0.0 : 1.0;
                return pr.value;
            }
        for (const auto& d : schemaFor(cm->classRaw))
            if (d.name == param) return d.def;
    }
    return 0.0;
}

inline double defaultValue(ModelHost& host, const std::string& organism, const std::string& param) {
    if (const auto* cm = host.model().byName(organism))
        for (const auto& d : schemaFor(cm->classRaw))
            if (d.name == param) return d.def;
    return 0.0;
}

inline bool holds(ModelHost& host, const std::string& organism, const LayoutCondition& condition) {
    return !condition.empty()
           && condition.holds(
               [&host, &organism](const std::string& p) { return paramValue(host, organism, p); },
               [](const std::string& f) { return Registry::instance().flag(f); },
               [&host, &organism](const std::string& f) {
                   auto* facts = dynamic_cast<LayoutFacts*>(host.liveOrganism(organism));
                   return facts != nullptr && facts->layoutFact(f);
               });
}

}
