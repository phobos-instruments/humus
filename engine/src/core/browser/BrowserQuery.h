// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include "core/browser/BrowserEntry.h"

namespace hum::browser {

struct Token {
    enum class Type { Word, Tag, MinRating, Kind, Bpm, Key, Favourite };
    Type type = Type::Word;
    std::string text;
    int rating = 0;
    Kind kind = Kind::Other;
    double bpmLow = 0.0, bpmHigh = 0.0;
    bool locked = false;

    std::string chip() const;
};

struct Query {
    std::vector<Token> tokens;
    std::string pending;

    bool empty() const { return tokens.empty() && pending.empty(); }
    bool matches(const Entry& e) const;
    void lockKind(Kind kind);
    void lockKinds(const std::vector<Kind>& kinds);
};

Token tokenOf(const std::string& word);
Query parseQuery(const std::string& text);
std::vector<const Entry*> filter(const std::vector<const Entry*>& from, const Query& q);

}
