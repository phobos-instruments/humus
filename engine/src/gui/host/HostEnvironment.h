// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

namespace hum {

struct HostEnvironment {
    bool drivenByDaw = false;
    bool claimsMachineResources = true;
    bool sharesProcessWithOtherHosts = false;
    std::string helperProgram;
};

}
