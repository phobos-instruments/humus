// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include "core/tuning/TuningProbe.h"

namespace hum {

namespace tuningProbeStore {

bool lookup(const std::string& classRaw, long long pluginMtimeMs, TuningProbeVerdict& out);

void store(const std::string& classRaw, long long pluginMtimeMs, TuningProbeVerdict v);

}
}
