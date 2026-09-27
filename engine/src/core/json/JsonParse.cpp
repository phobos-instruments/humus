// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/json/Json.h"

#include <cstdint>
#include <fstream>
#include <sstream>

#include "core/params/ValueText.h"
#include "hum/Number.h"
#include "hum/FileBytes.h"

namespace hum::json {

namespace {

constexpr int kMaxDepth = 512;

struct Broken {};

class Parser {
public:
    explicit Parser(std::string_view text) : text_(text) {}

    Value document() {
        skipSpace();
        if (take('{')) return object(0);
        if (take('[')) return array(0);
        if (!atEnd()) throw Broken{};
        return {};
    }

private:
    bool atEnd() const { return at_ >= text_.size() || text_[at_] == '\0'; }
    char peek() const { return atEnd() ? '\0' : text_[at_]; }
    char next() { return atEnd() ? '\0' : text_[at_++]; }

    bool take(char c) {
        if (peek() != c) return false;
        ++at_;
        return true;
    }

    void skipSpace() {
        for (char c = peek(); c == ' ' || (c >= '\t' && c <= '\r'); c = peek()) ++at_;
    }

    void expectWord(const char* rest) {
        while (*rest != 0)
            if (!take(*rest++)) throw Broken{};
    }

    Value any(int depth) {
        if (depth > kMaxDepth) throw Broken{};
        skipSpace();
        const char c = next();
        switch (c) {
            case '{': return object(depth + 1);
            case '[': return array(depth + 1);
            case '"':
            case '\'': return Value::fromString(string(c));
            case '-':
                skipSpace();
                return number(true);
            case 't': expectWord("rue"); return Value::fromBool(true);
            case 'f': expectWord("alse"); return Value::fromBool(false);
            case 'n': expectWord("ull"); return {};
            default:
                if (c >= '0' && c <= '9') {
                    --at_;
                    return number(false);
                }
                throw Broken{};
        }
    }

    Value number(bool negative) {
        const size_t start = at_;
        if (!(peek() >= '0' && peek() <= '9')) throw Broken{};
        std::uint64_t whole = 0;
        for (;;) {
            const char c = peek();
            if (c >= '0' && c <= '9') {
                whole = whole * 10 + (std::uint64_t) (c - '0');
                ++at_;
                continue;
            }
            if (c == 'e' || c == 'E' || c == '.') {
                size_t stop = at_;
                while (stop < text_.size() && isNumberChar(text_[stop])) ++stop;
                const std::string rest(text_.substr(start, stop - start));
                const char* end = rest.c_str();
                scanDouble(rest.c_str(), &end);
                at_ = start + (size_t) (end - rest.c_str());
                const double d = leadingDouble(std::string(rest.c_str(), end));
                return Value::fromDouble(negative ? -d : d);
            }
            if (c == ' ' || (c >= '\t' && c <= '\r') || c == ',' || c == '}' || c == ']' || c == '\0') break;
            throw Broken{};
        }
        const auto value = (std::int64_t) whole;
        return Value::fromInt(negative ? -value : value);
    }

    static bool isNumberChar(char c) {
        return (c >= '0' && c <= '9') || c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-';
    }

    unsigned hexDigit() {
        const char c = next();
        if (c >= '0' && c <= '9') return (unsigned) (c - '0');
        if (c >= 'a' && c <= 'f') return (unsigned) (c - 'a' + 10);
        if (c >= 'A' && c <= 'F') return (unsigned) (c - 'A' + 10);
        throw Broken{};
    }

    unsigned codeUnit() { return (hexDigit() << 12) | (hexDigit() << 8) | (hexDigit() << 4) | hexDigit(); }

    unsigned escapedCodePoint() {
        const unsigned first = codeUnit();
        if (first < 0xd800 || first > 0xdfff) return first;
        if (first > 0xdbff) throw Broken{};
        if (next() != '\\' || next() != 'u') throw Broken{};
        const unsigned low = codeUnit();
        if (low < 0xdc00 || low > 0xdfff) throw Broken{};
        return 0x10000 + ((first - 0xd800) << 10) + (low - 0xdc00);
    }

    static void appendUtf8(std::string& out, unsigned cp) {
        if (cp < 0x80) {
            out += (char) cp;
        } else if (cp < 0x800) {
            out += (char) (0xc0 | (cp >> 6));
            out += (char) (0x80 | (cp & 0x3f));
        } else if (cp < 0x10000) {
            out += (char) (0xe0 | (cp >> 12));
            out += (char) (0x80 | ((cp >> 6) & 0x3f));
            out += (char) (0x80 | (cp & 0x3f));
        } else {
            out += (char) (0xf0 | (cp >> 18));
            out += (char) (0x80 | ((cp >> 12) & 0x3f));
            out += (char) (0x80 | ((cp >> 6) & 0x3f));
            out += (char) (0x80 | (cp & 0x3f));
        }
    }

    std::string string(char quote) {
        std::string out;
        for (;;) {
            char c = next();
            if (c == quote) return out;
            if (c == '\\') {
                c = next();
                switch (c) {
                    case 'a': c = '\a'; break;
                    case 'b': c = '\b'; break;
                    case 'f': c = '\f'; break;
                    case 'n': c = '\n'; break;
                    case 'r': c = '\r'; break;
                    case 't': c = '\t'; break;
                    case 'u': {
                        const unsigned cp = escapedCodePoint();
                        if (cp == 0) throw Broken{};
                        appendUtf8(out, cp);
                        continue;
                    }
                    default: break;
                }
            }
            if (c == '\0') throw Broken{};
            out += c;
        }
    }

    Value object(int depth) {
        if (depth > kMaxDepth) throw Broken{};
        Value result = Value::fromMembers({});
        for (;;) {
            skipSpace();
            const char c = next();
            if (c == '}') break;
            if (c != '"') throw Broken{};
            auto key = string('"');
            if (key.empty()) throw Broken{};
            skipSpace();
            if (next() != ':') throw Broken{};
            result.set(std::move(key), any(depth));
            skipSpace();
            if (take(',')) continue;
            if (take('}')) break;
            throw Broken{};
        }
        return result;
    }

    Value array(int depth) {
        if (depth > kMaxDepth) throw Broken{};
        Value result = Value::fromItems({});
        for (;;) {
            skipSpace();
            if (take(']')) break;
            if (atEnd()) throw Broken{};
            result.append(any(depth));
            skipSpace();
            if (take(',')) continue;
            if (take(']')) break;
            throw Broken{};
        }
        return result;
    }

    std::string_view text_;
    size_t at_ = 0;
};

}

Value parse(std::string_view text) {
    try {
        return Parser(text).document();
    } catch (const Broken&) {
        return {};
    }
}

bool readTextFile(const std::string& path, std::string& out) {
    auto in = openForReading(path);
    if (!in) return false;
    std::ostringstream buffer;
    buffer << in.rdbuf();
    out = buffer.str();
    if (out.rfind("\xef\xbb\xbf", 0) == 0) out.erase(0, 3); // utf8-ok
    return true;
}

Value parseFile(const std::string& path) {
    std::string text;
    return readTextFile(path, text) ? parse(text) : Value{};
}

}
