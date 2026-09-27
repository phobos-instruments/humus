// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include "gui/host/ModelHost.h"

namespace hum {

struct Words {
    const char* key;
    const char* written;
};

inline std::string say(const ModelHost& host, const Words& words) {
    return host.translated(words.key, words.written);
}

}
