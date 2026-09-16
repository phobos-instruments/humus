// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cctype>
#include <string>

namespace hum {

inline std::string spacedParamName(const std::string& n) {
    std::string out;
    for (size_t i = 0; i < n.size(); ++i) {
        const unsigned char c = (unsigned char) n[i];
        if (i > 0 && std::isupper(c)) {
            const unsigned char prev = (unsigned char) n[i - 1];
            const bool afterWord = std::islower(prev) || std::isdigit(prev);
            const bool lastOfRun = std::isupper(prev) && i + 1 < n.size()
                                   && std::islower((unsigned char) n[i + 1]);
            if (afterWord || lastOfRun) out += ' ';
        }
        out += (char) c;
    }
    return out;
}

}
