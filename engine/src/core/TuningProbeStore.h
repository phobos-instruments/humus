#pragma once
#include <string>

#include "core/TuningProbe.h"

namespace hum {

namespace tuningProbeStore {

bool lookup(const std::string& classRaw, long long pluginMtimeMs, TuningProbeVerdict& out);

void store(const std::string& classRaw, long long pluginMtimeMs, TuningProbeVerdict v);

}
}
