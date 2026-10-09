// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/app/PoFile.h"

#include <cstdlib>
#include <sstream>

namespace hum {

namespace {

std::string unquoted(const std::string& line) {
    const auto open = line.find('"');
    const auto close = line.rfind('"');
    if (open == std::string::npos || close <= open) return {};
    std::string out;
    for (size_t i = open + 1; i < close; ++i) {
        const char c = line[i];
        if (c != '\\' || i + 1 >= close) { out += c; continue; }
        const char e = line[++i];
        switch (e) {
            case 'n': out += '\n'; break;
            case 't': out += '\t'; break;
            case 'r': out += '\r'; break;
            default: out += e; break;
        }
    }
    return out;
}

bool startsWith(const std::string& s, const char* prefix) {
    return s.rfind(prefix, 0) == 0;
}

std::string trimmed(const std::string& s) {
    const auto from = s.find_first_not_of(" \t\r");
    if (from == std::string::npos) return {};
    return s.substr(from, s.find_last_not_of(" \t\r") - from + 1);
}

std::map<std::string, std::string> headerFields(const std::string& text) {
    std::map<std::string, std::string> out;
    std::istringstream lines(text);
    for (std::string line; std::getline(lines, line);) {
        const auto colon = line.find(':');
        if (colon != std::string::npos) out[trimmed(line.substr(0, colon))] = trimmed(line.substr(colon + 1));
    }
    return out;
}

class Reader {
public:
    static constexpr size_t kMaxForms = 6;

    PoFile read(const std::string& text) {
        std::istringstream lines(text);
        for (std::string raw; std::getline(lines, raw);) line(trimmed(raw));
        flush();
        return std::move(out_);
    }

private:
    void line(const std::string& l) {
        if (l.empty()) { flush(); return; }
        if (startsWith(l, "#~")) return;
        if (l[0] == '#') {
            if (!current_.forms.empty()) flush();
            if (startsWith(l, "#,") && l.find("fuzzy") != std::string::npos) current_.fuzzy = true;
            return;
        }
        if (startsWith(l, "msgctxt")) {
            if (started_) flush();
            open(&current_.context, l);
            return;
        }
        if (startsWith(l, "msgid_plural")) { open(&current_.idPlural, l); return; }
        if (startsWith(l, "msgid")) {
            if (hasId_) flush();
            hasId_ = true;
            open(&current_.id, l);
            return;
        }
        if (startsWith(l, "msgstr[")) {
            const size_t index = (size_t) std::atoi(l.c_str() + 7);
            if (index >= kMaxForms) { target_ = nullptr; return; }
            if (current_.forms.size() <= index) current_.forms.resize(index + 1);
            open(&current_.forms[index], l);
            return;
        }
        if (startsWith(l, "msgstr")) {
            current_.forms.assign(1, std::string());
            open(&current_.forms[0], l);
            return;
        }
        if (l[0] == '"' && target_ != nullptr) *target_ += unquoted(l);
    }

    void open(std::string* field, const std::string& l) {
        started_ = true;
        target_ = field;
        *target_ = unquoted(l);
    }

    void flush() {
        if (started_) {
            if (current_.id.empty() && current_.context.empty())
                out_.header = headerFields(current_.forms.empty() ? std::string() : current_.forms[0]);
            else
                out_.entries.push_back(std::move(current_));
        }
        current_ = PoEntry();
        target_ = nullptr;
        started_ = false;
        hasId_ = false;
    }

    PoFile out_;
    PoEntry current_;
    std::string* target_ = nullptr;
    bool started_ = false;
    bool hasId_ = false;
};

}

PoFile parsePo(const std::string& text) {
    return Reader().read(text);
}

}
