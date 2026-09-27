// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <locale>
#include <sstream>
#include <string>
#include <vector>

#include "core/xml/Xml.h"

namespace hum::xmltext {

inline bool isSpace(char c) { return c == ' ' || (c >= 9 && c <= 13); }

inline std::string trimmed(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && isSpace(s[a])) ++a;
    while (b > a && isSpace(s[b - 1])) --b;
    return s.substr(a, b - a);
}

inline int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

inline std::string urlUnescaped(std::string s) {
    for (auto& c : s)
        if (c == '+') c = ' ';
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        const int hi = s[i] == '%' ? hexValue(i + 1 < s.size() ? s[i + 1] : 0) : -1;
        const int lo = hi >= 0 ? hexValue(i + 2 < s.size() ? s[i + 2] : 0) : -1;
        if (hi >= 0 && lo >= 0) {
            out += (char) ((hi << 4) + lo);
            i += 2;
        } else {
            out += s[i];
        }
    }
    return out;
}

inline std::vector<std::string> tokens(const std::string& text, const std::string& breaks) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : text) {
        if (breaks.find(c) != std::string::npos) {
            if (!cur.empty()) out.push_back(cur);
            cur.clear();
        } else {
            cur += c;
        }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

inline std::int64_t largeInt(const std::string& text) {
    size_t i = 0;
    while (i < text.size() && isSpace(text[i])) ++i;
    const bool negative = i < text.size() && text[i] == '-';
    if (negative || (i < text.size() && text[i] == '+')) ++i;
    std::uint64_t v = 0;
    for (; i < text.size() && text[i] >= '0' && text[i] <= '9'; ++i) v = v * 10u + (std::uint64_t) (text[i] - '0');
    return negative ? (std::int64_t) (0u - v) : (std::int64_t) v;
}

inline std::string plainNumber(double value) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << value;
    return out.str();
}

inline double textDouble(const xml::Element& e) { return xml::doubleValue(e.allSubText()); }

}
