// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <set>
#include <string>
#include <vector>

namespace hum::history {

inline constexpr std::int64_t kMinuteMs = 60 * 1000;
inline constexpr std::int64_t kHourMs = 60 * kMinuteMs;
inline constexpr std::int64_t kDayMs = 24 * kHourMs;
inline constexpr std::int64_t kKeepEveryMs = kHourMs;
inline constexpr std::int64_t kKeepHourlyMs = kDayMs;
inline constexpr std::int64_t kKeepDailyMs = 30 * kDayMs;
inline constexpr std::uint64_t kByteCap = 200ull * 1024ull * 1024ull;

struct Stamp {
    std::int64_t at = 0;
    bool kept = false;
    std::uint64_t bytes = 0;
};

inline std::vector<std::size_t> toDrop(const std::vector<Stamp>& items, std::int64_t now,
                                       std::uint64_t byteCap = kByteCap) {
    std::vector<std::size_t> newestFirst(items.size());
    std::iota(newestFirst.begin(), newestFirst.end(), std::size_t{0});
    std::stable_sort(newestFirst.begin(), newestFirst.end(),
                     [&](std::size_t a, std::size_t b) { return items[a].at > items[b].at; });

    std::vector<bool> drop(items.size(), false);
    std::set<std::int64_t> hours, days;
    for (std::size_t i : newestFirst) {
        const auto& s = items[i];
        if (s.kept) continue;
        const bool newest = i == newestFirst.front();
        const std::int64_t age = now - s.at;
        if (age <= kKeepEveryMs) continue;
        bool fresh = true;
        if (age <= kKeepHourlyMs) fresh = hours.insert(s.at / kHourMs).second;
        else if (age <= kKeepDailyMs) fresh = days.insert(s.at / kDayMs).second;
        else fresh = false;
        drop[i] = !newest && !fresh;
    }

    std::uint64_t total = 0;
    for (std::size_t i = 0; i < items.size(); ++i)
        if (!drop[i]) total += items[i].bytes;
    for (auto it = newestFirst.rbegin(); it != newestFirst.rend() && total > byteCap; ++it) {
        const std::size_t i = *it;
        if (drop[i] || items[i].kept || i == newestFirst.front()) continue;
        drop[i] = true;
        total -= items[i].bytes;
    }

    std::vector<std::size_t> out;
    for (std::size_t i = 0; i < items.size(); ++i)
        if (drop[i]) out.push_back(i);
    return out;
}

enum class Ago { JustNow, Minutes, Hours, Yesterday, Days };

struct AgoText {
    Ago unit = Ago::JustNow;
    std::int64_t count = 0;
};

inline AgoText agoOf(std::int64_t at, std::int64_t now) {
    const std::int64_t age = std::max<std::int64_t>(0, now - at);
    if (age < kMinuteMs) return {Ago::JustNow, 0};
    if (age < kHourMs) return {Ago::Minutes, age / kMinuteMs};
    if (age < kDayMs) return {Ago::Hours, age / kHourMs};
    if (age < 2 * kDayMs) return {Ago::Yesterday, 1};
    return {Ago::Days, age / kDayMs};
}

}
