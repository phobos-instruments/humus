// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace hum {

inline bool isNameSeparator(char c) { return std::isspace((unsigned char) c) || c == '-' || c == '_'; }

inline int fuzzyScore(const std::string& query, const std::string& text) {
    std::string q;
    q.reserve(query.size());
    for (const char c : query)
        if (!isNameSeparator(c)) q += (char) std::tolower((unsigned char) c);
    if (q.empty()) return 0;
    std::string t;
    std::vector<bool> wordStart;
    t.reserve(text.size());
    char prev = ' ';
    for (const char c : text) {
        if (isNameSeparator(c)) {
            prev = ' ';
            continue;
        }
        const auto u = (unsigned char) c;
        const auto p = (unsigned char) prev;
        wordStart.push_back(prev == ' ' || (std::isupper(u) && !std::isupper(p))
                            || (std::isdigit(u) && !std::isdigit(p)));
        t += (char) std::tolower(u);
        prev = c;
    }
    const int lenPenalty = (int) text.size();

    if (t.rfind(q, 0) == 0) return 1000 - lenPenalty;

    {
        size_t qi = 0;
        for (size_t i = 0; i < t.size() && qi < q.size(); ++i)
            if (wordStart[i] && t[i] == q[qi]) ++qi;
        if (qi == q.size()) return 850 - lenPenalty;
    }

    if (const auto at = t.find(q); at != std::string::npos)
        return 700 - (int) at * 3 - lenPenalty;

    {
        size_t qi = 0, first = std::string::npos, last = 0;
        for (size_t i = 0; i < t.size() && qi < q.size(); ++i)
            if (t[i] == q[qi]) {
                if (first == std::string::npos) first = i;
                last = i;
                ++qi;
            }
        if (qi == q.size()) return 450 - (int) (last - first) - lenPenalty;
    }
    return -1;
}

inline constexpr size_t kMinTextQuery = 3;

inline int wordsScore(const std::string& query, const std::string& text) {
    auto lower = [](const std::string& s) {
        std::string out;
        out.reserve(s.size());
        for (const char c : s) out += (char) std::tolower((unsigned char) c);
        return out;
    };
    const std::string q = lower(query), t = lower(text);
    size_t first = std::string::npos, words = 0, letters = 0;
    for (size_t i = 0; i < q.size();) {
        while (i < q.size() && std::isspace((unsigned char) q[i])) ++i;
        size_t j = i;
        while (j < q.size() && !std::isspace((unsigned char) q[j])) ++j;
        if (j == i) break;
        const std::string word = q.substr(i, j - i);
        size_t at = t.find(word);
        while (at != std::string::npos && at > 0 && std::isalnum((unsigned char) t[at - 1]))
            at = t.find(word, at + 1);
        if (at == std::string::npos) return -1;
        first = std::min(first, at);
        ++words;
        letters += word.size();
        i = j;
    }
    if (words == 0 || letters < kMinTextQuery) return -1;
    return 90 - (int) std::min<size_t>(first / 10, 40);
}

inline int fuzzyScoreNameThenMeta(const std::string& query, const std::string& name,
                                  const std::string& meta) {
    const int n = fuzzyScore(query, name);
    const int m = fuzzyScore(query, meta);
    return std::max(n, m >= 0 ? 100 + m / 10 : -1);
}

inline int fuzzyScoreNameMetaText(const std::string& query, const std::string& name,
                                  const std::string& meta, const std::string& text) {
    return std::max(fuzzyScoreNameThenMeta(query, name, meta), wordsScore(query, text));
}

}
