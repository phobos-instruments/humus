// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/app/Catalogue.h"

#include "core/app/PoFile.h"

namespace hum {

namespace {

bool complete(const PoEntry& e, int forms) {
    if (e.fuzzy || e.forms.empty()) return false;
    const size_t wanted = e.isPlural() ? (size_t) forms : 1;
    if (e.forms.size() < wanted) return false;
    for (size_t i = 0; i < wanted; ++i)
        if (e.forms[i].empty()) return false;
    return true;
}

std::string field(const std::map<std::string, std::string>& header, const char* name) {
    const auto it = header.find(name);
    return it == header.end() ? std::string() : it->second;
}

}

Catalogue Catalogue::fromPo(const std::string& text) {
    const auto po = parsePo(text);
    Catalogue out;
    out.rule_ = PluralRule::fromHeader(field(po.header, "Plural-Forms"));
    out.name_ = field(po.header, "X-Language-Name");
    for (const auto& e : po.entries) {
        if (!complete(e, out.rule_.forms())) continue;
        Entry kept;
        kept.plural = e.isPlural();
        kept.forms.assign(e.forms.begin(), e.forms.begin() + (kept.plural ? out.rule_.forms() : 1));
        out.entries_.emplace(e.key(), std::move(kept));
    }
    return out;
}

void Catalogue::add(const Catalogue& more) {
    for (const auto& [key, entry] : more.entries_) entries_.emplace(key, entry);
    if (!rule_.parsed() && more.rule_.parsed()) rule_ = more.rule_;
    if (name_.empty()) name_ = more.name_;
}

const std::string* Catalogue::text(const std::string& key) const {
    const auto it = entries_.find(key);
    if (it == entries_.end() || it->second.plural) return nullptr;
    return &it->second.forms[0];
}

const std::string* Catalogue::plural(const std::string& key, long long n) const {
    const auto it = entries_.find(key);
    if (it == entries_.end() || !it->second.plural) return nullptr;
    const auto form = (size_t) rule_.formFor(n);
    return form < it->second.forms.size() ? &it->second.forms[form] : nullptr;
}

std::string fillPlaceholders(const std::string& text,
                             const std::vector<std::pair<std::string, std::string>>& values) {
    std::string out;
    out.reserve(text.size());
    size_t pos = 0;
    while (pos < text.size()) {
        const auto open = text.find('{', pos);
        if (open == std::string::npos) break;
        const auto close = text.find('}', open + 1);
        if (close == std::string::npos) break;
        out.append(text, pos, open - pos);
        const auto name = text.substr(open + 1, close - open - 1);
        const std::string* value = nullptr;
        for (const auto& [k, v] : values)
            if (k == name) { value = &v; break; }
        if (value != nullptr) out += *value;
        else out.append(text, open, close - open + 1);
        pos = close + 1;
    }
    out.append(text, pos, std::string::npos);
    return out;
}

}
