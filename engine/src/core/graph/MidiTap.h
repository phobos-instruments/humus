// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <vector>

namespace hum {

struct TappedMidi {
    double beat = 0.0;
    int node = -1;
    unsigned char status = 0, d1 = 0, d2 = 0;
};

class MidiTapRing {
public:
    static constexpr std::size_t kCapacity = 4096;

    bool push(const TappedMidi& e) {
        const auto head = head_.load(std::memory_order_relaxed);
        const auto next = (head + 1) % kCapacity;
        if (next == tail_.load(std::memory_order_acquire)) return false;
        slots_[head] = e;
        head_.store(next, std::memory_order_release);
        return true;
    }

    void drainInto(std::vector<TappedMidi>& out) {
        auto tail = tail_.load(std::memory_order_relaxed);
        const auto head = head_.load(std::memory_order_acquire);
        while (tail != head) {
            out.push_back(slots_[tail]);
            tail = (tail + 1) % kCapacity;
        }
        tail_.store(tail, std::memory_order_release);
    }

private:
    std::array<TappedMidi, kCapacity> slots_{};
    std::atomic<std::size_t> head_{0}, tail_{0};
};

}
