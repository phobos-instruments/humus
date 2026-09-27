// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include "io/PatchDocument.h"

namespace hum {

inline void keepTailOutlets(std::vector<ConnectionModel>& cords, const std::string& name,
                            int oldOutlets, int newOutlets, int tail) {
    if (tail <= 0 || oldOutlets == newOutlets || oldOutlets < tail || newOutlets < tail) return;
    const int oldTail = oldOutlets - tail, newTail = newOutlets - tail;
    cords.erase(std::remove_if(cords.begin(), cords.end(),
                               [&](const ConnectionModel& c) {
                                   return c.src == name && c.srcOutlet >= newTail && c.srcOutlet < oldTail;
                               }),
                cords.end());
    for (auto& c : cords)
        if (c.src == name && c.srcOutlet >= oldTail && c.srcOutlet < oldOutlets)
            c.srcOutlet = newTail + (c.srcOutlet - oldTail);
}

inline void reclaimCordsForResize(std::vector<ConnectionModel>& cords,
                                  const std::string& name, int dstPorts, int srcPorts) {
    auto identical = [](const ConnectionModel& a, const ConnectionModel& b) {
        return a.src == b.src && a.srcOutlet == b.srcOutlet && a.dst == b.dst
            && a.dstInlet == b.dstInlet && a.midiChannel == b.midiChannel;
    };
    auto side = [&](bool dstSide, int count) {
        auto mine = [&](const ConnectionModel& c) {
            return dstSide ? c.dst == name : c.src == name;
        };
        auto port = [&](ConnectionModel& c) -> int& {
            return dstSide ? c.dstInlet : c.srcOutlet;
        };
        if (count <= 0) {
            cords.erase(std::remove_if(cords.begin(), cords.end(), mine), cords.end());
            return;
        }
        std::vector<char> used((size_t) count, 0);
        for (auto& c : cords)
            if (mine(c) && port(c) < count) used[(size_t) port(c)] = 1;
        std::vector<size_t> merged;
        for (size_t i = 0; i < cords.size(); ++i) {
            auto& c = cords[i];
            if (!mine(c) || port(c) < count) continue;
            int f = 0;
            while (f < count && used[(size_t) f]) ++f;
            if (f < count) { port(c) = f; used[(size_t) f] = 1; }
            else           { port(c) = count - 1; merged.push_back(i); }
        }
        for (size_t k = merged.size(); k-- > 0;) {
            const size_t m = merged[k];
            for (size_t i = 0; i < cords.size(); ++i)
                if (i != m && identical(cords[i], cords[m])) {
                    cords.erase(cords.begin() + (std::ptrdiff_t) m);
                    break;
                }
        }
    };
    side(true, dstPorts);
    side(false, srcPorts);
}

}
