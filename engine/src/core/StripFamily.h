#pragma once
#include <cctype>
#include <string>

#include "hum/Registry.h"

namespace hum::strips {

struct Name {
    std::string prefix, family;
    int count = 0;
};

inline Name parse(const std::string& cls) {
    size_t i = 0;
    while (i < cls.size() && std::isdigit((unsigned char) cls[i]) == 0) ++i;
    size_t j = i;
    while (j < cls.size() && std::isdigit((unsigned char) cls[j]) != 0) ++j;
    if (i == j || j == cls.size()) return {};
    return {cls.substr(0, i), cls.substr(j), std::stoi(cls.substr(i, j - i))};
}

inline std::string sized(const Name& n, int count) {
    return n.prefix + std::to_string(count) + n.family;
}

inline int sizeFor(const Name& n, int needed) {
    int best = 0, largest = 0;
    for (int k = 1; k <= 64; ++k) {
        if (!Registry::instance().isKnown(sized(n, k))) continue;
        largest = k;
        if (k >= needed && (best == 0 || k < best)) best = k;
    }
    return best != 0 ? best : largest;
}

}
