// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/browser/BrowserQuery.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>

#include "core/browser/FileIndex.h"

namespace hum::browser {

namespace {

constexpr double kBpmSlack = 0.5;

std::string lower(std::string s) {
    for (auto& c : s) c = (char) std::tolower((unsigned char) c);
    return s;
}

bool startsWith(const std::string& s, const char* head) { return s.rfind(head, 0) == 0; }

bool looksLikeToken(const std::string& w) {
    for (const char* head : {"#", ">", "kind:", "bpm:", "key:", "stars:"})
        if (startsWith(w, head)) return true;
    return false;
}

bool wholeNumber(const std::string& s, int& out) {
    if (s.empty() || !std::all_of(s.begin(), s.end(), [](char c) { return std::isdigit((unsigned char) c) != 0; }))
        return false;
    out = std::atoi(s.c_str());
    return true;
}

bool bpmRange(const std::string& s, double& lo, double& hi) {
    const auto dash = s.find('-');
    char* end = nullptr;
    lo = std::strtod(s.c_str(), &end);
    if (end == s.c_str() || lo <= 0.0) return false;
    hi = lo;
    if (dash == std::string::npos) return (size_t) (end - s.c_str()) == s.size();
    const auto upper = s.substr(dash + 1);
    hi = std::strtod(upper.c_str(), &end);
    return end != upper.c_str() && hi >= lo;
}

bool ratingToken(const std::string& w, int& stars) {
    if (startsWith(w, ">=")) return wholeNumber(w.substr(2), stars);
    if (startsWith(w, ">")) {
        if (!wholeNumber(w.substr(1), stars)) return false;
        stars += 1;
        return true;
    }
    if (w.size() >= 2 && w.back() == '+') return wholeNumber(w.substr(0, w.size() - 1), stars);
    if (startsWith(w, "stars:")) return wholeNumber(w.substr(6), stars);
    return false;
}

bool wordMatches(const Entry& e, const std::string& word) {
    if (lower(fileName(e.path)).find(word) != std::string::npos) return true;
    if (lower(e.path).find(word) != std::string::npos) return true;
    return std::any_of(e.tags.begin(), e.tags.end(), [&](const std::string& t) { return t == word; });
}

}

std::string Token::chip() const {
    switch (type) {
        case Type::Tag: return "#" + text;
        case Type::MinRating: return ">=" + std::to_string(rating);
        case Type::Kind: return std::string("kind:") + kindWord(kind);
        case Type::Bpm: return text;
        case Type::Key: return "key:" + text;
        case Type::Favourite: return "fav";
        case Type::Word: break;
    }
    return text;
}

Token tokenOf(const std::string& raw) {
    Token t;
    const auto w = lower(raw);
    t.text = w;
    int stars = 0;
    if (w.size() > 1 && w[0] == '#') {
        t.type = Token::Type::Tag;
        t.text = normalTag(w.substr(1));
    } else if (ratingToken(w, stars)) {
        t.type = Token::Type::MinRating;
        t.rating = std::clamp(stars, 0, kMaxRating);
    } else if (startsWith(w, "kind:") && parseKindWord(w.substr(5), t.kind)) {
        t.type = Token::Type::Kind;
    } else if (startsWith(w, "bpm:") && bpmRange(w.substr(4), t.bpmLow, t.bpmHigh)) {
        t.type = Token::Type::Bpm;
    } else if (startsWith(w, "key:") && w.size() > 4) {
        t.type = Token::Type::Key;
        t.text = w.substr(4);
    } else if (w == "fav" || w == "favourite" || w == "favorite") {
        t.type = Token::Type::Favourite;
    }
    return t;
}

Query parseQuery(const std::string& text) {
    Query q;
    std::string word;
    for (const char c : text) {
        if (!std::isspace((unsigned char) c)) { word += c; continue; }
        if (!word.empty()) q.tokens.push_back(tokenOf(word));
        word.clear();
    }
    q.pending = lower(word);
    return q;
}

void Query::lockKind(Kind kind) { lockKinds({kind}); }

void Query::lockKinds(const std::vector<Kind>& kinds) {
    tokens.erase(std::remove_if(tokens.begin(), tokens.end(),
                                [](const Token& t) { return t.type == Token::Type::Kind; }),
                 tokens.end());
    for (auto it = kinds.rbegin(); it != kinds.rend(); ++it) {
        Token t;
        t.type = Token::Type::Kind;
        t.kind = *it;
        t.locked = true;
        tokens.insert(tokens.begin(), t);
    }
}

bool Query::matches(const Entry& e) const {
    auto all = tokens;
    if (!pending.empty()) {
        const auto typed = tokenOf(pending);
        if (typed.type != Token::Type::Word || !looksLikeToken(pending)) all.push_back(typed);
    }
    if (e.kind == Kind::Folder) {
        for (const auto& t : all)
            if (t.type == Token::Type::Word && !wordMatches(e, t.text)) return false;
        return true;
    }
    const bool kindLocked = std::any_of(all.begin(), all.end(), [](const Token& t) { return t.locked; });
    bool kindAsked = false, kindHit = false;
    for (const auto& t : all) {
        switch (t.type) {
            case Token::Type::Word:
                if (!wordMatches(e, t.text)) return false;
                break;
            case Token::Type::Tag:
                if (std::find(e.tags.begin(), e.tags.end(), t.text) == e.tags.end()) return false;
                break;
            case Token::Type::MinRating:
                if (e.rating < t.rating) return false;
                break;
            case Token::Type::Kind:
                if (kindLocked && !t.locked) break;
                kindAsked = true;
                kindHit = kindHit || e.kind == t.kind;
                break;
            case Token::Type::Bpm:
                if (e.facts.bpm < t.bpmLow - kBpmSlack || e.facts.bpm > t.bpmHigh + kBpmSlack) return false;
                break;
            case Token::Type::Key:
                if (lower(e.facts.key) != t.text) return false;
                break;
            case Token::Type::Favourite:
                if (!e.favourite) return false;
                break;
        }
    }
    return !kindAsked || kindHit;
}

std::vector<const Entry*> filter(const std::vector<const Entry*>& from, const Query& q) {
    if (q.empty()) return from;
    std::vector<const Entry*> out;
    for (const auto* e : from)
        if (e != nullptr && q.matches(*e)) out.push_back(e);
    return out;
}

}
