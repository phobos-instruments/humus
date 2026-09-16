// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <atomic>
#include <mutex>
#include <utility>

namespace hum {

template <typename T>
class Prepared {
public:
    void publish(T fresh) {
        T retired;
        {
            const std::lock_guard<std::mutex> g(m_);
            retired = std::move(pending_);
            pending_ = std::move(fresh);
            hasPending_.store(true, std::memory_order_release);
        }
    }

    bool adopt(T& live) {
        if (!hasPending_.load(std::memory_order_acquire)) return false;
        const std::unique_lock<std::mutex> g(m_, std::try_to_lock);
        if (!g.owns_lock()) return false;
        std::swap(live, pending_);
        hasPending_.store(false, std::memory_order_relaxed);
        return true;
    }

    bool pending() const { return hasPending_.load(std::memory_order_acquire); }

private:
    std::mutex m_;
    T pending_{};
    std::atomic<bool> hasPending_{false};
};

}
