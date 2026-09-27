// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstddef>
#include <cstring>
#include <string>

namespace hum {

enum class RangeEnd { Whole, Low, High, Spread };

enum class RangeMode { ValueSpread, MinMax, Single };

inline const char* rangeModeName(RangeMode mode) {
    switch (mode) {
        case RangeMode::MinMax: return "ends";
        case RangeMode::Single: return "single";
        case RangeMode::ValueSpread:
        default:                return "span";
    }
}

inline RangeMode rangeModeNamed(const std::string& name) {
    if (name == "ends") return RangeMode::MinMax;
    if (name == "single") return RangeMode::Single;
    return RangeMode::ValueSpread;
}

inline constexpr const char* kRangeLowSuffix = " (min)";
inline constexpr const char* kRangeHighSuffix = " (max)";
inline constexpr const char* kRangeSpreadSuffix = " (spread)";

inline bool paramNameEndsWith(const std::string& param, const char* suffix) {
    const std::size_t n = std::strlen(suffix);
    return param.size() > n && param.compare(param.size() - n, n, suffix) == 0;
}

inline RangeEnd rangeEndOf(const std::string& param) {
    if (paramNameEndsWith(param, kRangeLowSuffix)) return RangeEnd::Low;
    if (paramNameEndsWith(param, kRangeHighSuffix)) return RangeEnd::High;
    if (paramNameEndsWith(param, kRangeSpreadSuffix)) return RangeEnd::Spread;
    return RangeEnd::Whole;
}

inline std::string rangeBaseOf(const std::string& param) {
    switch (rangeEndOf(param)) {
        case RangeEnd::Low:  return param.substr(0, param.size() - std::strlen(kRangeLowSuffix));
        case RangeEnd::High: return param.substr(0, param.size() - std::strlen(kRangeHighSuffix));
        case RangeEnd::Spread: return param.substr(0, param.size() - std::strlen(kRangeSpreadSuffix));
        case RangeEnd::Whole:
        default:             return param;
    }
}

inline std::string rangeEndParam(const std::string& base, RangeEnd end) {
    switch (end) {
        case RangeEnd::Low:  return base + kRangeLowSuffix;
        case RangeEnd::High: return base + kRangeHighSuffix;
        case RangeEnd::Spread: return base + kRangeSpreadSuffix;
        case RangeEnd::Whole:
        default:             return base;
    }
}

}
