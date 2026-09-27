// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include "core/browser/BrowserEntry.h"

namespace hum::browser {

inline constexpr double kPeaksLongest = 600.0;
inline constexpr double kLoopShortest = 2.0, kLoopLongest = 60.0;

Facts probeSound(const std::string& path);

}
