// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstddef>
#include <vector>

namespace hum {

inline int firstFreeInletRun(const std::vector<char>& busy, int need) {
    const int count = (int) busy.size();
    if (need <= 0 || need > count) return 0;
    auto freeFrom = [&](int start) {
        for (int i = start; i < start + need; ++i)
            if (busy[(size_t) i]) return false;
        return true;
    };
    for (int s = 0; s + need <= count; s += need)
        if (freeFrom(s)) return s;
    for (int s = 0; s + need <= count; ++s)
        if (freeFrom(s)) return s;
    return 0;
}

}
