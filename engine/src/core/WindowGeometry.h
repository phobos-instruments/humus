#pragma once
#include <cstddef>
#include <cstdlib>
#include <string>

namespace hum {

inline bool wantsOnboarding(bool guideSeen, bool setupCompleted) {
    return !guideSeen && !setupCompleted;
}

struct WindowSize { int w = 0, h = 0; };

inline constexpr int kFirstRunMaxW = 1920, kFirstRunMaxH = 1080;

inline WindowSize firstRunWindowSize(int screenW, int screenH, int minW, int minH) {
    auto pick = [](int screen, int lo, int hi) {
        const int want = screen * 96 / 100 < hi ? screen * 96 / 100 : hi;
        return want < lo ? lo : want;
    };
    return {pick(screenW, minW, kFirstRunMaxW), pick(screenH, minH, kFirstRunMaxH)};
}

inline WindowSize savedWindowSize(const std::string& state) {
    int vals[4] = {0, 0, 0, 0};
    int n = 0;
    std::size_t i = 0;
    while (i < state.size() && n < 4) {
        while (i < state.size() && state[i] == ' ') ++i;
        const std::size_t start = i;
        while (i < state.size() && state[i] != ' ') ++i;
        if (start == i) break;
        const std::string t = state.substr(start, i - start);
        if (n == 0 && (t[0] == 'f' || t[0] == 'F')) continue;
        char* end = nullptr;
        const long v = std::strtol(t.c_str(), &end, 10);
        if (end == t.c_str()) return {};
        vals[n++] = (int) v;
    }
    return n < 4 ? WindowSize{} : WindowSize{vals[2], vals[3]};
}

inline bool savedWindowSizeIsUsable(const std::string& state, int minW, int minH) {
    const auto s = savedWindowSize(state);
    return s.w >= minW && s.h >= minH;
}

}
