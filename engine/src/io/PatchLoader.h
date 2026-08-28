#pragma once
#include <functional>
#include <string>

#include "core/AudioGraph.h"
#include "io/PatchDocument.h"

namespace hum {

bool buildGraph(const PatchDocumentModel& doc, AudioGraph& graph, std::string& error,
                const std::function<const Organism*(const OrganismModel&)>& reuseLookup = {});

}
