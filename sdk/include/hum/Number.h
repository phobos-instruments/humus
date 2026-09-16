// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <clocale>
#include <cmath>

namespace hum {

inline double scanDouble(const char* s, const char** end = nullptr) {
    const char* p = s;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == '\f' || *p == '\v') ++p;

    const bool neg = *p == '-';
    if (*p == '+' || *p == '-') ++p;

    double m = 0.0;
    int frac = 0;
    bool any = false;
    for (; *p >= '0' && *p <= '9'; ++p) { m = m * 10.0 + (*p - '0'); any = true; }
    if (*p == '.') {
        ++p;
        for (; *p >= '0' && *p <= '9'; ++p) { m = m * 10.0 + (*p - '0'); ++frac; any = true; }
    }
    if (!any) { if (end != nullptr) *end = s; return 0.0; }

    int exp = 0;
    if (*p == 'e' || *p == 'E') {
        const char* q = p + 1;
        const bool expNeg = *q == '-';
        if (*q == '+' || *q == '-') ++q;
        if (*q >= '0' && *q <= '9') {
            for (; *q >= '0' && *q <= '9'; ++q) exp = exp * 10 + (*q - '0');
            if (expNeg) exp = -exp;
            p = q;
        }
    }

    if (end != nullptr) *end = p;
    if (exp != frac) m *= std::pow(10.0, (double) (exp - frac));
    return neg ? -m : m;
}

inline void fixDecimalPoint(char* s) {
    const char sep = *std::localeconv()->decimal_point;
    if (sep == '.' || sep == '\0') return;
    for (; *s != '\0'; ++s)
        if (*s == sep) *s = '.';
}

}
