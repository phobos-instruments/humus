// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/xml/Xml.h"

#include <cstdint>
#include <cstring>
#include <vector>

#include "hum/FileBytes.h"

namespace hum::xml {

namespace {

bool isSpace(char c) { return c == ' ' || (c >= 9 && c <= 13); }

bool isIdentifierChar(char c) {
    const auto u = (unsigned char) c;
    return (u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z') || (u >= '0' && u <= '9') || u == '_' || u == '-'
           || u == ':' || u == '.' || u >= 0x80;
}

void appendUtf8(std::string& out, std::uint32_t c) {
    if (c < 0x80) {
        out += (char) c;
    } else if (c < 0x800) {
        out += (char) (0xc0 | (c >> 6));
        out += (char) (0x80 | (c & 0x3f));
    } else if (c < 0x10000) {
        out += (char) (0xe0 | (c >> 12));
        out += (char) (0x80 | ((c >> 6) & 0x3f));
        out += (char) (0x80 | (c & 0x3f));
    } else {
        out += (char) (0xf0 | (c >> 18));
        out += (char) (0x80 | ((c >> 12) & 0x3f));
        out += (char) (0x80 | ((c >> 6) & 0x3f));
        out += (char) (0x80 | (c & 0x3f));
    }
}

int hexDigit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

class Parser {
public:
    explicit Parser(const std::string& text) : s_(text) {}

    std::unique_ptr<Element> document() {
        if (s_.empty()) return nullptr;
        if (!header() || !dtd()) return nullptr;
        auto root = element();
        return failed_ ? nullptr : std::move(root);
    }

private:
    char at(size_t i) const { return i < s_.size() ? s_[i] : 0; }
    bool startsWith(size_t i, const char* word) const { return s_.compare(i, std::strlen(word), word) == 0; }
    bool startsWithIgnoringCase(size_t i, const char* word) const {
        for (size_t k = 0; word[k] != 0; ++k)
            if (std::tolower((unsigned char) at(i + k)) != word[k]) return false;
        return true;
    }

    void fail() { failed_ = true; }

    void skipSpace() {
        for (;;) {
            while (pos_ < s_.size() && isSpace(s_[pos_])) ++pos_;
            if (pos_ >= s_.size()) {
                outOfData_ = true;
                return;
            }
            if (s_[pos_] == '<' && at(pos_ + 1) == '!' && at(pos_ + 2) == '-' && at(pos_ + 3) == '-') {
                const auto close = s_.find("-->", pos_ + 4);
                if (close == std::string::npos) {
                    outOfData_ = true;
                    return;
                }
                pos_ = close + 3;
                continue;
            }
            if (s_[pos_] == '<' && at(pos_ + 1) == '?') {
                const auto close = s_.find("?>", pos_ + 2);
                if (close == std::string::npos) {
                    outOfData_ = true;
                    return;
                }
                pos_ = close + 2;
                continue;
            }
            return;
        }
    }

    bool header() {
        skipSpace();
        if (startsWith(pos_, "<?xml")) {
            const auto end = s_.find("?>", pos_);
            if (end == std::string::npos) return false;
            pos_ = end + 2;
            skipSpace();
        }
        return true;
    }

    bool dtd() {
        if (!startsWith(pos_, "<!DOCTYPE")) return true;
        pos_ += 9;
        for (int depth = 1; depth > 0; ++pos_) {
            if (pos_ >= s_.size()) return false;
            if (s_[pos_] == '<') ++depth;
            else if (s_[pos_] == '>') --depth;
        }
        return true;
    }

    size_t tokenEnd(size_t i) const {
        while (i < s_.size() && isIdentifierChar(s_[i])) ++i;
        return i;
    }

    void entity(std::string& out) {
        ++pos_;
        for (const auto& [word, ch] : {std::pair<const char*, char>{"amp;", '&'}, {"quot;", '"'}, {"apos;", '\''},
                                       {"lt;", '<'}, {"gt;", '>'}})
            if (startsWithIgnoringCase(pos_, word)) {
                pos_ += std::strlen(word);
                out += ch;
                return;
            }
        if (at(pos_) == '#') {
            std::int64_t code = 0;
            ++pos_;
            if (at(pos_) == 'x' || at(pos_) == 'X') {
                ++pos_;
                for (int n = 0; at(pos_) != ';'; ++pos_) {
                    const int h = hexDigit(at(pos_));
                    if (h < 0 || ++n > 8) break;
                    code = (code << 4) | h;
                }
                ++pos_;
            } else if (at(pos_) >= '0' && at(pos_) <= '9') {
                for (int n = 0;; ++pos_) {
                    const char c = at(pos_);
                    if (c == 0) return;
                    if (c == ';' || ++n > 12) break;
                    code = code * 10 + (c - '0');
                }
                ++pos_;
            } else {
                out += '&';
                return;
            }
            appendUtf8(out, (std::uint32_t) code);
            return;
        }
        const auto semi = s_.find(';', pos_);
        if (semi == std::string::npos) {
            outOfData_ = true;
            out += '&';
            return;
        }
        out += s_.substr(pos_, semi - pos_);
        pos_ = semi + 1;
    }

    void quoted(std::string& out) {
        const char quote = s_[pos_++];
        for (;;) {
            const char c = at(pos_);
            if (c == quote) {
                ++pos_;
                return;
            }
            if (c == 0) {
                fail();
                outOfData_ = true;
                return;
            }
            if (c == '&') entity(out);
            else out += s_[pos_++];
        }
    }

    std::unique_ptr<Element> element() {
        skipSpace();
        if (outOfData_ || s_[pos_] != '<') return nullptr;
        ++pos_;
        auto end = tokenEnd(pos_);
        if (end == pos_) {
            skipSpace();
            end = tokenEnd(pos_);
            if (end == pos_) {
                fail();
                return nullptr;
            }
        }
        auto node = std::make_unique<Element>(s_.substr(pos_, end - pos_));
        pos_ = end;
        for (;;) {
            skipSpace();
            const char c = at(pos_);
            if (c == '/' && at(pos_ + 1) == '>') {
                pos_ += 2;
                break;
            }
            if (c == '>') {
                ++pos_;
                children(*node);
                break;
            }
            if (isIdentifierChar(c)) {
                const auto nameEnd = tokenEnd(pos_);
                const auto name = s_.substr(pos_, nameEnd - pos_);
                pos_ = nameEnd;
                skipSpace();
                if (at(pos_) == '=') {
                    ++pos_;
                    skipSpace();
                    if (at(pos_) == '"' || at(pos_) == '\'') {
                        std::string value;
                        quoted(value);
                        node->setAttribute(name, value);
                        continue;
                    }
                } else {
                    fail();
                    return node;
                }
            } else if (!outOfData_) {
                fail();
            }
            break;
        }
        return node;
    }

    void children(Element& parent) {
        for (;;) {
            const auto before = pos_;
            skipSpace();
            if (outOfData_) {
                fail();
                return;
            }
            if (s_[pos_] == '<') {
                if (at(pos_ + 1) == '/') {
                    const auto close = s_.find('>', pos_);
                    if (close != std::string::npos) pos_ = close + 1;
                    return;
                }
                if (at(pos_ + 1) == '!' && startsWith(pos_ + 2, "[CDATA[")) {
                    const auto close = s_.find("]]>", pos_ + 9);
                    if (close == std::string::npos) {
                        fail();
                        outOfData_ = true;
                        return;
                    }
                    parent.addChild(Element::textNode(s_.substr(pos_ + 9, close - pos_ - 9)));
                    pos_ = close + 3;
                    continue;
                }
                auto child = element();
                if (child == nullptr) return;
                parent.addChild(std::move(child));
                continue;
            }
            pos_ = before;
            if (!text(parent)) return;
        }
    }

    bool text(Element& parent) {
        std::string content;
        bool used = false;
        for (;;) {
            const char c = at(pos_);
            if (c == '<') {
                if (at(pos_ + 1) == '!' && at(pos_ + 2) == '-' && at(pos_ + 3) == '-') {
                    const auto close = s_.find("-->", pos_ + 4);
                    if (close == std::string::npos) {
                        fail();
                        outOfData_ = true;
                        return false;
                    }
                    pos_ = close + 3;
                    continue;
                }
                break;
            }
            if (c == 0) {
                fail();
                outOfData_ = true;
                return false;
            }
            if (c == '&') {
                std::string decoded;
                entity(decoded);
                content += decoded;
                for (char d : decoded) used = used || !isSpace(d);
                continue;
            }
            for (;; ++pos_) {
                char next = at(pos_);
                if (next == '\r') {
                    next = '\n';
                    if (at(pos_ + 1) == '\n') continue;
                }
                if (next == '<' || next == '&') break;
                if (next == 0) {
                    fail();
                    outOfData_ = true;
                    return false;
                }
                content += next;
                used = used || !isSpace(next);
            }
        }
        if (used) parent.addChild(Element::textNode(content));
        return true;
    }

    const std::string& s_;
    size_t pos_ = 0;
    bool failed_ = false;
    bool outOfData_ = false;
};

}

std::unique_ptr<Element> parse(const std::string& text) { return Parser(text).document(); }

std::unique_ptr<Element> parseFile(const std::string& path) {
    std::vector<std::uint8_t> bytes;
    if (!readFileBytes(path, bytes)) return nullptr;
    const bool bom = bytes.size() >= 3 && bytes[0] == 0xef && bytes[1] == 0xbb && bytes[2] == 0xbf;
    return parse(std::string(bytes.begin() + (bom ? 3 : 0), bytes.end()));
}

}
