#pragma once
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "gui/AppSettings.h"
#include "gui/ModernPicker.h"
#include "gui/QuickAddPalette.h"
#include "gui/Telemetry.h"
#include "gui/TelemetryEvents.h"

namespace hum {

inline bool modernMenusEnabled() {
    return AppSettings::instance().getString("menu.style", "classic") == "modern";
}

inline void setModernMenus(bool on) {
    AppSettings::instance().set("menu.style", juce::String(on ? "modern" : "classic"));
}

inline void showCreatePicker(juce::Rectangle<int> screenAnchor,
                             std::function<void(const std::string&)> onPick) {
    auto counted = [onPick = std::move(onPick)](const std::string& cls) {
        telemetryCount(telemetry::organismAdd(cls));
        onPick(cls);
    };
    if (modernMenusEnabled())
        ModernPicker::show(screenAnchor, std::move(counted));
    else
        QuickAddPalette::show(screenAnchor, std::move(counted));
}

}
