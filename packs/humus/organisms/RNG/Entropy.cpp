// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "RNG/Entropy.h"

#include <chrono>
#include <cstdint>
#include <random>

namespace hum::rng {

std::uint64_t osEntropy() {
    std::random_device device;
    std::uint64_t bits = 0;
    for (int i = 0; i < 8; ++i) bits = (bits << 8) | (std::uint64_t) (device() & 0xffu);
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return bits ^ (std::uint64_t) std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
}

}
