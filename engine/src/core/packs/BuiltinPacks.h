// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <vector>

namespace hum {

class Registry;

struct BuiltinPack {
    const char* id;
    void (*registerFactories)(Registry&);
};

const std::vector<BuiltinPack>& builtinPacks();

}
