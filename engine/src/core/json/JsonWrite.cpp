// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/json/Json.h"

namespace hum::json {

namespace {

void writeString(std::string& out, const std::string& s) {
    static constexpr char kHex[] = "0123456789abcdef";
    out += '"';
    for (const char ch : s) {
        const auto c = (unsigned char) ch;
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    out += "\\u00";
                    out += kHex[c >> 4];
                    out += kHex[c & 0xf];
                } else {
                    out += ch;
                }
        }
    }
    out += '"';
}

void writeValue(std::string& out, const Value& v) {
    switch (v.kind()) {
        case Value::Kind::Null: out += "null"; break;
        case Value::Kind::Bool: out += v.truthy() ? "true" : "false"; break;
        case Value::Kind::Int: out += v.text(); break;
        case Value::Kind::Double: out += numberText(v.number()); break;
        case Value::Kind::String: writeString(out, v.text()); break;
        case Value::Kind::Array: {
            out += '[';
            bool first = true;
            for (const auto& item : v.items()) {
                if (!first) out += ',';
                first = false;
                writeValue(out, item);
            }
            out += ']';
            break;
        }
        case Value::Kind::Object: {
            out += '{';
            bool first = true;
            for (const auto& [key, value] : v.members()) {
                if (!first) out += ',';
                first = false;
                writeString(out, key);
                out += ':';
                writeValue(out, value);
            }
            out += '}';
            break;
        }
    }
}

}

std::string write(const Value& value) {
    std::string out;
    writeValue(out, value);
    return out;
}

}
