// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/settings/Appearance.h"

#include "gui/app/AppSettings.h"
#include "gui/settings/ThemeStore.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

void applySavedAppearance() {
    auto& s = AppSettings::instance();
    if (s.getInt("themeListVersion", 1) < 2) {
        static constexpr int kRemap[] = {0, 2, 4, 0, 5};
        const int old = s.getInt("themeIndex", 0);
        if (old >= 0 && old < 5) s.set("themeIndex", kRemap[old]);
        s.set("themeListVersion", 2);
    }
    if (s.getInt("themeListVersion", 1) < 3) {
        static constexpr int kRemap[] = {0, 1, 2, 2, 0, 1, 3, 1,
                                         3, 4, 2, 4, 4, 3, 3, 4};
        const int old = s.getInt("themeIndex", 0);
        if (old >= 0 && old < (int) (sizeof(kRemap) / sizeof(kRemap[0])))
            s.set("themeIndex", kRemap[old]);
        s.set("themeListVersion", 3);
    }
    const auto mode = s.getString("themeMode", "preset");
    if (mode == "custom") {
        setBaseColours(themeColoursFromString(s.getString("customColours"), baseColours()));
    } else if (mode == "user") {
        ThemeColours c;
        if (themestore::find(s.getString("themeName"), c)) setBaseColours(c);
        else applyTheme(0);
    } else {
        applyTheme(s.getInt("themeIndex", 0));
    }
    setAppearanceAdjustments(s.getDouble("brightness", 0.0), s.getDouble("contrast", 0.0));
}

}
