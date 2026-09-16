// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <string>

namespace hum {

struct ControlCord {
    std::string src;
    int srcOutlet = 0;
    std::string dst;
    int dstInlet = 0;
};

}
