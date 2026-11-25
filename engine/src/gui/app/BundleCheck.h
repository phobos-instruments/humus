// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

namespace hum::bundlecheck {

struct Report {
    std::vector<std::string> lines;
    int problems = 0;
};

Report run();

}
