// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "hum/Number.h"
#include "hum/dsp/Formula.h"

namespace hum::serial {

constexpr int kMaxValues = 8;

struct Slot {
    float value = 0.0f;
    double scale = 1.0;
    bool integer = false;
    std::string name;
};

struct Reading {
    float value = 0.0f;
    std::string name;
};

enum class Field {
    None, Number, Byte, Word, Name, Words, Count, All, AllBytes, AllWords,
    Xor, Sum, XorHex, SumHex, SumStart, SumEnd, Expr, ExprByte, ExprWord, If
};

struct Placeholder {
    Field field = Field::None;
    int slot = -1;
    size_t length = 0;
    std::vector<std::string> words;
    std::string expr;
    std::string body;
};

inline bool isSlotDigit(char c) { return c >= '1' && c <= '8'; }

inline size_t matchingClose(const std::string& s, size_t open, char openCh, char closeCh) {
    int depth = 0;
    for (size_t k = open; k < s.size(); ++k) {
        if (s[k] == openCh) ++depth;
        else if (s[k] == closeCh && --depth == 0) return k;
    }
    return std::string::npos;
}

inline Placeholder placeholderAt(const std::string& s, size_t i) {
    Placeholder p;
    if (i + 1 >= s.size()) return p;
    const char d = s[i + 1];
    auto one = [&](Field f, size_t len) { p.field = f; p.length = len; return p; };
    auto exprAt = [&](size_t open, Field f) {
        const size_t close = matchingClose(s, open, '(', ')');
        if (close == std::string::npos) return p;
        p.expr = s.substr(open + 1, close - open - 1);
        if (f != Field::If) return one(f, close + 1 - i);
        if (close + 1 >= s.size() || s[close + 1] != '{') return p;
        const size_t end = matchingClose(s, close + 1, '{', '}');
        if (end == std::string::npos) return p;
        p.body = s.substr(close + 2, end - close - 2);
        return one(f, end + 1 - i);
    };
    if (isSlotDigit(d)) { p.slot = d - '1'; return one(Field::Number, 2); }
    if (d == '(') return exprAt(i + 1, Field::Expr);
    if (d == '?' && i + 2 < s.size() && s[i + 2] == '(') return exprAt(i + 2, Field::If);
    if ((d == 'b' || d == 'w') && i + 2 < s.size() && s[i + 2] == '(')
        return exprAt(i + 2, d == 'b' ? Field::ExprByte : Field::ExprWord);
    if (d == 'n' && i + 2 < s.size() && isSlotDigit(s[i + 2])) {
        p.slot = s[i + 2] - '1';
        return one(Field::Name, 3);
    }
    if (d == 'b' || d == 'w') {
        if (i + 2 < s.size() && isSlotDigit(s[i + 2])) {
            p.slot = s[i + 2] - '1';
            return one(d == 'b' ? Field::Byte : Field::Word, 3);
        }
        if (i + 2 < s.size() && s[i + 2] == '*') return one(d == 'b' ? Field::AllBytes : Field::AllWords, 3);
        return p;
    }
    switch (d) {
        case '*': return one(Field::All, 2);
        case 'c': return one(Field::Count, 2);
        case 'x': return one(Field::Xor, 2);
        case 's': return one(Field::Sum, 2);
        case 'X': return one(Field::XorHex, 2);
        case 'S': return one(Field::SumHex, 2);
        case '[': return one(Field::SumStart, 2);
        case ']': return one(Field::SumEnd, 2);
        default: break;
    }
    if (d != '{') return p;
    const size_t close = s.find('}', i + 2);
    if (close == std::string::npos || close + 1 >= s.size() || !isSlotDigit(s[close + 1])) return p;
    std::string word;
    for (size_t k = i + 2; k < close; ++k) {
        if (s[k] == ',') { p.words.push_back(word); word.clear(); }
        else word += s[k];
    }
    p.words.push_back(word);
    p.slot = s[close + 1] - '1';
    return one(Field::Words, close + 2 - i);
}

inline bool fieldIsBinary(Field f) {
    return f == Field::Byte || f == Field::Word || f == Field::AllBytes || f == Field::AllWords
        || f == Field::Count || f == Field::Xor || f == Field::Sum || f == Field::ExprByte
        || f == Field::ExprWord;
}

inline bool isBinaryPattern(const std::string& s) {
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] != '%') continue;
        const auto p = placeholderAt(s, i);
        if (fieldIsBinary(p.field)) return true;
        i += p.length > 0 ? p.length - 1 : (i + 1 < s.size() && s[i + 1] == '%' ? 1 : 0);
    }
    return false;
}

inline int hexDigit(char h) {
    if (h >= '0' && h <= '9') return h - '0';
    if (h >= 'a' && h <= 'f') return 10 + h - 'a';
    if (h >= 'A' && h <= 'F') return 10 + h - 'A';
    return -1;
}

inline size_t decodeEscape(const std::string& s, size_t i, std::string& out) {
    if (i + 1 >= s.size()) { out += '\\'; return 1; }
    const char e = s[i + 1];
    switch (e) {
        case 'n': out += '\n'; return 2;
        case 'r': out += '\r'; return 2;
        case 't': out += '\t'; return 2;
        case '0': out += '\0'; return 2;
        case '\\': out += '\\'; return 2;
        case 'x': {
            int value = 0, digits = 0;
            size_t j = i + 2;
            while (digits < 2 && j < s.size() && hexDigit(s[j]) >= 0) {
                value = value * 16 + hexDigit(s[j]);
                ++digits;
                ++j;
            }
            if (digits > 0) out += (char) value;
            return j - i;
        }
        default: out += e; return 2;
    }
}

inline long clampedWhole(double v, long hi) {
    const long w = std::lround(v);
    return w < 0 ? 0 : w > hi ? hi : w;
}

inline unsigned checksumOver(const char* data, size_t from, size_t to, bool xorNotSum) {
    unsigned acc = 0;
    for (size_t k = from; k < to; ++k) {
        const unsigned b = (unsigned char) data[k];
        acc = xorNotSum ? (acc ^ b) : ((acc + b) & 0xff);
    }
    return acc & 0xff;
}

inline std::string hexByte(unsigned v) {
    static const char* const digits = "0123456789ABCDEF";
    return {digits[(v >> 4) & 0xf], digits[v & 0xf]};
}

struct ChecksumRange {
    size_t start = 0;
    size_t end = std::string::npos;
    size_t endOr(size_t pos) const { return end == std::string::npos ? pos : end; }
};

inline void appendPlainNumber(std::string& out, double v, bool whole) {
    char num[32];
    if (whole) std::snprintf(num, sizeof(num), "%ld", std::lround(v));
    else std::snprintf(num, sizeof(num), "%.4f", v);
    fixDecimalPoint(num);
    out += num;
}

inline void appendNumber(std::string& out, const Slot& s) {
    appendPlainNumber(out, s.value * s.scale, s.integer);
}

inline bool isIdentChar(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

inline std::string bindSocketNames(const std::string& expr) {
    static const char* const kSlotVars[kMaxValues] = {"a", "b", "x", "y", "z", "w", "note", "freq"};
    std::string out;
    for (size_t i = 0; i < expr.size();) {
        if (!isIdentChar(expr[i])) { out += expr[i++]; continue; }
        size_t j = i;
        while (j < expr.size() && isIdentChar(expr[j])) ++j;
        const std::string ident = expr.substr(i, j - i);
        if (ident.size() == 2 && ident[0] == 'v' && isSlotDigit(ident[1])) out += kSlotVars[ident[1] - '1'];
        else if (ident == "n") out += "ch";
        else out += ident;
        i = j;
    }
    return out;
}

inline float evalSocketExpr(const std::string& expr, const Slot* slots, int count) {
    FormulaProgram prog;
    if (!compileFormula(bindSocketNames(expr).c_str(), prog)) return 0.0f;
    FormulaEnv env;
    const FVar order[kMaxValues] = {fvA, fvB, fvX, fvY, fvZ, fvW, fvNote, fvFreq};
    for (int k = 0; k < kMaxValues && k < count; ++k) env.v[order[k]] = slots[k].value;
    env.v[fvCh] = (float) count;
    const float v = evalFormula(prog, env);
    return std::isfinite(v) ? v : 0.0f;
}

inline void appendExpr(std::string& out, const Placeholder& p, const Slot* slots, int count) {
    const float v = evalSocketExpr(p.expr, slots, count);
    if (p.field == Field::ExprByte) { out += (char) (unsigned char) clampedWhole(v, 255); return; }
    if (p.field == Field::ExprWord) {
        const long w = clampedWhole(v, 65535);
        out += (char) (unsigned char) (w & 0xff);
        out += (char) (unsigned char) ((w >> 8) & 0xff);
        return;
    }
    appendPlainNumber(out, v, std::abs(v - std::round(v)) < 1e-6f);
}

inline void formatInto(std::string& out, const std::string& format, const Slot* slots, int count,
                       ChecksumRange& sum);

inline void appendByte(std::string& out, const Slot& s) {
    out += (char) (unsigned char) clampedWhole(s.value * s.scale, 255);
}

inline void appendWord(std::string& out, const Slot& s) {
    const long w = clampedWhole(s.value * s.scale, 65535);
    out += (char) (unsigned char) (w & 0xff);
    out += (char) (unsigned char) ((w >> 8) & 0xff);
}

inline void appendField(std::string& out, const Placeholder& p, const Slot* slots, int count,
                        ChecksumRange& sum) {
    switch (p.field) {
        case Field::Number: if (p.slot < count) appendNumber(out, slots[p.slot]); return;
        case Field::Byte: if (p.slot < count) appendByte(out, slots[p.slot]); return;
        case Field::Word: if (p.slot < count) appendWord(out, slots[p.slot]); return;
        case Field::Name: if (p.slot < count) out += slots[p.slot].name; return;
        case Field::Words:
            if (p.slot < count && !p.words.empty())
                out += p.words[(size_t) clampedWhole(slots[p.slot].value * slots[p.slot].scale,
                                                     (long) p.words.size() - 1)];
            return;
        case Field::Count: out += (char) (unsigned char) count; return;
        case Field::All:
            for (int k = 0; k < count; ++k) { if (k > 0) out += ' '; appendNumber(out, slots[k]); }
            return;
        case Field::AllBytes: for (int k = 0; k < count; ++k) appendByte(out, slots[k]); return;
        case Field::AllWords: for (int k = 0; k < count; ++k) appendWord(out, slots[k]); return;
        case Field::Xor: out += (char) checksumOver(out.data(), sum.start, sum.endOr(out.size()), true); return;
        case Field::Sum: out += (char) checksumOver(out.data(), sum.start, sum.endOr(out.size()), false); return;
        case Field::XorHex: out += hexByte(checksumOver(out.data(), sum.start, sum.endOr(out.size()), true)); return;
        case Field::SumHex: out += hexByte(checksumOver(out.data(), sum.start, sum.endOr(out.size()), false)); return;
        case Field::SumStart: sum.start = out.size(); sum.end = std::string::npos; return;
        case Field::SumEnd: sum.end = out.size(); return;
        case Field::Expr: case Field::ExprByte: case Field::ExprWord: appendExpr(out, p, slots, count); return;
        case Field::If:
            if (evalSocketExpr(p.expr, slots, count) != 0.0f) formatInto(out, p.body, slots, count, sum);
            return;
        case Field::None: return;
    }
}

inline void formatInto(std::string& out, const std::string& format, const Slot* slots, int count,
                       ChecksumRange& sum) {
    for (size_t i = 0; i < format.size(); ++i) {
        const char c = format[i];
        if (c == '%') {
            if (i + 1 < format.size() && format[i + 1] == '%') { out += '%'; ++i; continue; }
            const auto p = placeholderAt(format, i);
            if (p.field == Field::None) { out += c; continue; }
            i += p.length - 1;
            appendField(out, p, slots, count, sum);
            continue;
        }
        if (c == '\\') { i += decodeEscape(format, i, out) - 1; continue; }
        out += c;
    }
}

inline std::string formatLine(const std::string& format, const Slot* slots, int count) {
    std::string out;
    ChecksumRange sum;
    formatInto(out, format, slots, count, sum);
    return out;
}

inline std::string printable(const std::string& bytes) {
    std::string out;
    char hex[8];
    for (const char c : bytes) {
        const unsigned char u = (unsigned char) c;
        if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if (c == '\\') out += "\\\\";
        else if (u < 32 || u > 126) { std::snprintf(hex, sizeof(hex), "\\x%02X", u); out += hex; }
        else out += c;
    }
    return out;
}

}
