// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"
#include "io/ModRouteBuild.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "core/params/ParamSchema.h"
#include "core/graph/PerfBox.h"
#include "gui/editor/ControlDefaults.h"
#include "core/graph/RtWord.h"

namespace hum {

std::string EngineHost::clockNodeName() {
    for (auto& cm : model_.organisms)
        if (cm.displayClass == "ClockPseudoSP") return cm.name;
    OrganismModel cm;
    cm.name = "Clock";
    while (model_.byName(cm.name) != nullptr) cm.name += "_";
    cm.classRaw = "ClockPseudoSP";
    cm.displayClass = "ClockPseudoSP";
    model_.organisms.push_back(std::move(cm));
    return model_.organisms.back().name;
}

std::string EngineHost::metapadNodeName() {
    for (auto& cm : model_.organisms)
        if (isMetapadPseudo(cm.displayClass)) return cm.name;
    OrganismModel cm;
    cm.name = "Metapad";
    while (model_.byName(cm.name) != nullptr) cm.name += "_";
    cm.classRaw = "MetasurfacePseudoSP";
    cm.displayClass = "MetasurfacePseudoSP";
    model_.organisms.push_back(std::move(cm));
    return model_.organisms.back().name;
}

}
