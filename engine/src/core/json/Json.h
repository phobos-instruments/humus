// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace hum::json {

class Value {
public:
    enum class Kind { Null, Bool, Int, Double, String, Array, Object };
    using Items = std::vector<Value>;
    using Members = std::vector<std::pair<std::string, Value>>;

    Value() = default;
    static Value fromBool(bool b);
    static Value fromInt(std::int64_t i);
    static Value fromDouble(double d);
    static Value fromString(std::string s);
    static Value fromItems(Items items);
    static Value fromMembers(Members members);

    Kind kind() const { return kind_; }
    bool isNull() const { return kind_ == Kind::Null; }
    bool isBool() const { return kind_ == Kind::Bool; }
    bool isNumber() const { return kind_ == Kind::Int || kind_ == Kind::Double; }
    bool isString() const { return kind_ == Kind::String; }
    bool isArray() const { return kind_ == Kind::Array; }
    bool isObject() const { return kind_ == Kind::Object; }

    const Value& operator[](std::string_view key) const;
    bool has(std::string_view key) const;
    const Items& items() const { return items_; }
    const Members& members() const { return members_; }
    void set(std::string key, Value value);
    void append(Value value) { items_.push_back(std::move(value)); }

    std::string text() const;
    double number() const;
    int integer() const;
    bool truthy() const;

private:
    Kind kind_ = Kind::Null;
    bool bool_ = false;
    std::int64_t int_ = 0;
    double double_ = 0.0;
    std::string string_;
    Items items_;
    Members members_;
};

Value parse(std::string_view text);
Value parseFile(const std::string& path);
bool readTextFile(const std::string& path, std::string& out);
std::string write(const Value& value);
std::string numberText(double d);

}
