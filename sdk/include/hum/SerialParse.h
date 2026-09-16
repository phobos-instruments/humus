// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <cstring>
#include <string>

#include "hum/SerialCodec.h"

namespace hum::serial {

constexpr int kNeedMore = -2;

inline bool isSeparator(char c) {
    return c == ' ' || c == '\t' || c == ',' || c == ';' || c == ':' || c == '=' || c == '\r' || c == '\n';
}

inline int parseLine(const char* line, Reading* out, int maxVals) {
    int n = 0;
    std::string pendingName;
    const char* p = line;
    while (n < maxVals && *p != '\0') {
        while (isSeparator(*p)) ++p;
        if (*p == '\0') break;
        const char* end = nullptr;
        const auto v = (float) scanDouble(p, &end);
        const bool numberEnds = end != p && (*end == '\0' || (isSeparator(*end) && *end != ':' && *end != '='));
        if (numberEnds) {
            p = end;
            if (!std::isfinite(v)) { pendingName.clear(); continue; }
            out[n].value = v;
            out[n].name = pendingName;
            pendingName.clear();
            ++n;
            continue;
        }
        const char* start = p;
        while (*p != '\0' && !isSeparator(*p)) ++p;
        pendingName.assign(start, (size_t) (p - start));
    }
    return n;
}

inline int parseFloats(const char* line, float* out, int maxVals) {
    Reading readings[kMaxValues];
    const int n = parseLine(line, readings, maxVals < kMaxValues ? maxVals : kMaxValues);
    for (int i = 0; i < n; ++i) out[i] = readings[i].value;
    return n;
}

struct Matcher {
    const std::string& pattern;
    const char* data;
    size_t len;
    bool complete;
    Reading* out;
    bool* captured;
    int maxVals;
    size_t pos = 0;
    int n = 0;
    int pendingCount = -1;
    ChecksumRange sum;

    int fail() const { return complete ? -1 : kNeedMore; }

    void capture(int slot, float v) {
        if (slot < 0 || slot >= maxVals) return;
        out[slot].value = v;
        captured[slot] = true;
        ++n;
    }

    int number(float& v) {
        size_t p = pos;
        while (p < len && (data[p] == ' ' || data[p] == '\t')) ++p;
        char tmp[32];
        size_t k = 0;
        while (k < sizeof(tmp) - 1 && p + k < len) { tmp[k] = data[p + k]; ++k; }
        tmp[k] = '\0';
        if (k == 0) return fail();
        const char* end = nullptr;
        v = (float) scanDouble(tmp, &end);
        if (end == tmp || !std::isfinite(v)) return -1;
        if ((size_t) (end - tmp) == k && !complete) return kNeedMore;
        pos = p + (size_t) (end - tmp);
        return 0;
    }

    int bytes(size_t need, float& v) {
        if (pos + need > len) return fail();
        const unsigned lo = (unsigned char) data[pos];
        const unsigned hi = need == 2 ? (unsigned char) data[pos + 1] : 0u;
        v = need == 1 ? (float) lo / 255.0f : (float) (lo | (hi << 8)) / 65535.0f;
        pos += need;
        return 0;
    }

    int name(int slot) {
        size_t p = pos;
        while (p < len && (data[p] == ' ' || data[p] == '\t')) ++p;
        const size_t start = p;
        while (p < len && !isSeparator(data[p])) ++p;
        if (p == start) return fail();
        if (p >= len && !complete) return kNeedMore;
        if (slot >= 0 && slot < maxVals) out[slot].name.assign(data + start, p - start);
        pos = p;
        return 0;
    }

    int words(const Placeholder& p) {
        int best = -1;
        size_t bestLen = 0;
        bool truncated = false;
        for (size_t w = 0; w < p.words.size(); ++w) {
            const auto& word = p.words[w];
            const size_t avail = len - pos;
            const size_t cmp = word.size() < avail ? word.size() : avail;
            if (std::memcmp(data + pos, word.data(), cmp) != 0) continue;
            if (cmp < word.size()) { truncated = true; continue; }
            if (word.size() >= bestLen) { best = (int) w; bestLen = word.size(); }
        }
        if (best < 0) return truncated && !complete ? kNeedMore : -1;
        pos += bestLen;
        capture(p.slot, p.words.size() > 1 ? (float) best / (float) (p.words.size() - 1) : 1.0f);
        return 0;
    }

    int list(const Placeholder& p) {
        if (p.field == Field::All) {
            for (int slot = 0; slot < maxVals; ++slot) {
                float v = 0.0f;
                const int r = number(v);
                if (r == kNeedMore) return r;
                if (r < 0) break;
                capture(slot, v);
            }
            return 0;
        }
        if (pendingCount < 0) return -1;
        const size_t each = p.field == Field::AllBytes ? 1 : 2;
        if (pos + each * (size_t) pendingCount > len) return fail();
        for (int k = 0; k < pendingCount; ++k) {
            float v = 0.0f;
            bytes(each, v);
            if (k < maxVals) capture(k, v);
        }
        return 0;
    }

    int checksum(Field f) {
        const bool xorNotSum = f == Field::Xor || f == Field::XorHex;
        const unsigned want = checksumOver(data, sum.start, sum.endOr(pos), xorNotSum);
        if (f == Field::Xor || f == Field::Sum) {
            if (pos >= len) return fail();
            if ((unsigned char) data[pos] != want) return -1;
            ++pos;
            return 0;
        }
        if (pos + 2 > len) return fail();
        const int hi = hexDigit(data[pos]), lo = hexDigit(data[pos + 1]);
        if (hi < 0 || lo < 0 || (unsigned) (hi * 16 + lo) != want) return -1;
        pos += 2;
        return 0;
    }

    int field(const Placeholder& p) {
        float v = 0.0f;
        int r = 0;
        switch (p.field) {
            case Field::Number: if ((r = number(v)) == 0) capture(p.slot, v); return r;
            case Field::Byte: if ((r = bytes(1, v)) == 0) capture(p.slot, v); return r;
            case Field::Word: if ((r = bytes(2, v)) == 0) capture(p.slot, v); return r;
            case Field::Name: return name(p.slot);
            case Field::Words: return words(p);
            case Field::Count:
                if (pos >= len) return fail();
                pendingCount = (unsigned char) data[pos++];
                return 0;
            case Field::All: case Field::AllBytes: case Field::AllWords: return list(p);
            case Field::Xor: case Field::Sum: case Field::XorHex: case Field::SumHex: return checksum(p.field);
            case Field::SumStart: sum.start = pos; sum.end = std::string::npos; return 0;
            case Field::SumEnd: sum.end = pos; return 0;
            case Field::Expr: case Field::ExprByte: case Field::ExprWord: case Field::If: return -1;
            case Field::None: return 0;
        }
        return 0;
    }

    int literal(const std::string& text) {
        for (const char want : text) {
            if (want == ' ') {
                while (pos < len && (data[pos] == ' ' || data[pos] == '\t')) ++pos;
                continue;
            }
            if (pos >= len) return fail();
            if (data[pos] != want) return -1;
            ++pos;
        }
        return 0;
    }

    int run() {
        for (int i = 0; i < maxVals; ++i) { captured[i] = false; out[i].name.clear(); }
        for (size_t i = 0; i < pattern.size(); ++i) {
            const char c = pattern[i];
            std::string text;
            if (c == '%' && i + 1 < pattern.size() && pattern[i + 1] == '%') {
                text = "%";
                ++i;
            } else if (c == '%') {
                const auto p = placeholderAt(pattern, i);
                if (p.field != Field::None) {
                    i += p.length - 1;
                    if (const int r = field(p); r != 0) return r;
                    continue;
                }
                text = "%";
            } else if (c == '\\') {
                i += decodeEscape(pattern, i, text) - 1;
            } else {
                text = c;
            }
            if (const int r = literal(text); r != 0) return r;
        }
        return n;
    }
};

inline int matchBuffer(const std::string& pattern, const char* data, size_t len, bool complete,
                       Reading* out, bool* captured, int maxVals, size_t& consumed) {
    Matcher m{pattern, data, len, complete, out, captured, maxVals};
    const int r = m.run();
    consumed = m.pos;
    return r;
}

inline int matchLine(const std::string& pattern, const char* line, Reading* out, bool* captured,
                     int maxVals) {
    size_t consumed = 0;
    return matchBuffer(pattern, line, std::strlen(line), true, out, captured, maxVals, consumed);
}

}
