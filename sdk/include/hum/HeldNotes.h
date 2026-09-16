// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <array>

#include "hum/caps/Midi.h"

namespace hum {

struct HeldNotes {
    std::array<int, 16> notes{};
    int count = 0;
    int lastVelocity = 0;

    void clear() { count = 0; }
    bool any() const { return count > 0; }
    int top() const { return count > 0 ? notes[(size_t) count - 1] : -1; }

    void on(int n, int vel) {
        off(n);
        if (count == (int) notes.size()) {
            std::copy(notes.begin() + 1, notes.end(), notes.begin());
            --count;
        }
        notes[(size_t) count++] = n;
        lastVelocity = vel;
    }
    void off(int n) {
        for (int i = 0; i < count; ++i) {
            if (notes[(size_t) i] != n) continue;
            std::copy(notes.begin() + i + 1, notes.begin() + count, notes.begin() + i);
            --count;
            return;
        }
    }
    void apply(const MidiEvent& e) {
        const int status = e.data[0] & 0xF0;
        if (status == 0x90 && e.data[2] > 0) on(e.data[1], e.data[2]);
        else if (status == 0x80 || status == 0x90) off(e.data[1]);
    }
};

}
