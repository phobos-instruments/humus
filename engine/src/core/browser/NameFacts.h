// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

namespace hum::browser {

struct NamedFacts {
    double bpm = 0.0;
    std::string key;
};

NamedFacts factsFromName(const std::string& fileName);

}
