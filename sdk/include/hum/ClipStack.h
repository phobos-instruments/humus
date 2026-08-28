#pragma once
#include <algorithm>
#include <vector>

#include "hum/Pattern.h"

namespace hum::clipstack {

struct Span { int startTick = 0; int lengthTicks = 0; };

inline constexpr int kEndlessTicks = 1 << 28;

inline bool placed(const PatternChannel& ch) {
    return ch.type != "trigger-timepoints" && ch.startTick >= 0 && ch.lengthTicks > 0;
}

inline Span extentOf(const PatternChannel& ch) {
    return {ch.startTick, ch.loopClip ? kEndlessTicks : ch.lengthTicks};
}

inline std::vector<Span> soundingSpans(const Pattern& p, int index) {
    std::vector<Span> out;
    if (index < 0 || index >= (int) p.channels.size()) return out;
    const auto& me = p.channels[(size_t) index];
    if (!placed(me)) {
        out.push_back({std::max(0, me.startTick), kEndlessTicks});
        return out;
    }
    Span base = extentOf(me);
    if (me.loopClip)
        for (int i = index + 1; i < (int) p.channels.size(); ++i) {
            const auto& other = p.channels[(size_t) i];
            if (!placed(other) || other.startTick <= me.startTick) continue;
            base.lengthTicks = std::min(base.lengthTicks, other.startTick - me.startTick);
        }
    out.push_back(base);
    for (int i = index + 1; i < (int) p.channels.size(); ++i) {
        const auto& other = p.channels[(size_t) i];
        if (!placed(other)) continue;
        const auto oe = extentOf(other);
        const int cs = oe.startTick, ce = cs + oe.lengthTicks;
        std::vector<Span> next;
        for (const auto& s : out) {
            const int ss = s.startTick, se = ss + s.lengthTicks;
            if (ce <= ss || cs >= se) { next.push_back(s); continue; }
            if (cs > ss) next.push_back({ss, cs - ss});
            if (ce < se) next.push_back({ce, se - ce});
        }
        out.swap(next);
        if (out.empty()) break;
    }
    return out;
}

inline bool hasTakes(const Pattern& p) {
    for (int i = 0; i < (int) p.channels.size(); ++i) {
        if (!placed(p.channels[(size_t) i])) continue;
        for (int j = i + 1; j < (int) p.channels.size(); ++j) {
            const auto& a = p.channels[(size_t) i];
            const auto& b = p.channels[(size_t) j];
            if (!placed(b)) continue;
            if (a.startTick < b.startTick + b.lengthTicks
                && b.startTick < a.startTick + a.lengthTicks) return true;
        }
    }
    return false;
}

}
