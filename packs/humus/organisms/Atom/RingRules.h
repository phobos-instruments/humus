// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

#include "hum/Scale.h"
#include "hum/Tuning.h"
#include "hum/dsp/DspMath.h"

namespace hum::rings {

inline constexpr int kMaxHeld = 24;
inline constexpr int kMaxPool = kMaxHeld * 4;
inline constexpr int kWalkRoot = 48;

enum Order { kUp = 0, kDown, kUpDown, kRandom, kPlayed };

class HeldNotes {
public:
    void clear() { count_ = 0; down_ = 0; }
    int size() const { return count_; }
    int at(int i) const { return notes_[(size_t) i]; }

    void press(int note, bool latch) {
        if (latch && down_ == 0) count_ = 0;
        ++down_;
        drop(note);
        if (count_ < kMaxHeld) notes_[(size_t) count_++] = note;
    }
    void release(int note, bool latch) {
        down_ = std::max(0, down_ - 1);
        if (!latch) drop(note);
    }
    void unlatch() {
        if (down_ == 0) count_ = 0;
    }

private:
    void drop(int note) {
        int kept = 0;
        for (int i = 0; i < count_; ++i)
            if (notes_[(size_t) i] != note) notes_[(size_t) kept++] = notes_[(size_t) i];
        count_ = kept;
    }
    std::array<int, kMaxHeld> notes_{};
    int count_ = 0;
    int down_ = 0;
};

inline int snapped(int note, const Scale& scale, const Tuning& tuning, int key) {
    for (int away = 0; away < 12; ++away) {
        if (scale.allows(tuning, note - away, key)) return note - away;
        if (scale.allows(tuning, note + away, key)) return note + away;
    }
    return note;
}

struct Pool {
    std::array<int, kMaxPool> notes{};
    int count = 0;
    void add(int note) {
        if (note < 0 || note > kMidiMax || count >= kMaxPool) return;
        for (int i = 0; i < count; ++i)
            if (notes[(size_t) i] == note) return;
        notes[(size_t) count++] = note;
    }
};

inline Pool poolOf(const HeldNotes& held, const Scale& scale, const Tuning& tuning, int key,
                   int octaves, bool keepPlayedOrder) {
    Pool once;
    if (held.size() == 0) {
        for (int n = kWalkRoot + key; n < kWalkRoot + key + 12; ++n)
            if (scale.allows(tuning, n, key)) once.add(n);
    } else {
        for (int i = 0; i < held.size(); ++i) once.add(snapped(held.at(i), scale, tuning, key));
        if (!keepPlayedOrder) std::sort(once.notes.begin(), once.notes.begin() + once.count);
    }
    Pool pool;
    for (int o = 0; o < std::clamp(octaves, 1, 4); ++o)
        for (int i = 0; i < once.count; ++i) pool.add(once.notes[(size_t) i] + 12 * o);
    return pool;
}

inline std::uint32_t nextRandom(std::uint32_t& state) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

inline int pick(int poolSize, int order, long long turn, std::uint32_t& random) {
    if (poolSize < 2) return 0;
    const int at = (int) (turn % poolSize);
    if (order == kDown) return poolSize - 1 - at;
    if (order == kRandom) return (int) (nextRandom(random) % (std::uint32_t) poolSize);
    if (order != kUpDown) return at;
    const int lap = 2 * poolSize - 2;
    const int swing = (int) (turn % lap);
    return swing < poolSize ? swing : lap - swing;
}

}
