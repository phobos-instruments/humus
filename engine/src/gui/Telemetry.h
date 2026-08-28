#pragma once
#include <juce_core/juce_core.h>

#include "core/AppPaths.h"
#include "core/TelemetrySpool.h"
#include "gui/AppSettings.h"

namespace hum {

inline TelemetrySpool& telemetrySpool() {
    static TelemetrySpool spool;
    return spool;
}

inline juce::File telemetryFile() {
    return appDataDir().getChildFile("telemetry.json");
}

inline void telemetryCount(const std::string& name, juce::int64 n = 1) {
    telemetrySpool().count(name, n);
}

inline void telemetrySave() {
    if (telemetrySpool().empty()) telemetryFile().deleteFile();
    else telemetryFile().replaceWithText(telemetrySpool().toJson());
}

inline void telemetrySyncConsent() {
    const bool on = AppSettings::instance().getInt("telemetry.enabled", 0) != 0;
    telemetrySpool().setEnabled(on);
    if (!on) {
        telemetrySpool().clearAfterFlush();
        telemetrySave();
    }
}

}
