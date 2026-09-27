// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include "io/PatchDocument.h"

namespace hum::windowfloat {

inline bool fillsDisplay(int screen, int displays) { return screen >= 1 && screen <= displays; }

inline bool staysAbove(int screen, int displays, bool keepOnTop) {
    return fillsDisplay(screen, displays) || keepOnTop;
}

inline bool reopensOnLoad(int screen) { return screen >= 1; }

inline int keptGuard(int guardedScreen, int wanted) {
    return guardedScreen > 0 && wanted != guardedScreen ? 0 : guardedScreen;
}

inline int effectiveScreen(int screen, int displays, int guardedScreen) {
    if (!fillsDisplay(screen, displays)) return 0;
    return guardedScreen > 0 && screen == guardedScreen ? 0 : screen;
}

inline std::vector<std::string> outputsToReopen(const PatchDocumentModel& model,
                                                const char* screenParam) {
    std::vector<std::string> out;
    for (const auto& cm : model.organisms)
        for (const auto& p : cm.properties)
            if (p.name == screenParam && reopensOnLoad((int) p.value)) out.push_back(cm.name);
    return out;
}

}
