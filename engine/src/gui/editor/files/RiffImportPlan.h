// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "core/midi/RiffImport.h"
#include "gui/editor/Words.h"
#include "gui/editor/files/FilePick.h"
#include "gui/host/ModelHost.h"
#include "hum/PatternMatrix.h"
#include "hum/FileBytes.h"

namespace hum::files {

inline constexpr const char* kRiffFileWildcard = "*.mid;*.midi;*.syx;*.seq";
inline constexpr const char* kTripletResolution = "1/12";
inline constexpr const char* kStraightResolution = "1/16";

inline bool isRiffPath(const std::string& path) {
    const auto name = fileNameOf(path);
    const auto dot = name.rfind('.');
    if (dot == std::string::npos) return false;
    std::string ext = name.substr(dot + 1);
    for (auto& c : ext) c = (char) std::tolower((unsigned char) c);
    return ext == "mid" || ext == "midi" || ext == "syx" || ext == "seq";
}

inline std::vector<riff::Imported> readRiffPaths(const std::vector<std::string>& paths) {
    std::vector<riff::Imported> all;
    for (const auto& path : paths) {
        auto in = openForReading(path);
        if (!in) continue;
        const std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        auto got = riff::importBytes(reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size());
        for (auto& one : got) all.push_back(std::move(one));
    }
    return all;
}

struct RiffPlan {
    std::string resolution;
    int firstBank = 0;
    int placed = 0;
    bool overflow = false;
};

inline RiffPlan planRiffImport(const std::vector<riff::Imported>& all, int firstBank,
                               const std::string& currentResolution) {
    RiffPlan plan;
    plan.firstBank = firstBank;
    if (all.empty()) return plan;
    if (all.front().triplet) plan.resolution = kTripletResolution;
    else if (currentResolution == kTripletResolution) plan.resolution = kStraightResolution;
    while (plan.placed < (int) all.size() && firstBank + plan.placed < kPatternBanks) ++plan.placed;
    plan.overflow = plan.placed < (int) all.size();
    return plan;
}

inline constexpr Words kRiffImportTitle{"pattern-step-grid.import-title", "Import bassline patterns"};

inline FilePick riffPick(const ModelHost& host) {
    FilePick pick;
    pick.title = say(host, kRiffImportTitle);
    pick.patterns = kRiffFileWildcard;
    pick.multiple = true;
    return pick;
}

}
