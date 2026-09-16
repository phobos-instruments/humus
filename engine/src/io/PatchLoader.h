// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <string>

#include "core/graph/AudioGraph.h"
#include "io/PatchDocument.h"

namespace hum {

bool buildGraph(const PatchDocumentModel& doc, AudioGraph& graph, std::string& error,
                const std::function<const Organism*(const OrganismModel&)>& reuseLookup = {});

}
