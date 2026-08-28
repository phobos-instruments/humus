#pragma once
#include <string>

#include <juce_core/juce_core.h>

#if !JUCE_WINDOWS
#include <signal.h>
#include <unistd.h>
#endif

#include "core/UiWatchdog.h"
#include "gui/AppSettings.h"

namespace hum {

inline bool matchesPluginList(const std::string& classRaw, const char* setting,
                              const char* defaults) {
    juce::StringArray list;
    list.addTokens(AppSettings::instance().getString(setting, defaults), "\n", "");
    list.removeEmptyStrings();
    for (const auto& s : list)
        if (juce::String(classRaw).containsIgnoreCase(s)) return true;
    return false;
}

inline bool shouldLeakEditor(const std::string& classRaw) {
    return matchesPluginList(classRaw, "plugins.leakEditor", "ParametricOD");
}

inline bool isFragileEditor(const std::string& classRaw) {
    return matchesPluginList(classRaw, "plugins.fragileEditor", "ParametricOD");
}

inline bool isWindowedEditor(const std::string& classRaw) {
    return matchesPluginList(classRaw, "plugins.windowedEditor", "");
}

inline void enrollWindowedEditor(const std::string& classRaw) {
    juce::StringArray list;
    list.addTokens(AppSettings::instance().getString("plugins.windowedEditor", ""),
                   "\n", "");
    list.removeEmptyStrings();
    list.addIfNotAlreadyThere(
        juce::String(classRaw).upToFirstOccurrenceOf("?", false, false));
    AppSettings::instance().set("plugins.windowedEditor", list.joinIntoString("\n"));
}

class EditorOpGuard {
public:
    EditorOpGuard(const std::string& classRaw, const char* listSetting,
                  const char* opDescription) {
        crumb().replaceWithText(juce::String(listSetting) + "\n"
                                + juce::String(classRaw) + "\n"
                                + juce::String(opDescription) + "\n"
#if !JUCE_WINDOWS
                                + juce::String((int) ::getpid()) + "\n");
#else
                                + "0\n");
#endif
    }
    ~EditorOpGuard() { crumb().deleteFile(); }
    EditorOpGuard(const EditorOpGuard&) = delete;
    EditorOpGuard& operator=(const EditorOpGuard&) = delete;

    static juce::String sweepAtStartup() {
        auto f = crumb();
        if (!f.existsAsFile()) return {};
        juce::StringArray lines;
        lines.addLines(f.loadFileAsString());
        const int pid = lines.size() > 3 ? lines[3].getIntValue() : 0;
#if !JUCE_WINDOWS
        if (pid > 0 && ::kill((pid_t) pid, 0) == 0) return {};
#endif
        f.deleteFile();
        if (lines.size() < 3 || lines[0].isEmpty() || lines[1].isEmpty()) return {};
        const juce::String setting = lines[0], cls = lines[1], op = lines[2];
        const char* defaults =
            setting == "plugins.leakEditor" || setting == "plugins.fragileEditor"
                ? "ParametricOD"
                : setting == "plugins.leakInstance" ? "Synplant" : "";
        juce::StringArray list;
        list.addTokens(AppSettings::instance().getString(setting, defaults), "\n", "");
        list.removeEmptyStrings();
        list.addIfNotAlreadyThere(cls);
        AppSettings::instance().set(setting, list.joinIntoString("\n"));
        return cls.upToFirstOccurrenceOf("?", false, false)
             + " crashed while " + op + " last session.\n\n"
             + "It now uses a safe fallback automatically. To retry the normal "
               "path, remove it from \"" + setting + "\" in settings.xml.";
    }

private:
    static juce::File crumb() {
        auto f = UiWatchdog::configDir().getChildFile("editor-op-pedal.txt");
        f.getParentDirectory().createDirectory();
        return f;
    }
};

}
