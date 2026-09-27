// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <atomic>
#include <thread>

namespace hum {

class SpinLock {
public:
    void lock() noexcept {
        for (int spins = 0; flag_.test_and_set(std::memory_order_acquire); ++spins)
            if (spins > 20) std::this_thread::yield();
    }
    bool try_lock() noexcept { return !flag_.test_and_set(std::memory_order_acquire); }
    void unlock() noexcept { flag_.clear(std::memory_order_release); }

private:
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};

}
