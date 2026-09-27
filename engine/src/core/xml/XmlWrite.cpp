// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/xml/Xml.h"

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <locale>
#include <sstream>
#include <system_error>

#include "hum/FileBytes.h"

namespace hum::xml {

namespace {

bool isLegal(std::uint32_t c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' || c == '.'
           || c == ',' || c == ';' || c == ':' || c == '-' || c == '(' || c == ')' || c == '_' || c == '+' || c == '='
           || c == '?' || c == '!' || c == '$' || c == '#' || c == '@' || c == '[' || c == ']' || c == '/' || c == '|'
           || c == '*' || c == '%' || c == '~' || c == '{' || c == '}' || c == '\'' || c == '\\';
}

std::uint32_t nextCodePoint(const std::string& s, size_t& i) {
    const auto lead = (std::uint32_t) (unsigned char) s[i++];
    if (lead < 0x80) return lead;
    std::uint32_t mask = 0x7f, bit = 0x40;
    int extra = 0;
    while ((lead & bit) != 0 && bit > 0x8) {
        mask >>= 1;
        ++extra;
        bit >>= 1;
    }
    std::uint32_t n = lead & mask;
    for (; extra > 0 && i < s.size(); --extra) {
        const auto next = (std::uint32_t) (unsigned char) s[i];
        if ((next & 0xc0) != 0x80) break;
        ++i;
        n = (n << 6) | (next & 0x3f);
    }
    return n;
}

void escape(std::string& out, const std::string& text, bool changeNewLines) {
    for (size_t i = 0; i < text.size();) {
        const auto c = nextCodePoint(text, i);
        if (c == 0) break;
        if (isLegal(c)) {
            out += (char) c;
            continue;
        }
        switch (c) {
            case '&': out += "&amp;"; break;
            case '"': out += "&quot;"; break;
            case '>': out += "&gt;"; break;
            case '<': out += "&lt;"; break;
            case '\n':
            case '\r':
                if (!changeNewLines) {
                    out += (char) c;
                    break;
                }
                [[fallthrough]];
            default: out += "&#" + std::to_string((int) c) + ";"; break;
        }
    }
}

void writeElement(std::string& out, const Element& e, int indent, int lineWrap, const char* newLine) {
    if (indent >= 0) out.append((size_t) indent, ' ');
    if (e.isText()) {
        escape(out, e.text(), false);
        return;
    }
    out += '<';
    out += e.tag();
    const auto attIndent = (size_t) (indent + (int) e.tag().size() + 1);
    int lineLen = 0;
    for (int a = 0; a < e.numAttributes(); ++a) {
        if (lineLen > lineWrap && indent >= 0) {
            out += newLine;
            out.append(attIndent, ' ');
            lineLen = 0;
        }
        const auto start = out.size();
        out += ' ';
        out += e.attributeName(a);
        out += "=\"";
        escape(out, e.attributeValue(a), true);
        out += '"';
        lineLen += (int) (out.size() - start);
    }
    if (e.numChildren() == 0) {
        out += "/>";
        return;
    }
    out += '>';
    bool lastWasText = false;
    for (const auto* child : e.children()) {
        if (child->isText()) {
            escape(out, child->text(), false);
            lastWasText = true;
            continue;
        }
        if (indent >= 0 && !lastWasText) out += newLine;
        writeElement(out, *child, lastWasText ? 0 : indent + (indent >= 0 ? 2 : 0), lineWrap, newLine);
        lastWasText = false;
    }
    if (indent >= 0 && !lastWasText) {
        out += newLine;
        out.append((size_t) indent, ' ');
    }
    out += "</";
    out += e.tag();
    out += '>';
}

std::string streamed(double value, int decimals, bool scientific) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out.setf(scientific ? std::ios_base::scientific : std::ios_base::fixed);
    out.precision((std::streamsize) decimals);
    out << value;
    return out.str();
}

std::string shortened(const std::string& in) {
    const int end = (int) in.size();
    int trimStart = end, trimEnd = end, expStart = end, expEnd = end;
    char current = 0;
    for (int c = end - 1; c > 0; --c) {
        current = in[(size_t) c];
        if (current == '0' && c + 1 == trimStart) {
            --trimStart;
        } else if (current == '.') {
            if (trimStart == c + 1 && trimStart != end && in[(size_t) trimStart] == '0') ++trimStart;
            break;
        } else if (current == 'e' || current == 'E') {
            int next = c + 1;
            if (next != end) {
                if (in[(size_t) next] == '-') ++next;
                expStart = next;
                if (next != end && in[(size_t) next] == '+') ++next;
                expEnd = next;
            }
            while (next != end && in[(size_t) next++] == '0') expEnd = next;
            if (expEnd == end) expStart = c;
            trimStart = c;
            trimEnd = trimStart;
        }
    }
    const auto part = [&in](int from, int to) { return in.substr((size_t) from, (size_t) (to - from)); };
    if ((trimStart != trimEnd && current == '.') || expStart != expEnd) {
        if (trimStart == trimEnd) return part(0, expStart) + part(expEnd, end);
        if (expStart == expEnd) return part(0, trimStart) + part(trimEnd, end);
        if (trimEnd == expStart) return part(0, trimStart) + part(expEnd, end);
        return part(0, trimStart) + part(trimEnd, expStart) + part(expEnd, end);
    }
    return in;
}

}

std::string numberText(double value) {
    const double a = std::abs(value);
    if (a >= 1.0e6 || a <= 1.0e-5) return shortened(streamed(value, 15, true));
    if ((double) (int) value == value) return streamed(value, 1, false);
    const int decimals = a < 1.0 ? (a >= 1.0e-3 ? (a >= 1.0e-1 ? 16 : a >= 1.0e-2 ? 17 : 18) : a >= 1.0e-4 ? 19 : 20)
                         : a < 1.0e3 ? (a < 1.0e1 ? 15 : a < 1.0e2 ? 14 : 13)
                         : a < 1.0e4 ? 12
                         : a < 1.0e5 ? 11
                                     : 10;
    return shortened(streamed(value, decimals, false));
}

std::string write(const Element& root, const Format& format) {
    std::string out;
    if (format.header) {
        out += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
        if (format.newLine == nullptr) out += ' ';
        else out += std::string(format.newLine) + format.newLine;
    }
    writeElement(out, root, format.newLine == nullptr ? -1 : 0, format.lineWrap, format.newLine);
    if (format.newLine != nullptr) out += format.newLine;
    return out;
}

bool writeFile(const std::string& path, const Element& root, const Format& format) {
    const auto text = write(root, format);
    const auto target = utf8Path(path);
    auto staging = target;
    staging += ".tmp";
    {
        auto out = openForWriting(utf8Text(staging));
        if (!out.write(text.data(), (std::streamsize) text.size())) return false;
    }
    std::error_code ec;
    std::filesystem::rename(staging, target, ec);
    if (ec) std::filesystem::remove(staging, ec);
    return !ec;
}

}
