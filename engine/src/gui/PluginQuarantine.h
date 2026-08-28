#pragma once
#include <string>

#include <juce_core/juce_core.h>

#include "gui/AppSettings.h"

namespace hum::quarantine {

inline juce::StringArray list() {
    juce::StringArray a;
    a.addTokens(AppSettings::instance().getString("plugins.uiQuarantine", ""), "\n", "");
    a.removeEmptyStrings();
    return a;
}

inline bool contains(const std::string& classRaw) {
    return list().contains(juce::String(classRaw));
}

inline void add(const std::string& classRaw) {
    auto a = list();
    a.addIfNotAlreadyThere(juce::String(classRaw));
    AppSettings::instance().set("plugins.uiQuarantine", a.joinIntoString("\n"));
}

inline void remove(const std::string& classRaw) {
    auto a = list();
    a.removeString(juce::String(classRaw));
    AppSettings::instance().set("plugins.uiQuarantine", a.joinIntoString("\n"));
}

inline void clear() { AppSettings::instance().set("plugins.uiQuarantine", ""); }

}
