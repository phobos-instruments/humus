// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <ios>
#include <locale>
#include <sstream>
#include <string>

#include "hum/Number.h"

namespace hum {

inline std::string decimalText(double v, int decimals) {
    if (decimals <= 0) return std::to_string((long long) std::llround(v));
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out.setf(std::ios_base::fixed);
    out.precision((std::streamsize) decimals);
    out << v;
    return out.str();
}

inline std::string smartValueText(double v) {
    if (std::abs(v - std::round(v)) < 1e-9) return decimalText(v, 0);
    const double a = std::abs(v);
    const int dp = a >= 100.0 ? 1 : a >= 10.0 ? 2 : 3;
    auto t = decimalText(v, dp);
    while (t.find('.') != std::string::npos && t.back() == '0') t.pop_back();
    if (!t.empty() && t.back() == '.') t.pop_back();
    return t;
}

inline double leadingDouble(const std::string& text) {
    const char* start = text.c_str();
    const char* end = start;
    scanDouble(start, &end);
    if (end == start) return 0.0;
    std::istringstream in(std::string(start, end));
    in.imbue(std::locale::classic());
    double v = 0.0;
    in >> v;
    return in.fail() ? 0.0 : v;
}

}
