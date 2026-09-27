// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "core/browser/FileKind.h"

namespace hum::browser {

inline constexpr int kMaxRating = 5;
inline constexpr int kPeakCount = 64;

struct Facts {
    double seconds = 0.0;
    double sampleRate = 0.0;
    int channels = 0;
    double bpm = 0.0;
    std::string key;
    std::vector<std::uint8_t> peaks;
    int boxes = 0;
    std::string families;
    bool probed = false;
};

struct Entry {
    std::string path;
    Kind kind = Kind::Other;
    std::int64_t size = 0;
    std::int64_t modified = 0;
    std::int64_t created = 0;
    std::int64_t added = 0;
    std::int64_t lastUsed = 0;
    int rating = 0;
    bool favourite = false;
    std::vector<std::string> tags;
    Facts facts;

    bool annotated() const { return rating > 0 || favourite || !tags.empty() || lastUsed > 0; }
};

struct Collection {
    std::string name;
    std::vector<std::string> paths;
};

}
