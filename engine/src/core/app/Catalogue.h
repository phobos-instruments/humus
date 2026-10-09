// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/app/PluralRule.h"

namespace hum {

class Catalogue {
public:
    static Catalogue fromPo(const std::string& text);

    void add(const Catalogue& more);

    const std::string* text(const std::string& key) const;
    const std::string* plural(const std::string& key, long long n) const;

    const std::string& languageName() const { return name_; }
    const PluralRule& rule() const { return rule_; }
    size_t size() const { return entries_.size(); }

private:
    struct Entry {
        std::vector<std::string> forms;
        bool plural = false;
    };

    std::unordered_map<std::string, Entry> entries_;
    PluralRule rule_;
    std::string name_;
};

std::string fillPlaceholders(const std::string& text,
                             const std::vector<std::pair<std::string, std::string>>& values);

}
