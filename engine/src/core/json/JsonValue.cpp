// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/json/Json.h"

#include <cctype>
#include <climits>
#include <cstdint>
#include <cmath>
#include <locale>
#include <sstream>

#include "core/params/ValueText.h"

namespace hum::json {

namespace {

const Value& nullValue() {
    static const Value v;
    return v;
}

std::int64_t leadingInt(const std::string& s) {
    size_t i = 0;
    while (i < s.size() && std::isspace((unsigned char) s[i])) ++i;
    const bool negative = i < s.size() && s[i] == '-';
    if (i < s.size() && (s[i] == '-' || s[i] == '+')) ++i;
    std::uint64_t v = 0;
    for (; i < s.size() && s[i] >= '0' && s[i] <= '9'; ++i) v = v * 10 + (std::uint64_t) (s[i] - '0');
    return negative ? -(std::int64_t) v : (std::int64_t) v;
}

bool sameWord(const std::string& s, const char* word) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char) s[a])) ++a;
    while (b > a && std::isspace((unsigned char) s[b - 1])) --b;
    size_t n = 0;
    for (; word[n] != 0; ++n)
        if (a + n >= b || std::tolower((unsigned char) s[a + n]) != word[n]) return false;
    return a + n == b;
}

}

Value Value::fromBool(bool b) {
    Value v;
    v.kind_ = Kind::Bool;
    v.bool_ = b;
    return v;
}

Value Value::fromInt(std::int64_t i) {
    Value v;
    v.kind_ = Kind::Int;
    v.int_ = i;
    return v;
}

Value Value::fromDouble(double d) {
    Value v;
    v.kind_ = Kind::Double;
    v.double_ = d;
    return v;
}

Value Value::fromString(std::string s) {
    Value v;
    v.kind_ = Kind::String;
    v.string_ = std::move(s);
    return v;
}

Value Value::fromItems(Items items) {
    Value v;
    v.kind_ = Kind::Array;
    v.items_ = std::move(items);
    return v;
}

Value Value::fromMembers(Members members) {
    Value v;
    v.kind_ = Kind::Object;
    for (auto& [key, value] : members) v.set(std::move(key), std::move(value));
    return v;
}

const Value& Value::operator[](std::string_view key) const {
    for (const auto& [k, v] : members_)
        if (k == key) return v;
    return nullValue();
}

bool Value::has(std::string_view key) const {
    for (const auto& m : members_)
        if (m.first == key) return true;
    return false;
}

void Value::set(std::string key, Value value) {
    for (auto& m : members_)
        if (m.first == key) {
            m.second = std::move(value);
            return;
        }
    members_.emplace_back(std::move(key), std::move(value));
}

std::string Value::text() const {
    switch (kind_) {
        case Kind::Bool: return bool_ ? "1" : "0";
        case Kind::Int: return std::to_string(int_);
        case Kind::Double: return numberText(double_);
        case Kind::String: return string_;
        case Kind::Array: return "[Array]";
        default: return {};
    }
}

double Value::number() const {
    switch (kind_) {
        case Kind::Bool: return bool_ ? 1.0 : 0.0;
        case Kind::Int: return (double) int_;
        case Kind::Double: return double_;
        case Kind::String: return leadingDouble(string_);
        default: return 0.0;
    }
}

int Value::integer() const {
    switch (kind_) {
        case Kind::Bool: return bool_ ? 1 : 0;
        case Kind::Int: return (int) int_;
        case Kind::Double:
            if (!(std::abs(double_) < (double) INT_MAX)) return 0;
            return (int) double_;
        case Kind::String: return (int) leadingInt(string_);
        default: return 0;
    }
}

bool Value::truthy() const {
    switch (kind_) {
        case Kind::Bool: return bool_;
        case Kind::Int: return int_ != 0;
        case Kind::Double: return double_ != 0.0;
        case Kind::String:
            return leadingInt(string_) != 0 || sameWord(string_, "true") || sameWord(string_, "yes");
        case Kind::Array:
        case Kind::Object: return true;
        default: return false;
    }
}

std::string numberText(double d) {
    if (!std::isfinite(d)) return "0";
    if (std::abs(d) < 1.0e15 && d == std::trunc(d)) {
        std::string s = std::to_string((long long) d);
        return (d == 0.0 && std::signbit(d) ? "-" : "") + s + ".0";
    }
    for (int precision = 15; precision <= 17; ++precision) {
        std::ostringstream out;
        out.imbue(std::locale::classic());
        out.precision(precision);
        out << d;
        if (leadingDouble(out.str()) == d || precision == 17) return out.str();
    }
    return {};
}

}
