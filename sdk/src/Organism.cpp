#include "hum/Organism.h"

namespace hum {

void Organism::loadFrom(const OrganismState& state) {
    for (const auto& p : state.properties)
        params.add(p);
}

}
