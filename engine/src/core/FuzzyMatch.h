#pragma once
#include <algorithm>
#include <cctype>
#include <string>

namespace hum {

inline int fuzzyScore(const std::string& query, const std::string& text) {
    if (query.empty()) return 0;
    std::string q, t;
    q.reserve(query.size());
    t.reserve(text.size());
    for (const char c : query) q += (char) std::tolower((unsigned char) c);
    for (const char c : text) t += (char) std::tolower((unsigned char) c);
    const int lenPenalty = (int) text.size();

    if (t.rfind(q, 0) == 0) return 1000 - lenPenalty;

    {
        size_t qi = 0;
        for (size_t i = 0; i < text.size() && qi < q.size(); ++i) {
            const bool hump = i == 0
                || (std::isupper((unsigned char) text[i])
                    && !std::isupper((unsigned char) text[i - 1]))
                || (std::isdigit((unsigned char) text[i])
                    && !std::isdigit((unsigned char) text[i - 1]));
            if (hump && t[i] == q[qi]) ++qi;
        }
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

inline int fuzzyScoreNameThenMeta(const std::string& query, const std::string& name,
                                  const std::string& meta) {
    const int n = fuzzyScore(query, name);
    const int m = fuzzyScore(query, meta);
    return std::max(n, m >= 0 ? 100 + m / 10 : -1);
}

}
