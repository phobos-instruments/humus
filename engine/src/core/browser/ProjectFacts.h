// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include "core/browser/BrowserEntry.h"

namespace hum::browser {

inline constexpr int kFamiliesMost = 64;

std::string patchOf(const std::string& projectOrPatch);
Facts probeProject(const std::string& projectOrPatch);

}
