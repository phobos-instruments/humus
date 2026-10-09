// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include <utility>
#include <vector>

#include "core/app/Catalogue.h"
#include "gui/host/ModelHost.h"

namespace hum {

struct Words {
    const char* key;
    const char* written;
};

inline std::string say(const ModelHost& host, const Words& words) {
    return host.translated(words.key, words.written);
}

inline std::string say(const ModelHost& host, const Words& words,
                       const std::vector<std::pair<std::string, std::string>>& values) {
    return fillPlaceholders(host.translated(words.key, words.written), values);
}

inline std::string trn(const char* key, const char* one, const char* other, long long n, const ModelHost& host) {
    return host.translatedCount(key, one, other, n);
}

}
