// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

namespace hum {

class LayoutFacts {
public:
    virtual ~LayoutFacts() = default;
    virtual bool layoutFact(const std::string& name) const = 0;
};

}
