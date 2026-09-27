// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include "hum/caps/Graph.h"

namespace hum {

inline std::string inletNameFor(const std::string& raw) {
    std::string out;
    for (char c : raw) {
        if (c >= 'A' && c <= 'Z') c = (char) (c - 'A' + 'a');
        const bool keep = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
        if (!keep) c = '_';
        if (out.empty() && !(c >= 'a' && c <= 'z')) out += 'v';
        out += c;
    }
    if (out.size() >= (size_t) NamedInlet::kNameChars) out.resize((size_t) NamedInlet::kNameChars - 1);
    return out;
}

}
