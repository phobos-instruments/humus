// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/browser/NameFacts.h"

#include <cctype>
#include <cstdlib>
#include <vector>

namespace hum::browser {

namespace {

constexpr double kSlowestBpm = 40.0, kFastestBpm = 300.0;

std::vector<std::string> wordsOf(const std::string& stem) {
    std::vector<std::string> out;
    std::string w;
    for (const char c : stem) {
        if (std::isalnum((unsigned char) c) || c == '#') { w += c; continue; }
        if (!w.empty()) out.push_back(w);
        w.clear();
    }
    if (!w.empty()) out.push_back(w);
    return out;
}

std::string lower(std::string s) {
    for (auto& c : s) c = (char) std::tolower((unsigned char) c);
    return s;
}

bool digitsOnly(const std::string& s) {
    if (s.empty()) return false;
    for (const char c : s)
        if (!std::isdigit((unsigned char) c)) return false;
    return true;
}

double tempoOf(const std::string& digits) {
    const double v = std::atof(digits.c_str());
    return v >= kSlowestBpm && v <= kFastestBpm ? v : 0.0;
}

double bpmOf(const std::vector<std::string>& words) {
    for (size_t i = 0; i < words.size(); ++i) {
        const auto w = lower(words[i]);
        if (w.size() > 3 && w.compare(w.size() - 3, 3, "bpm") == 0 && digitsOnly(w.substr(0, w.size() - 3)))
            return tempoOf(w.substr(0, w.size() - 3));
        if (w == "bpm" && i > 0 && digitsOnly(words[i - 1])) return tempoOf(words[i - 1]);
    }
    return 0.0;
}

std::string keyOf(const std::string& word) {
    if (word.empty() || word.size() > 5) return {};
    const char root = (char) std::toupper((unsigned char) word[0]);
    if (root < 'A' || root > 'G') return {};
    std::string key(1, root);
    size_t i = 1;
    if (i < word.size() && (word[i] == '#' || word[i] == 'b')) key += word[i++];
    const auto rest = word.substr(i);
    if (rest.empty() && word.size() == 1) return {};
    if (rest.empty() || rest == "maj" || rest == "major") return key;
    if (rest == "m" || rest == "min" || rest == "minor") return key + "m";
    return {};
}

}

NamedFacts factsFromName(const std::string& fileName) {
    NamedFacts out;
    const auto dot = fileName.find_last_of('.');
    const auto words = wordsOf(dot == std::string::npos ? fileName : fileName.substr(0, dot));
    out.bpm = bpmOf(words);
    for (auto it = words.rbegin(); it != words.rend() && out.key.empty(); ++it)
        if (it->size() >= 2) out.key = keyOf(*it);
    return out;
}

}
