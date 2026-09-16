// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

namespace hum {

inline int parseBoundedInt(const std::string& digits, int maxDigits = 6) {
    if (digits.empty() || (int) digits.size() > maxDigits) return -1;
    int value = 0;
    for (const char c : digits) {
        if (c < '0' || c > '9') return -1;
        value = value * 10 + (c - '0');
    }
    return value;
}

}
