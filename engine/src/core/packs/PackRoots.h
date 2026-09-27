// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

namespace hum {

class PackRoots {
public:
    virtual ~PackRoots() = default;
    virtual std::string builtinRoot() const = 0;
    virtual std::vector<std::string> builtinIds() const = 0;
};

}
