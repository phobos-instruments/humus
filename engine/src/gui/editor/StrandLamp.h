// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdlib>
#include <string>

#include "hum/caps/Files.h"

namespace hum::strandlamp {

inline constexpr int kStates = 5;
inline constexpr unsigned kBlinkMs = 250;

struct Rule {
    int strand = -1;
    int litMask = 0;
    int dimMask = 0;
    bool dimWithoutUndo = false;
    bool dimWithoutRedo = false;
    int pendingPress = StrandIntent::kPressNone;
    bool active() const { return strand >= 0; }
};

struct Lamp {
    bool lit = false;
    bool dim = false;
    bool pending = false;
    int tint = StrandStatus::kEmpty;
};

inline int stateNamed(const std::string& word) {
    if (word == "empty") return StrandStatus::kEmpty;
    if (word == "record") return StrandStatus::kRecord;
    if (word == "play") return StrandStatus::kPlay;
    if (word == "dub") return StrandStatus::kDub;
    if (word == "stopped") return StrandStatus::kStopped;
    return -1;
}

inline int pressNamed(const std::string& word) {
    if (word == "record") return StrandIntent::kPressRecord;
    if (word == "dub") return StrandIntent::kPressDub;
    if (word == "play") return StrandIntent::kPressPlay;
    if (word == "stop") return StrandIntent::kPressStop;
    return StrandIntent::kPressNone;
}

inline int stateOfPress(int press) {
    switch (press) {
        case StrandIntent::kPressRecord: return StrandStatus::kRecord;
        case StrandIntent::kPressDub: return StrandStatus::kDub;
        case StrandIntent::kPressPlay: return StrandStatus::kPlay;
        case StrandIntent::kPressStop: return StrandStatus::kStopped;
        default: return StrandStatus::kEmpty;
    }
}

template <class Fn>
inline void forEachWord(const std::string& list, Fn&& fn) {
    size_t start = 0;
    while (start <= list.size()) {
        const auto comma = list.find(',', start);
        const auto end = comma == std::string::npos ? list.size() : comma;
        auto first = list.find_first_not_of(' ', start);
        auto last = list.find_last_not_of(' ', end == 0 ? 0 : end - 1);
        if (first != std::string::npos && first < end && last != std::string::npos && last >= first)
            fn(list.substr(first, last - first + 1));
        if (comma == std::string::npos) break;
        start = comma + 1;
    }
}

inline Rule parse(const std::string& strand, const std::string& litIn, const std::string& dimIn,
                  const std::string& pending) {
    Rule r;
    const int n = std::atoi(strand.c_str());
    if (n <= 0) return r;
    r.strand = n - 1;
    forEachWord(litIn, [&r](const std::string& w) {
        if (const int s = stateNamed(w); s >= 0) r.litMask |= 1 << s;
    });
    forEachWord(dimIn, [&r](const std::string& w) {
        if (w == "no-undo") r.dimWithoutUndo = true;
        else if (w == "no-redo") r.dimWithoutRedo = true;
        else if (const int s = stateNamed(w); s >= 0) r.dimMask |= 1 << s;
    });
    r.pendingPress = pressNamed(pending);
    return r;
}

inline Lamp evaluate(const Rule& r, int state, int pendingPress, bool canUndo, bool canRedo) {
    Lamp l;
    const bool known = state >= 0 && state < kStates;
    l.lit = known && ((r.litMask >> state) & 1) != 0;
    l.dim = (known && ((r.dimMask >> state) & 1) != 0) || (r.dimWithoutUndo && !canUndo)
            || (r.dimWithoutRedo && !canRedo);
    l.pending = r.pendingPress != StrandIntent::kPressNone && r.pendingPress == pendingPress;
    l.tint = l.pending ? stateOfPress(r.pendingPress) : (known ? state : StrandStatus::kEmpty);
    return l;
}

}
