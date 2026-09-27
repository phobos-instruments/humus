// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>

#include <juce_core/juce_core.h>

#include "core/app/UsageTally.h"
#include "gui/app/AboutCredits.h"
#include "gui/app/AppSettings.h"
#include "gui/common/Localisation.h"

namespace hum::usagelog {

inline usage::Tally read() {
    auto& st = AppSettings::instance();
    usage::Tally t;
    t.openSeconds = st.getDouble("usage.openSeconds", 0.0);
    t.bouncedSeconds = st.getDouble("usage.bouncedSeconds", 0.0);
    t.bounces = st.getInt("usage.bounces", 0);
    return t;
}

inline void write(const usage::Tally& t) {
    auto& st = AppSettings::instance();
    st.beginBatch();
    st.set("usage.openSeconds", t.openSeconds);
    st.set("usage.bouncedSeconds", t.bouncedSeconds);
    st.set("usage.bounces", t.bounces);
    st.endBatch();
}

inline void addOpenSeconds(double seconds) {
    auto t = read();
    t.openSeconds += seconds;
    write(t);
}

inline juce::String summary(const usage::Tally& t) {
    return tr("usage.open-for", "Open for") + " " + juce::String(usage::spanText(t.openSeconds)) + "  -  "
         + juce::String(usage::spanText(t.bouncedSeconds)) + " " + tr("usage.bounced", "bounced");
}

inline void openDonatePage() { juce::URL(juce::String(about::kSite) + "/donate").launchInDefaultBrowser(); }

}
